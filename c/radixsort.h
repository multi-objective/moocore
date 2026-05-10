/*****************************************************************************

   This file implements a LDS radix sort for floating-point values:

   - Sorting is stable.
   - Signed zeros are treated as unsigned zeros.
   - Fallback to insertion sort for sizes not larger than RADIX_INSERTION_THRESHOLD.
   - Terdiman's skip test (to skip passes)
   - Lexicographic sorting only sorts tied keys and stops as soon as there are no ties.
   - Travis Downs's queue pointers instead of explicit prefix sums.
     https://travisdowns.github.io/blog/2019/05/22/sorting.html

   Ideas for future improvements are:

   - https://github.com/DragonSpit/ParallelAlgorithms
   - https://github.com/ClickHouse/ClickHouse/blob/master/src/Common/RadixSort.h
   - https://github.com/fastverse/collapse/blob/master/src/base_radixsort.c
   - https://github.com/Rdatatable/data.table/blob/master/src/forder.c
   - https://github.com/ramou/dfr
   - https://probablydance.com/2017/01/17/faster-sorting-algorithm-part-2/
   - https://arxiv.org/abs/1008.2849

   Runtime should be O(K * n * dim), where n is the number of elements, dim the
   number of dimensions (in case of lexicographic sorting) and K is a constant
   that is at least 10 (keys encoding + histogram building + 8 radix passes).

   Do not believe anything that AI models tell you about how to make this code
   run faster without thorough testing.  It is very likely that AI suggestions
   will make the code more complex but not faster.

 ---------------------------------------------------------------------

 Copyright (C) 2026
 Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

*****************************************************************************/
#ifndef RADIXSORT_H
#define RADIXSORT_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h> // memcpy, memmove
#include "insort.h"
#include "common.h"
#include "maxminclamp.h" // SWAP()

#ifndef RADIX_INSERTION_THRESHOLD
#define RADIX_INSERTION_THRESHOLD 128
#endif

typedef struct tie_run_length_t {
    uint32_t start;
    uint32_t len;
} tie_run_length_t;

typedef struct radix_doublep_ws {
    uint32_t * restrict idxbuf;
    uint64_t * restrict keys;
    const double ** restrict ptrs_tmp;
    tie_run_length_t * restrict ties;
} radix_doublep_ws;

typedef enum { NOT_SORTED = 0, SORTED = 1, REV_SORTED = 2 } sort_result_t;

static void
radix_doublep_ws_alloc(radix_doublep_ws * ws, uint32_t n, dimension_t dim)
{
    ws->idxbuf = malloc(2 * n * sizeof(*ws->idxbuf));
    ws->keys = malloc(n * sizeof(*ws->keys));
    ws->ptrs_tmp = malloc(n * sizeof(*ws->ptrs_tmp));
    ws->ties = dim ? malloc((n/2) * sizeof(*ws->ties)) : NULL;
}

static void
radix_doublep_ws_free(radix_doublep_ws * ws)
{
    free(ws->idxbuf);
    free(ws->keys);
    free(ws->ptrs_tmp);
    if (ws->ties)
        free(ws->ties);
    ws->idxbuf = NULL;
    ws->keys = NULL;
    ws->ptrs_tmp = NULL;
    ws->ties = NULL;
}

static __force_inline__ uint64_t
load_u64_from_double(double x)
{
    STATIC_ASSERT(sizeof(uint64_t) == sizeof(double), "sizeof(uint64_t) != sizeof(double)");
    uint64_t u;
    memcpy(&u, &x, sizeof(u));
    return u;
}

static __force_inline__ uint8_t
key_nth_byte(uint64_t key, unsigned byte_index)
{
    return (key >> (byte_index << 3)) & 0xFFu;
}

/**
   FIXME: This function requires IEEE-754 double values, but there is no
   standard way to check this at compile time as far as I know.
 */
static __force_inline__ uint64_t
double_to_sort_key_asc(double x)
{
    uint64_t u = load_u64_from_double(x);
    /* Canonicalise both +0.0 and -0.0 to +0.0.

       IEEE-754 +0.0: 0x0000000000000000
       IEEE-754 -0.0: 0x8000000000000000
    */
    if (unlikely(u == UINT64_C(0x8000000000000000)))
        u = 0;

    uint64_t mask = (uint64_t)-(int64_t)(u >> 63) | UINT64_C(0x8000000000000000);
    return u ^ mask;
}

static __force_inline__ uint64_t
double_to_sort_key_des(double x)
{
    return ~double_to_sort_key_asc(x);
}

static __force_inline__ uint64_t
double_to_sort_key(double x, bool descending)
{
    return descending ? double_to_sort_key_des(x) : double_to_sort_key_asc(x);
}

static inline sort_result_t
build_keys(uint64_t * restrict keys,
           const double ** restrict ptrs, uint32_t n,
           bool descending, dimension_t col,
           tie_run_length_t * restrict ties, size_t * restrict nties_p)
{
    assert((ties == NULL) == (nties_p == NULL));
    keys[0] = double_to_sort_key(ptrs[0][col], descending);
    keys[1] = double_to_sort_key(ptrs[1][col], descending);
    uint32_t i = 2;
    if (keys[0] <= keys[1]) {
        bool in_ties = keys[0] == keys[1];
        size_t nties = 0;
        if (ties)
            ties->start = 0;
        // Check if already sorted.
        for (; i < n; i++) {
            keys[i] = double_to_sort_key(ptrs[i][col], descending);
            if (keys[i-1] > keys[i])
                goto not_sorted;
            if (ties) {
                if (keys[i-1] == keys[i]) {
                    if (!in_ties) {
                        ties->start = i - 1;
                        in_ties = true;
                    }
                } else if (in_ties) {
                    ties->len = i - ties->start;
                    assert(ties->len > 1);
                    ties++, nties++;
                    in_ties = false;
                }
            }
        }
        assert(i == n);
        if (ties) {
            if (in_ties) {
                ties->len = n - ties->start;
                assert(ties->len > 1);
                ties++, nties++;
            }
            *nties_p = nties;
        }
        return SORTED;
    } else {
        // FIXME: '<=' assumes that we can reverse it if there are no
        // duplicates.  We could relax this condition to '<', then reverse by
        // chunks of non-equal values.
        for (; i < n; i++) {
            keys[i] = double_to_sort_key(ptrs[i][col], descending);
            if (keys[i-1] <= keys[i])
                goto not_sorted;
        }
        assert(i == n);
        return REV_SORTED;
    }
not_sorted:
    for (i++; i < n; i++)
        keys[i] = double_to_sort_key(ptrs[i][col], descending);
    return NOT_SORTED;
}

static __force_inline__ unsigned
radix_sort_build_hist(uint32_t hist[8][256], unsigned shift[8],
                      const uint64_t * restrict keys, uint32_t n)
{
    // Build all histograms simultaneously.
    for (uint32_t i = 0; i < n; i++) {
        uint64_t k = keys[i];
        hist[0][key_nth_byte(k, 0)]++;
        hist[1][key_nth_byte(k, 1)]++;
        hist[2][key_nth_byte(k, 2)]++;
        hist[3][key_nth_byte(k, 3)]++;
        hist[4][key_nth_byte(k, 4)]++;
        hist[5][key_nth_byte(k, 5)]++;
        hist[6][key_nth_byte(k, 6)]++;
        hist[7][key_nth_byte(k, 7)]++;
    }
    /*
      Terdiman skip test. See https://codercorner.com/RadixSortRevisited.htm

      If all keys have the same byte in a pass, that stable radix pass cannot
      change the order and can be omitted. It is sufficient to test the bucket
      corresponding to any key's byte; here we use keys[n-1] because it is the
      one last processed above.  If that bucket has count n, all bytes in this
      pass are equal. Otherwise the pass is needed.
    */
    const uint64_t sample = keys[n - 1];
    unsigned total_passes = 0;
    for (unsigned pass = 0; pass < 8; pass++) {
        uint32_t * hist_src = hist[pass];
        if (hist_src[key_nth_byte(sample, pass)] == n)
            continue;
        // Pre-calculate the shift for this pass.
        shift[total_passes] = pass << 3;
        uint32_t * hist_dst = hist[total_passes];
        total_passes++;
        // Prefix sum: (1) shift hist_src values one place up, so [0] is free.
        memmove(hist_dst + 1, hist_src, 255 * sizeof(*hist_src));
        // (2) Cumulative sum.
        hist_dst[0] = 0;
        for (int b = 2; b < 256; b++)
            hist_dst[b] += hist_dst[b-1];
    }

    DEBUG2(
        for (unsigned pass = 0; pass < total_passes; pass++) {
            uint32_t * restrict count = hist[pass];
            fprintf(stderr, "hist[%u] = { %d", pass, count[0]);
            for (int b = 1; b < 256; b++)
                fprintf(stderr, " %d", count[b]);
            fprintf(stderr, " }\n");
        });

    /* If all 8 bytes are identical for all keys, the original order is already
       sorted, so we should have detected this earlier.  */
    ASSUME(total_passes > 0);
    ASSUME(total_passes <= 8);
    return total_passes;
}

static __force_inline__ void
setup_queue(uint32_t * restrict queue[256], const uint32_t * restrict count, uint32_t * restrict dst)
{
    // Setup queues (buckets) as pointers into the destination buffer.
    /* count[0] is always zero so we could skip i=0, but this loop is
       vectorized better by GCC than the alternatives.  */
    for (int i = 0; i < 256; i++)
        queue[i] = dst + count[i];
}

static __force_inline__ uint32_t *
radix_sort_u64_range(uint32_t * restrict idxbuf, const uint64_t * restrict keys, uint32_t n)
{
    ASSUME(n > RADIX_INSERTION_THRESHOLD);
    uint32_t hist[8][256] = { {0} };
    unsigned shift[8];
    unsigned total_passes = radix_sort_build_hist(hist, shift, keys, n);
    uint32_t * src = idxbuf;
    uint32_t * dst = idxbuf + n;

    uint32_t * queue[256];
    setup_queue(queue, hist[0], dst);
    unsigned byte_shift = shift[0];
    // This first pass avoids the use of src[i].
    for (uint32_t i = 0; i < n; i++) {
        // Copy each index into the appropriate queue based on the byte value of the key.
        // FIXME: We access the keys in bytes so why not store them as
        // keys[pass][idx] and process 8*8 at a time?
        uint8_t b = (keys[i] >> byte_shift) & 0xFFu;
        *queue[b]++ = i; // append to queue.
    }
    for (unsigned pass = 1; pass < total_passes; pass++) {
        SWAP(src, dst);
        setup_queue(queue, hist[pass], dst);
        byte_shift = shift[pass];
        for (uint32_t i = 0; i < n; i++) {
            /* One may think that resorting keys after each pass may be faster
               than this indirection, but there seems to be no difference.  */
            uint32_t idx = src[i];
            uint8_t b = (keys[idx] >> byte_shift) & 0xFFu;
            *queue[b]++ = idx; // append to queue.
        }
    }
    DEBUG2(fprintf(stderr, "idx = %u", dst[0]);
           for (uint32_t i = 1; i < n; i++)
               fprintf(stderr, " %u", dst[i]);
           fprintf(stderr, "\n"));
    return dst;
}

/**
   At the end of this function ptrs are sorted and, optionally, keys and order.
*/
static inline sort_result_t
radix_sort_doublep_range_ws_asc(const radix_doublep_ws * restrict ws,
                                const double ** restrict ptrs, uint32_t n, dimension_t col,
                                uint32_t * restrict order,
                                tie_run_length_t * restrict ties, size_t * restrict nties_p)
{
    assert((ties == NULL) == (nties_p == NULL));
    uint64_t * keys = ws->keys;
    sort_result_t res = build_keys(keys, ptrs, n, /*desc=*/false, col, ties, nties_p);
    if (res == NOT_SORTED) {
        uint32_t * restrict perm = radix_sort_u64_range(ws->idxbuf, keys, n);
        // FIXME: Do we really need to sort ptrs here for recursive sorting?
        reorder_doublep(ptrs, ws->ptrs_tmp, perm, n);
        if (ties) {
            bool in_ties = false;
            size_t nties = 0;
            uint64_t prev_key = keys[perm[0]];
            for (uint32_t i = 1; i < n; i++) {
                uint64_t key = keys[perm[i]];
                if (prev_key == key) {
                    if (!in_ties) {
                        ties->start = i - 1;
                        in_ties = true;
                    }
                } else {
                    prev_key = key;
                    if (in_ties) {
                        ties->len = i - ties->start;
                        assert(ties->len > 1);
                        ties++, nties++;
                        in_ties = false;
                    }
                }
            }
            if (in_ties) {
                ties->len = n - ties->start;
                assert(ties->len > 1);
                ties++, nties++;
            }
            *nties_p = nties;
        }
        if (order != NULL) {
            // Re-order the order array.
            uint32_t * order_tmp = ws->idxbuf + ((perm == ws->idxbuf) ? n : 0);
            reorder_u32(order, order_tmp, perm, n);
        }
    } else if (res == REV_SORTED) {
        reverse_doublep(ptrs, n);
        // REV_SORTED implies that there are no ties.
        if (nties_p)
            *nties_p = 0;
        if (order != NULL)
            reverse_u32(order, n);

    }
    return res;
}

static inline void
radix_sort_asc_rev_rec(const radix_doublep_ws * restrict ws,
                       const double ** restrict rows, uint32_t n, dimension_t col,
                       uint32_t * restrict order, tie_run_length_t * restrict ties)
{
    ASSUME(col > 0);
    size_t nties;
    radix_sort_doublep_range_ws_asc(ws, rows, n, col, order, ties, &nties);
    if (nties == 0) // No ties, early exit.
        return;

    col--; // Done with col, move to the next one.
    // Process ties in reverse order to re-use the ties buffer.
    tie_run_length_t * this_tie = ties + nties;
    do {
        this_tie--;
        uint32_t start = this_tie->start;
        uint32_t len = this_tie->len;
        assert(len > 1);
        const double ** rows_start = rows + start;
        uint32_t * order_start = order ? order + start : NULL;
        if (len <= RADIX_INSERTION_THRESHOLD) {
            if (order == NULL)
                insertion_sort_pdouble_asc_rev(rows_start, len, col);
            else
                insertion_argsort_pdouble_asc_rev(rows_start, len, col, order_start);
        } else if (col == 0) {
            radix_sort_doublep_range_ws_asc(ws, rows_start, len, col, order_start,
                                            /*ties = */NULL, /*nties_p=*/NULL);
        } else {
            radix_sort_asc_rev_rec(ws, rows_start, len, col, order_start, this_tie);
        }
    } while (this_tie != ties);
}

static inline void
radix_argsort_asc_only(const double **rows, size_t len, dimension_t col, uint32_t * restrict order)
{
    if (len <= RADIX_INSERTION_THRESHOLD) {
        if (order == NULL)
            insertion_sort_pdouble_asc_only(rows, len, col);
        else
            insertion_argsort_pdouble_asc_only(rows, len, col, order);
        return;
    }
    uint32_t n = (uint32_t) len;
    radix_doublep_ws ws;
    radix_doublep_ws_alloc(&ws, n, 0);
    radix_sort_doublep_range_ws_asc(&ws, rows, n, col, order, NULL, NULL);
    radix_doublep_ws_free(&ws);
}

static inline void
radix_sort_asc_only(const double **rows, size_t len, dimension_t col)
{
    radix_argsort_asc_only(rows, len, col, /*order=*/NULL);
}

static void
radix_argsort_asc_rev(const double ** restrict rows, size_t len, dimension_t col, uint32_t * restrict order)
{
    if (len <= RADIX_INSERTION_THRESHOLD) {
        if (order == NULL)
            insertion_sort_pdouble_asc_rev(rows, len, col);
        else
            insertion_argsort_pdouble_asc_rev(rows, len, col, order);
        return;
    }
    assert(len <= UINT32_MAX);
    uint32_t n = (uint32_t) len;
    radix_doublep_ws ws;
    radix_doublep_ws_alloc(&ws, n, col);
    radix_sort_asc_rev_rec(&ws, rows, n, col, order, ws.ties);
    radix_doublep_ws_free(&ws);
}

static void
radix_sort_asc_rev(const double **rows, size_t len, dimension_t col)
{
    radix_argsort_asc_rev(rows, len, col, /*order=*/NULL);
}

static inline void
radix_sort_asc_1d(const double **rows, size_t len)
{
    radix_sort_asc_only(rows, len, 0);
}

static inline void
radix_argsort_asc_1d(const double **rows, size_t len, uint32_t * restrict order)
{
    radix_argsort_asc_only(rows, len, 0, order);
}

static inline void
radix_sort_asc_only_3d(const double **rows, size_t len)
{
    radix_sort_asc_only(rows, len, 2);
}

static inline void
radix_sort_asc_only_4d(const double **rows, size_t len)
{
    radix_sort_asc_only(rows, len, 3);
}

static inline void
radix_sort_asc_rev_2d(const double **rows, size_t len)
{
    radix_sort_asc_rev(rows, len, 1);
}

static inline void
radix_sort_asc_rev_3d(const double **rows, size_t len)
{
    radix_sort_asc_rev(rows, len, 2);
}

static inline void
radix_argsort_asc_rev_3d(const double ** restrict rows, size_t len, uint32_t * restrict order)
{
    radix_argsort_asc_rev(rows, len, 2, order);
}


#endif // RADIXSORT_H
