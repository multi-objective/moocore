#ifndef INSERTION_SORT_H
#define INSERTION_SORT_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h> // memcpy, memmove
#include "common.h"
#include "sort.h"

static __force_inline__ void
identity_u32(uint32_t * restrict ptrs, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
        ptrs[i] = i;
}

#define DEFINE_REVERSE_FUNC(DTYPE, NAME)                        \
static __force_inline__ void NAME(DTYPE * restrict ptrs, size_t n)         \
{                                                                          \
    for (size_t i = 0, j = n - 1; i < j; ++i, --j)                         \
        SWAP(ptrs[i], ptrs[j]);                                            \
}

DEFINE_REVERSE_FUNC(uint32_t, reverse_u32)
DEFINE_REVERSE_FUNC(uint64_t, reverse_u64)
DEFINE_REVERSE_FUNC(const double *, reverse_doublep)
//--------------------------------------------------------------------------

/**
   PRAGMA_ASSUME_NO_VECTOR_DEPENDENCY helps to vectorize but only in very
   modern CPUs.
*/
#define DEFINE_REORDER_FUNC(DTYPE, NAME)                            \
static __force_inline__ void                                        \
NAME(DTYPE * restrict dst, DTYPE * restrict tmp,                        \
     uint32_t * restrict order, size_t n)                               \
{                                                                       \
    memcpy(tmp, dst, n * sizeof(*dst));                                 \
    PRAGMA_ASSUME_NO_VECTOR_DEPENDENCY                                  \
    for (size_t i = 0; i < n; i++)                                      \
        dst[i] = tmp[order[i]];                                         \
}

DEFINE_REORDER_FUNC(uint32_t, reorder_u32)
DEFINE_REORDER_FUNC(uint64_t, reorder_u64)
DEFINE_REORDER_FUNC(const double *, reorder_doublep)
//--------------------------------------------------------------------------

/**
   Stable binary insertion sort of row pointers.
   data points to an array of len pointers, each pointing to ncol doubles.
*/
static inline void
insertion_sort_r(const double ** restrict data, size_t len, cmp_r_fun_t cmp,
                 void * restrict arg)
{
    for (size_t i = 1; i < len; i++) {
        const double * restrict key = data[i];
        // Find insertion point in data[0..i) using binary search.
        size_t lo = 0, hi = i;
        do {
            size_t mid = lo + (hi - lo) / 2;
            if (cmp(key, data[mid], arg) < 0)
                hi = mid;
            else
                lo = mid + 1;
        } while (lo < hi);

        // Shift pointers up by one and insert key.
        if (lo != i) {
            memmove(data + lo + 1, data + lo, (i - lo) * sizeof(*data));
            data[lo] = key;
        }
    }
}

/**
   Stable binary insertion sort of row pointers.
   data points to an array of len pointers, each pointing to ncol doubles.

   This function sorts both data and ord simultaneously.
*/
static inline void
insertion_argsort_r(const double ** restrict data, size_t len,
                    uint32_t * restrict order,
                    cmp_r_fun_t cmp, void * restrict arg)
{
    ASSUME(len <= UINT32_MAX);
    uint32_t * perm = malloc(len * sizeof(*perm));
    identity_u32(perm, (uint32_t)len);

    for (size_t i = 1; i < len; i++) {
        uint32_t key = perm[i];
        const double * restrict key_row = data[key];
        // Find insertion point in data[0..i) using binary search.
        size_t lo = 0, hi = i;
        do {
            size_t mid = lo + (hi - lo) / 2;
            const double * mid_row = data[perm[mid]];
            if (cmp(key_row, mid_row, arg) < 0)
                hi = mid;
            else
                lo = mid + 1;
        } while (lo < hi);

        // Shift pointers up by one and insert key.
        if (lo != i) {
            memmove(perm + lo + 1, perm + lo, (i - lo) * sizeof(*perm));
            perm[lo] = key;
        }
    }
    // Apply the permutation.
    const double ** tmp_data = malloc(len * sizeof(*tmp_data));
    reorder_doublep(data, tmp_data, perm, len);
    free(tmp_data);

    uint32_t * tmp_order = malloc(len * sizeof(*tmp_order));
    reorder_u32(order, tmp_order, perm, len);
    free(tmp_order);

    free(perm);
}

static __force_inline__ void
insertion_sort_pdouble_asc_1d(const double * data[], size_t len)
{
    insertion_sort_r(data, len, qsort_r_cmp_pdouble_asc_1d, NULL);
}

static __force_inline__ void
insertion_argsort_pdouble_asc_1d(const double * data[], size_t len, uint32_t * restrict ord)
{
    insertion_argsort_r(data, len, ord, qsort_r_cmp_pdouble_asc_1d, NULL);
}

static __force_inline__ void
insertion_sort_pdouble_asc_only(const double * data[], size_t len, dimension_t col)
{
    switch (col) {
      case 0:
          insertion_sort_pdouble_asc_1d(data, len);
          return;
      case 2:
          insertion_sort_r(data, len, qsort_r_cmp_pdouble_asc_only_3d, NULL);
          return;
      case 3:
          insertion_sort_r(data, len, qsort_r_cmp_pdouble_asc_only_4d, NULL);
          return;
    }
    assert(false);
}

static __force_inline__ void
insertion_argsort_pdouble_asc_only(const double * data[], size_t len, dimension_t col, uint32_t * restrict ord)
{
    switch (col) {
      case 0:
          insertion_argsort_pdouble_asc_1d(data, len, ord);
          return;
      case 2:
          insertion_argsort_r(data, len, ord, qsort_r_cmp_pdouble_asc_only_3d, NULL);
          return;
      case 3:
          insertion_argsort_r(data, len, ord, qsort_r_cmp_pdouble_asc_only_4d, NULL);
          return;
    }
    assert(false);
}

static __force_inline__ void
insertion_sort_pdouble_asc_rev(const double * data[], size_t len, dimension_t col)
{
    switch (col) {
      case 0:
          insertion_sort_pdouble_asc_1d(data, len);
          return;
      case 1:
          insertion_sort_r(data, len, qsort_r_cmp_pdouble_asc_rev_2d, NULL);
          return;
      case 2:
          insertion_sort_r(data, len, qsort_r_cmp_pdouble_asc_rev_3d, NULL);
          return;
    }
    insertion_sort_r(data, len, qsort_r_cmp_pdouble_asc_rev, &col);
}

static __force_inline__ void
insertion_argsort_pdouble_asc_rev(const double * data[], size_t len, dimension_t col, uint32_t * restrict ord)
{
    switch (col) {
      case 0:
          insertion_argsort_pdouble_asc_1d(data, len, ord);
          return;
      case 1:
          insertion_argsort_r(data, len, ord, qsort_r_cmp_pdouble_asc_rev_2d, NULL);
          return;
      case 2:
          insertion_argsort_r(data, len, ord, qsort_r_cmp_pdouble_asc_rev_3d, NULL);
          return;
    }
    insertion_argsort_r(data, len, ord, qsort_r_cmp_pdouble_asc_rev, &col);
}

#endif // INSERTION_SORT_H
