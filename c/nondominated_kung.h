#ifndef NONDOMINATED_KUNG_H
#define NONDOMINATED_KUNG_H
/*****************************************************************************

 Implementation of the algorithm proposed by:

 H. T. Kung, F. Luccio, and F. P. Preparata.  On Finding the Maxima of a Set
 of Vectors. Journal of the ACM, 22(4):469–476, 1975.

 A different implementation is available from Duarte M. Dias, Alexandre
 D. Jesus, Luís Paquete, A software library for archiving nondominated
 points, GECCO 2021. https://github.com/TLDart/nondLib/blob/main/nondlib.hpp

 ---------------------------------------------------------------------

 Copyright (C) 2026
 Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

*****************************************************************************/
#ifndef NONDOMINATED_H
#error "Must be included from nondominated.h"
#endif

// If a sub-problem is not larger than this value, use n^2 algorithm instead of splitting it.
#ifndef KUNG_SMALL_THRESHOLD
#define KUNG_SMALL_THRESHOLD 16
#endif
// When merging, if |R| * |S| is not larger than this value, use n^2 algorithm.
#ifndef KUNG_MERGE_THRESHOLD
#define KUNG_MERGE_THRESHOLD 1024
#endif

static inline bool
check_nondom(const double ** rows, size_t size)
{
    for (size_t k = 0; k < size; k++) {
        if (rows[k] == NULL)
            return false;
    }
    return true;
}

static void
filter_dominated(const double ** restrict rows, size_t new_size, size_t size, dimension_t dim)
{
    DEBUG2_PRINT("filter_dominated: dim=%d, new_size=%zu, size=%zu\n", dim, new_size, size);

#if DEBUG >= 1
    size_t total = 0, j = 0;
    do {
        if (rows[j])
            total++;
        j++;
    } while (j < size);
    assert(total == new_size);
#endif

    size_t k = 0;
    // Find first dominated.
    while (rows[k] != NULL) k++;
    size_t n = k;
    while (k < new_size) {
        do {
            n++;
        } while (rows[n] == NULL); // Find next nondominated.
        rows[k] = rows[n];
        k++;
    }
    DEBUG2(print_rows(rows, new_size, dim));
}


/**
   Returns k, 0 <= k <= size, such that:

   * if k == 0, then v < u[j][0] for 0 <= j < size
   * else if k == size, then u[j][0] <= v for 0 <= j < size
   * otherwise, 0 < k < size, then u[j][0] <= v < u[m][0] for 0 <= j < k and k <= m < size.

   This corresponds to Eq. (5.3) in Kung et al. (1975).

   u is already sorted in increasing order.

   FIXME: This function could be made much faster using exponential (galloping) search,
   autovectorization-friendly block scan, and maybe Eytzinger layout search.
*/
static size_t
maxima_partition(const double * restrict * restrict u, size_t size, double v)
{
    if (v < u[0][0])
        return 0;
    if (v >= u[size - 1][0])
        return size;
    // Binary search.
    size_t low = 1, high = size - 1;
    do {
        size_t mid = low  + (high - low) / 2;
        if (v < u[mid][0])
            high = mid;
        else
            low = mid + 1;
    } while (low < high);

    DEBUG1(for (size_t j = 0; j < low; j++) assert(u[j][0] <= v));
    DEBUG1(for (size_t j = low; j < size; j++) assert(v < u[j][0]));
    return low;
}

/**
   Returns NULL if point is dominated by a different point in the tree.
*/
static avl_node_t *
dominated_by_tree_3d(const avl_tree_t * restrict tree,
                     const double * restrict point)
{
    avl_node_t * nodeaux;
    int res = avl_search_closest(tree, point, &nodeaux);
    if (res >= 0) { // nodeaux goes before point
        const double * restrict prev = nodeaux->item;
        DEBUG2(printf_point("res > 0: prev = [ ", prev, 3, " ], "));
        DEBUG2(printf_point("point = [ ", point, 3, " ]\n"));
        assert(prev[0] != INFINITY);
        assert(prev[0] <= point[0]);
        assert(prev[1] <= point[1]);
        return prev[2] <= point[2] ? NULL : nodeaux->next;
    } else if (nodeaux->prev) { // nodeaux goes after point, so move to the next one.
        const double * restrict prev = nodeaux->prev->item;
        DEBUG2(printf_point("res <= 0: prev = [ ", prev, 3, " ], "));
        DEBUG2(printf_point("point = [ ", point, 3, " ]\n"));
        assert(prev[0] != INFINITY);
        assert(prev[0] <= point[0]);
        assert(prev[1] <= point[1]);
        return prev[2] <= point[2] ? NULL : nodeaux;
    }
    assert(nodeaux != NULL);
    return nodeaux; // point is not dominated
}

/**
   ALGORITHM 5.1. This algorithm accepts two sets R and S of 3-dimensional
   vectors with r_size and s_size elements, respectively, and finds all the
   elements of S that are not dominated by any element of R.
*/
static size_t
kung_merge_dim3(const double ** restrict r, size_t r_size,
                const double ** restrict s, size_t s_size)
{
    // r and s should be already sorted.
    // Everything in s that is below the first element in r is nondominated.
    // FIXME: Use binary search to find k.
    const double r0 = r[0][0];
    if (r0 > s[s_size - 1][0])
        return s_size;
    size_t k = 0;
    while (r0 > s[k][0])
        k++;

    DEBUG1(for (size_t j = 0; j < k; j++) assert(s[j][0] < r0));
    DEBUG1(for (size_t j = k; j < s_size; j++) assert(r0 <= s[j][0]));

    avl_tree_t tree;
    avl_init_tree(&tree, qsort_cmp_pdouble_asc_y_asc_z);
    avl_node_t * tnodes = malloc((r_size + 1) * sizeof(*tnodes));
    avl_node_t * node = tnodes;
    node->item = r[0];
    avl_insert_top(&tree, node);

    const double sentinel[] = { INFINITY, INFINITY, -INFINITY};
    (++node)->item = sentinel;
    avl_insert_after(&tree, node - 1, node);

    size_t i = 1, new_size = s_size;
    do {
        const double * restrict v = s[k];
        DEBUG2_PRINT("i = %zu, j = %zu, s_size = %zu", i, k, s_size);
        DEBUG2(printf_point("v = [ ", v, 3, " ]\n"));

        while (i < r_size) { // Add to the tree all points in R that could dominate v.
            const double * restrict u = r[i];
            if (u[0] > v[0]) {
                break;
            }
            avl_node_t * nodeaux = dominated_by_tree_3d(&tree, u);
            if (nodeaux != NULL) { // u is NOT dominated by a point in the tree.
                const double * restrict point = nodeaux->item;
                assert(u[1] <= point[1]);
                // Delete everything in the tree that is dominated by u.
                while (u[2] <= point[2]) {
                    // printf("delete point: "); print_point(point); printf("\n");
                    assert(u[1] <= point[1]);
                    nodeaux = nodeaux->next;
                    point = nodeaux->item;
                    /* FIXME: A possible speed up is to delete without
                       rebalancing the tree because avl_insert_before() will
                       rebalance. */
                    avl_unlink_node(&tree, nodeaux->prev);
                }
                // printf("insert before point: "); print_point(point); printf("\n");
                (++node)->item = u;
                avl_insert_before(&tree, nodeaux, node);
            }
            i++;
        }
        if (dominated_by_tree_3d(&tree, v) == NULL) {
            DEBUG2(printf_point("dominated_by_tree_3d: v=[ ", v, 3, " ]\n"));
            s[k] = NULL; // dominated
            new_size--;
        }
        k++;
    } while (k < s_size);

    free(tnodes);
    return new_size;
}

static size_t
kung_merge_brute_force(const double ** restrict r, size_t r_size,
                       const double ** restrict s, size_t s_size,
                       dimension_t dim)
{
    DEBUG2_PRINT("kung_merge_brute_force: dim=%d, r_size=%zu, s_size=%zu\n",
                 dim, r_size, s_size);
    ASSUME(dim >= 3);

    size_t new_size = s_size;
    for (size_t j = 0; j < s_size; j++) {
        const double * restrict sj = s[j];
        for (size_t i = 0; i < r_size; i++) {
            const double * restrict ri = r[i];
            // Use unsigned instead of bool to allow auto-vectorization.
            unsigned r_leq_s = true;
            for (dimension_t d = 0; d < dim; d++)
                r_leq_s &= (ri[d] <= sj[d]);

            if (unlikely(r_leq_s)) {
                s[j] = NULL; // dominated
                new_size--;
                break;
            }
        }
    }
    assert(check_nondom(r, r_size));
    return new_size;
}

// Move to the next dimension and sort the rows again.
static inline void
shift_to_next_dimension(const double ** restrict dest,
                        const double ** restrict src, size_t size)
{
    assert(size > 0);
    for (size_t k = 0; k < size; k++)
        dest[k] = src[k] + 1;
}

/**
   Find a partition pivot (ideally close to size/2) such that

   rows[k][0] < rows[pivot][0]         for k=0, ..., pivot-1
   rows[pivot-1][0] < rows[pivot][0]
   rows[pivot-1][0] < rows[k][0]       for k=pivot, ..., size-1

   If pivot == size, then rows[k][0] <= rows[pivot-1][0] for k=0, ..., pivot-1,
   that is, all values are duplicated.

   This function assumes that rows is already sorted.
*/
static size_t
half_size_with_duplicates(const double * restrict * restrict rows, size_t size)
{
    size_t pivot = size / 2;
    assert(pivot > 0);
    assert(pivot < size);
    double value = rows[pivot][0];
    // Find first non-duplicate
    size_t left = pivot;
    while (left > 0  && rows[left-1][0] == value) left--;
    /* left is first duplicate to the left of pivot (left == 0 means all are
       duplicates).  */
    assert(rows[left][0] == value);
    /* Everything left <= k <= pivot is duplicated. Find first non-duplicate to
       the right of pivot.  */
    size_t right = pivot + 1;
    while (right < size && rows[right][0] == value) right++;
    // right is first non-duplicate (right == size means all are duplicates).
    /* If everything before pivot is duplicated, return the position of the
       first non-duplicate to the right.  */
    if (left == 0)
        return right;
    /* If everything after pivot is duplicated, return the position of the
       first duplicate.  */
    if (right == size)
        return left;

    return (pivot - left) <= (right - pivot) ? left : right;
}

#if DEBUG >= 1
static size_t
debug_half_size_with_duplicates(const double ** restrict rows, size_t size)
{
    size_t pivot = half_size_with_duplicates(rows, size);
    assert(pivot > 0);
    // Useful for debugging:
    /*
    for (size_t k= 0; k < pivot; k++)
        fprintf(stderr, "%g ", rows[k][0]);
    fprintf(stderr, "|");
    for (size_t k= pivot; k < size; k++)
        fprintf(stderr, " %g", rows[k][0]);
    fprintf(stderr, "\n");
    */

    if (pivot == size) {
        for (size_t k = 1; k < size; k++)
            assert(rows[0][0] == rows[k][0]);
    } else {
        for (size_t k = 0; k < pivot; k++)
            assert(rows[k][0] < rows[pivot][0]);
        assert(rows[pivot - 1][0] < rows[pivot][0]);
        for (size_t k = pivot; k < size; k++)
            assert(rows[pivot - 1][0] < rows[k][0]);
    }
    return pivot;
}
#define half_size_with_duplicates debug_half_size_with_duplicates
#endif

static inline size_t
compact_rows(const double ** restrict r, size_t r_size,
             const double ** restrict s, size_t s_size, size_t size)
{
    size_t new_size = r_size + s_size;
    ASSUME(new_size <= size);
    if (new_size < size) { // Compact nondominated rows
        for (size_t k = 0; k < s_size; k++)
            r[r_size + k] = s[k];
    }
    return new_size;
}

typedef struct shifted_row {
    const double * row;
    const double ** prow;
} shifted_row_t;

// Deterministic tie-break by pointer value.
DEFINE_QSORT_CMP(cmp_shifted_row_asc_x_nonzero_stable, shifted_row_t *)
{
    ASSUME(a != b);
    return cmp_pdouble_asc_x_nonzero_stable(a->row, b->row);
}

// Deterministic tie-break by pointer value.
DEFINE_QSORT_CMP(cmp_shifted_row_asc_rev_3d, shifted_row_t *)
{
    ASSUME(a != b);
    return cmp_pdouble_asc_rev(a->row, b->row, 3);
}

static shifted_row_t *
alloc_shifted_rows(const double ** restrict r, size_t size,
                   cmp_fun_t cmp_fun, const double ** restrict r_new)
{
    shifted_row_t * shifted_rows = malloc(size * sizeof(*shifted_rows));
    for (size_t k = 0; k < size; k++) {
        const double ** prow = r + k;
        shifted_rows[k].row = (*prow) + 1; // Shift to next dimension.
        shifted_rows[k].prow = prow;       // Remember the original row.
    }
    qsort(shifted_rows, size, sizeof(*shifted_rows), cmp_fun);
    for (size_t k = 0; k < size; k++) {
        r_new[k] = shifted_rows[k].row;
    }
    return shifted_rows;
}

/**
   This function marks elements of shifted_rows for removal (*prow = NULL).
   Only elements in r[] that are present in shifted_rows.row are not marked.
   We can assume that elements in r appear in the same order as in
   shifted_rows.
*/
static void
filter_shifted(shifted_row_t * restrict shifted_rows,
               const double ** restrict r, size_t new_size, size_t old_size)
{
    size_t i = 0, k = 0, n = 0;
    while (n < new_size) {
        while (r[i] == NULL) i++; // Find next nondominated
        while (r[i] != shifted_rows[k].row) { // Find it in shifted_rows.
            *(shifted_rows[k].prow) = NULL;
            k++;
        }
        i++;
        k++;
        n++;
    }
    // The rest are dominated.
    while (k < old_size) {
        *(shifted_rows[k].prow) = NULL;
        k++;
    }
}

static size_t kung_merge_nobase(const double ** restrict r, size_t r_size,
                                const double ** restrict s, size_t s_size,
                                dimension_t dim);

/**
   Recurse one dimension before merging.
*/
static size_t
kung_merge_rec_dim(const double ** restrict r, size_t r_size,
                   const double ** restrict s, size_t s_size,
                   dimension_t dim)
{
    DEBUG2(printf_rows("kung_merge_rec_dim: R", r, r_size, dim, "r_size"));
    DEBUG2(printf_rows("kung_merge_rec_dim: S", s, s_size, dim, "s_size"));
    assert(check_nondom(r, r_size));
    assert(check_nondom(s, s_size));
    /* FIXME: It is unclear whether kung_merge_dim3() is faster than
       kung_merge_brute_force() when r_size * s_size <= KUNG_MERGE_THRESHOLD.
       It is probably true that kung_merge_brute_force() is faster when
       r_size == 1 || s_size == 1 because we avoid sorting.
    */
    size_t new_size;
    if (r_size * s_size <= KUNG_MERGE_THRESHOLD || r_size == 1 || s_size == 1) {
        // kung_merge_brute_force() does not need sorting, so copying r and s is not needed.
        for (size_t k = 0; k < r_size; k++)
            r[k]++;
        for (size_t k = 0; k < s_size; k++)
            s[k]++;
        new_size = kung_merge_brute_force(r, r_size, s, s_size, dim - 1);
        for (size_t k = 0; k < r_size; k++)
            r[k]--;
        // We filter first so we remove all NULL pointers.
        if (new_size < s_size)
            filter_dominated(s, new_size, s_size, dim);
        for (size_t k = 0; k < new_size; k++)
            s[k]--;
    } else {
        /* We will create new row pointers that point to the next dimension,
           however, for s, we also want to remember the original position of
           each row pointer to be able to filter dominated, thus shifted_rows
           stores the new row pointer and a pointer to the previous one, so we
           can modify it directly.  */
        const double ** r1 = (const double **) malloc((r_size + s_size) * sizeof(*r));
        shift_to_next_dimension(r1, r, r_size);
        // We shifted to next dimension, so we need to sort.
        qsort_typesafe(r1, r_size, cmp_ppdouble_asc_x_nonzero_stable);
        const double ** s1 = r1 + r_size;
        shifted_row_t * shifted_rows = alloc_shifted_rows(
            s, s_size, qsort_cmp_shifted_row_asc_x_nonzero_stable, s1);
        DEBUG2(printf_rows("kung_merge_rec_dim2: R", r1, r_size, dim - 1, "r_size"));
        DEBUG2(printf_rows("kung_merge_rec_dim2: S", s1, s_size, dim - 1, "s_size"));
        assert(check_nondom(r1, r_size));
        assert(check_nondom(s1, s_size));
        if (dim == 4) {
            new_size = kung_merge_dim3(r1, r_size, s1, s_size);
            DEBUG2_PRINT("kung_merge_dim3: new_size=%zu\n", new_size);
        } else {
            assert(dim > 4);
            new_size = kung_merge_nobase(r1, r_size, s1, s_size, dim - 1);
        }
        assert(check_nondom(r1, r_size));
        if (new_size < s_size) {
            filter_shifted(shifted_rows, s1, new_size, s_size);
            filter_dominated(s, new_size, s_size, dim);
        }
        free(shifted_rows);
        free(r1);
    }
    assert(check_nondom(s, new_size));
    return new_size;
}

static size_t
kung_merge(const double ** restrict r, size_t r_size,
           const double ** restrict s, size_t s_size,
           dimension_t dim);

// Find the elements of S that are not dominated by any elements of R.
static size_t
kung_merge_nobase(const double ** restrict r, size_t r_size,
                  const double ** restrict s, size_t s_size,
                  dimension_t dim)
{
    DEBUG2_PRINT("kung_merge_nobase: dim=%d, r_size=%zu\n", dim, r_size);
    DEBUG2(print_rows(r, r_size, dim));
    DEBUG2_PRINT("kung_merge_nobase: dim=%d, s_size=%zu\n", dim, s_size);
    DEBUG2(print_rows(s, s_size, dim));
    assert(r_size > 0);
    assert(s_size > 0);
    assert(check_nondom(r, r_size));
    assert(check_nondom(s, s_size));
    ASSUME(dim > 3);
    ASSUME(r_size > 1 && s_size > 1 && r_size * s_size > KUNG_MERGE_THRESHOLD);

    size_t s1_size = half_size_with_duplicates(s, s_size);
    size_t s2_size = s_size - s1_size;
    // Any element of s1 is smaller than any element of s2, but the elements
    // within each are not sorted.
    assert(s1_size > 0);
    // s2_size may be zero if all values in s are duplicated. In that case, the
    // pivot_value should just be one of the values of s1. Otherwise, it should
    // be the minimum of s2, which is already sorted, so s[s1_size][0].
    const double pivot_value = s[(s2_size == 0) ? s1_size - 1 : s1_size][0];
    DEBUG2_PRINT("kung_merge_nobase: s2_size = %zu, maxima_partition: s[%zu][0] = %g\n",
                 s2_size, (s2_size == 0) ? s1_size - 1 : s1_size,  pivot_value);
    const size_t r1_size = maxima_partition(r, r_size, pivot_value);
    const size_t r2_size = r_size - r1_size;
    const double ** r2 = r + r1_size;
    const double ** s2 = s + s1_size;
    // Solve sub-problem (R1,S1)
    DEBUG2(printf_rows("R1", r, r1_size, dim, "r1_size"));
    DEBUG2(printf_rows("S1", s, s1_size, dim, "s1_size"));
    if (r2_size == 0 && s2_size == 0) {
        ASSUME(r1_size > 0);
        ASSUME(s1_size > 0);
        DEBUG2_PRINT("Solve sub-problem  (R1, S1) in a lower dimension\n");
        return kung_merge_rec_dim(r, r1_size, s, s1_size, dim);
    }

    // Solve sub-problem (R2, S2)
    DEBUG2(printf_rows("R2", r2, r2_size, dim, "r2_size"));
    DEBUG2(printf_rows("S2", s2, s2_size, dim, "s2_size"));
    if (r2_size > 0 && s2_size > 0) {
        DEBUG2_PRINT("Solve sub-problem  (R2, S2)\n");
        s2_size = kung_merge(r2, r2_size, s2, s2_size, dim);
        assert(check_nondom(r2, r2_size));
        assert(check_nondom(s2, s2_size));
    }
    if (r1_size > 0) {
        DEBUG2_PRINT("Solve sub-problem  (R1, S1)\n");
        // Solve sub-problem  (R1, S1)
        s1_size = kung_merge(r, r1_size, s, s1_size, dim);
        assert(check_nondom(r, r1_size));
        assert(check_nondom(s, s1_size));
        if (s2_size > 0) {
            DEBUG2_PRINT("Solve sub-problem  (R1, S2) in a lower dimension\n");
            s2_size = kung_merge_rec_dim(r, r1_size, s2, s2_size, dim);
        }
    }
    size_t new_size = compact_rows(s, s1_size, s2, s2_size, s_size);
    DEBUG2_PRINT("kung_merge: return S = %zu\n", new_size);
    DEBUG2(print_rows(s, new_size, dim));
    assert(check_nondom(s, new_size));
    return new_size;
}

// Find the elements of S that are not dominated by any elements of R.
static size_t
kung_merge(const double ** restrict r, size_t r_size,
           const double ** restrict s, size_t s_size,
           dimension_t dim)
{
    DEBUG2_PRINT("kung_merge: dim=%d, r_size=%zu\n", dim, r_size);
    DEBUG2(print_rows(r, s_size, dim));
    DEBUG2_PRINT("kung_merge: dim=%d, s_size=%zu\n", dim, s_size);
    DEBUG2(print_rows(s, s_size, dim));
    ASSUME(dim > 3);
    ASSUME(r_size > 0);
    ASSUME(s_size > 0);
    assert(check_nondom(r, r_size));
    assert(check_nondom(s, s_size));
    if (r_size * s_size <= KUNG_MERGE_THRESHOLD || r_size == 1 || s_size == 1) {
        size_t new_size = kung_merge_brute_force(r, r_size, s, s_size, dim);
        if (new_size < s_size) {
            filter_dominated(s, new_size, s_size, dim);
        }
        assert(check_nondom(s, new_size));
        return new_size;
    } else {
        // kung_merge_nobase() already filters, so we can return immediately.
        return kung_merge_nobase(r, r_size, s, s_size, dim);
    }
}

static size_t maxima_rec(const double ** rows, size_t size, dimension_t dim,
                         bool keep_weakly);

/**
   This function is called if one dimension has all equal values, so we look at
   the next dimension.
*/
static size_t
maxima_rec_dim(const double ** rows, size_t size, dimension_t dim,
               bool keep_weakly)
{
    size_t new_size;
    const double ** r_new = (const double **) malloc(size * sizeof(*rows));
    /* We will create new row pointers that point to the next dimension,
       however, we also want to remember the original position of each row
       pointer to be able to filter dominated, thus shifted_rows stores the new
       row pointer and a pointer to the previous one, so we can modify it
       directly.  */
    shifted_row_t * shifted_rows;

    if (dim == 4) { // We can reach this base case if one dimension has all equal values.
        // Sort in ascending lexicographic order from the last dimension.
        shifted_rows = alloc_shifted_rows(
            rows, size, qsort_cmp_shifted_row_asc_rev_3d, r_new);
         new_size = keep_weakly // Help GCC generate specialized code for true/false.
             ? find_nondominated_3d_impl_sorted(r_new, size,  true, /* find_one_dominated=*/false)
             : find_nondominated_3d_impl_sorted(r_new, size, false, /* find_one_dominated=*/false);
         DEBUG2_PRINT("maxima_dim3: size=%zu, new_size=%zu\n", size, new_size);
    } else {
        ASSUME(size > KUNG_SMALL_THRESHOLD);
        shifted_rows = alloc_shifted_rows(
            rows, size, qsort_cmp_shifted_row_asc_x_nonzero_stable, r_new);
        new_size = maxima_rec(r_new, size, dim - 1, keep_weakly);
        DEBUG2_PRINT("maxima_rec: s_size == 0: size=%zu, new_size=%zu\n", size, new_size);
    }
    if (new_size < size) {
        filter_shifted(shifted_rows, r_new, new_size, size);
        filter_dominated(rows, new_size, size, dim+1);
    }
    free(shifted_rows);
    free(r_new);
    assert(check_nondom(rows, new_size));
    return new_size;
}

/**
   Brute-force algorithm O(size^2 * dim), with a few improvements.

   Simplified version of find_nondominated_bf_impl().
*/
typedef enum {
    VEC_A_LT_B = -1,
    VEC_INCOMPARABLE = 0,
    VEC_A_GT_B = 1,
} dominance_cmp_t;

static inline dominance_cmp_t
vec_cmp_dominance(const double * restrict a, const double * restrict b,
                  dimension_t dim, bool keep_weakly)
{
    // Use unsigned instead of bool to allow auto-vectorization.
    unsigned lt = 0; // any a[i] < b[i]
    unsigned gt = 0; // any a[i] > b[i]
    for (dimension_t d = 0; d < 4; d++) {
        lt |= (a[d] < b[d]);
        gt |= (a[d] > b[d]);
    }
    if (lt & gt)
        return VEC_INCOMPARABLE;

    for (dimension_t d = 4; d < dim; d++) {
        lt |= (a[d] < b[d]);
        gt |= (a[d] > b[d]);
    }
    if (lt & gt)
        return VEC_INCOMPARABLE;

    if (lt) return VEC_A_LT_B;
    if (gt) return VEC_A_GT_B;
    // They are equal.
    if (keep_weakly)
        return VEC_INCOMPARABLE;
    // Remove the one with the highest pointer value.
    return (a < b) ? VEC_A_LT_B : VEC_A_GT_B;
}

static __force_inline__ size_t
maxima_brute_force(const double ** restrict rows, size_t size, dimension_t dim,
                   bool keep_weakly)
{
    DEBUG2_PRINT("maxima_brute_force: size=%zu\n", size);
    DEBUG2(print_rows(rows, size, dim));

    size_t new_size = size;
    size_t min_k = 0;
    for (size_t j = 1; j < size; j++) {
        size_t k = min_k;
        while (rows[k] == NULL)
            k++;
        min_k = k;
        const double * restrict pj = rows[j];
        ASSUME(pj != NULL);
        ASSUME(rows[k] != NULL);
        ASSUME(k < j);
        for (; k < j; k++) {
            const double * restrict pk = rows[k];
            if (pk == NULL) continue;

            dominance_cmp_t res = vec_cmp_dominance(pk, pj, dim, keep_weakly);
            if (likely(res == VEC_INCOMPARABLE)) {
                continue;
            } else if (res == VEC_A_LT_B) {
                // pk[i] <= pj[i] for all i, and pk[i] < pj[i] for at least one i.
                new_size--;
                assert(rows[j]);
                rows[j] = NULL;
                break;
            } else {
                assert(res == VEC_A_GT_B);
                // pj[i] <= pk[i] for all i, and pj[i] < pk[i] for at least one i.
                new_size--;
                assert(rows[k]);
                rows[k] = NULL;
            }
        }
    }
    assert(new_size > 0);
    return new_size;
}

static __force_inline__ size_t
maxima_brute_force_filter_dom(const double ** restrict rows, size_t size,
                              dimension_t dim, bool keep_weakly)
{
    // Help GCC generate specialized code for true/false.
    size_t new_size = keep_weakly
        ? maxima_brute_force(rows, size, dim, true)
        : maxima_brute_force(rows, size, dim, false);
    if (new_size < size) {
        filter_dominated(rows, new_size, size, dim);
    }
    assert(check_nondom(rows, new_size));
    return new_size;
}

/**
   Algorithm 4.1
*/
static size_t
maxima_rec(const double ** restrict rows, size_t size, dimension_t dim,
           bool keep_weakly)
{
    ASSUME(dim > 3);
    DEBUG2_PRINT("maxima_rec: dim=%d, size=%zu\n", dim, size);
    DEBUG2(printf_rows("maxima_rec: rows", rows, size, dim, "size"));
    size_t r_size = half_size_with_duplicates(rows, size);
    size_t s_size = size - r_size;
    DEBUG2(printf_rows("maxima_rec: R", rows, r_size, dim, "r_size"));

    if (s_size == 0)
        return maxima_rec_dim(rows, size, dim, keep_weakly);

    const double ** s = rows + r_size;
    DEBUG2(printf_rows("maxima_rec: S", s, s_size, dim, "s_size"));

    if (r_size > 1)
        r_size = (r_size <= KUNG_SMALL_THRESHOLD)
            ? maxima_brute_force_filter_dom(rows, r_size, dim, keep_weakly)
            : maxima_rec(rows, r_size, dim, keep_weakly);
    DEBUG2(printf_rows("maxima_rec2: R", rows, r_size, dim, "r_size"));

    if (s_size > 1)
        s_size = (s_size <= KUNG_SMALL_THRESHOLD)
            ? maxima_brute_force_filter_dom(s, s_size, dim, keep_weakly)
            : maxima_rec(s, s_size, dim, keep_weakly);
    DEBUG2(printf_rows("maxima_rec2: S", s, s_size, dim, "s_size"));

    s_size = kung_merge_rec_dim(rows, r_size, s, s_size, dim);
    size_t new_size = compact_rows(rows, r_size, s, s_size, size);
    DEBUG2_PRINT("maxima_rec2: r U s = %zu\n", new_size);
    DEBUG2(print_rows(rows, new_size, dim));
    assert(check_nondom(rows, new_size));
    return new_size;
}

static inline size_t
find_nondominated_set_kung(const double * restrict points,
                           size_t size, dimension_t dim,
                           bool keep_weakly, boolvec * restrict nondom)
{
    ASSUME(size > KUNG_SMALL_THRESHOLD);
    ASSUME(dim > 3);

    const double ** rows = generate_row_pointers(points, size, dim);
    qsort_typesafe(rows, size, cmp_ppdouble_asc_x_nonzero_stable);
    size_t new_size = maxima_rec(rows, size, dim, keep_weakly);

    if (new_size < size) {
        memset(nondom, 0, size * sizeof(*nondom));
        // maxima_rec() has already filtered dominated rows.
        for (size_t n = 0; n < new_size; n++) {
            assert(rows[n] != NULL);
            nondom[row_index_from_ptr(points, rows[n], dim)] = true;
        }
    }
    free(rows);
    return new_size;
}

#undef KUNG_MERGE_THRESHOLD

#endif // NONDOMINATED_KUNG_H
