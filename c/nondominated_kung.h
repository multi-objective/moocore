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
check_nondom(const double * points, const double ** rows,
             size_t size, dimension_t dim, boolvec * restrict nondom)
{
    for (size_t k = 0; k < size; k++) {
        if (!nondom[row_index_from_ptr(points, rows[k], dim)])
            return false;
    }
    return true;
}

static void
filter_dominated(const double * points, const double ** rows,
                 size_t new_size, dimension_t dim, dimension_t max_dim,
                 const boolvec * restrict nondom, size_t size)
{
    DEBUG2_PRINT("filter_dominated: dim=%d, size=%zu, new_size=%zu\n",
                 dim, size, new_size);
#if DEBUG >= 1
    size_t total = 0, j = 0;
    do {
        size_t pos = row_index_from_ptr(points, rows[j], max_dim);
        if (nondom[pos])
            total++;
        j++;
    } while (j < size);
    assert(total == new_size);
#endif

    size_t k = 0;
    // Find first dominated.
    while (nondom[row_index_from_ptr(points, rows[k], max_dim)])
        k++;
    size_t n = k;
    while (k < new_size) {
        do {
            n++;
        } while (!nondom[row_index_from_ptr(points, rows[n], max_dim)]); // Find next nondominated.
        rows[k] = rows[n];
        k++;
    }

    DEBUG2(print_rows(rows, new_size, dim));
}


/**
   Brute-force algorithm O(size^2 * dim), with a few improvements.

   Simplified version of find_nondominated_set_agree_bf().
*/
static inline size_t
maxima_brute_force(const double * points, const double ** restrict rows,
                   size_t size, dimension_t dim, dimension_t max_dim,
                   bool keep_weakly, boolvec * restrict nondom)
{
    DEBUG2_PRINT("maxima_brute_force: row=%zu, size=%zu\n",
            row_index_from_ptr(points, rows[0], max_dim), size);
    DEBUG2(print_rows(rows, size, dim));

    const unsigned nkw = !keep_weakly;
    size_t new_size = size;
    size_t min_k = 0;

    for (size_t j = 1; j < size; j++) {
        const double * restrict pj = rows[j];
        size_t k = min_k;
        assert(nondom[row_index_from_ptr(points, pj, max_dim)]);
        while (!nondom[row_index_from_ptr(points, rows[k], max_dim)])
            k++;
        min_k = k;
        ASSUME(k < j);
        ASSUME(nondom[row_index_from_ptr(points, rows[k], max_dim)]);

        for (; k < j; k++) {
            const double * restrict pk = rows[k];
            size_t pos_k = row_index_from_ptr(points, pk, max_dim);
            if (!nondom[pos_k]) continue;

            // Use unsigned instead of bool to allow auto-vectorization.
            unsigned k_leq_j = true, j_leq_k = true;
            for (dimension_t d = 0; d < dim; d++) {
                double cmp = pj[d] - pk[d];
                k_leq_j &= (cmp >= 0.0);
                j_leq_k &= (cmp <= 0.0);
            }
            // When j_leq_k & k_leq_j & nkw, we compare pj < pk to decide which
            // one should be removed (the one with the highest pointer value).
            unsigned dom_k = j_leq_k & ((!k_leq_j) | (nkw & (pj < pk)));
            unsigned dom_j = k_leq_j & ((!j_leq_k) | (nkw & (pj > pk)));
            /* As soon as j_leq_k and k_leq_j become false, neither k or j will
               be removed, so skip the rest.  */
            if (!(dom_k | dom_j)) continue;
            assert(dom_k ^ dom_j); // At least one but not both can be removed.

            size_t last_dom_pos = dom_j ? row_index_from_ptr(points, pj, max_dim) : pos_k;
            assert(nondom[last_dom_pos]);
            nondom[last_dom_pos] = false;
            new_size--;
            if (dom_j)
                break;
        }
    }
    assert(new_size > 0);
    return new_size;
}

static size_t
maxima_brute_force_filter_dom(
    const double * points, const double ** restrict rows,
    size_t size, dimension_t dim, dimension_t max_dim,
    bool keep_weakly, boolvec * restrict nondom)
{
    // Help GCC generate specialized code for true/false.
    size_t new_size = keep_weakly
        ? maxima_brute_force(points, rows, size, dim, max_dim, true, nondom)
        : maxima_brute_force(points, rows, size, dim, max_dim, false, nondom);
    if (new_size < size) {
        filter_dominated(points, rows, new_size, dim, max_dim, nondom, size);
    }
    assert(check_nondom(points, rows, new_size, max_dim, nondom));
    return new_size;
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
kung_merge_dim3(const double * points,
                const double ** restrict r, size_t r_size,
                const double ** restrict s, size_t s_size,
                dimension_t max_dim, boolvec * restrict nondom)
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
        DEBUG2_PRINT("i = %zu, j = %zu, s_size = %zu, v_row = %zu, ", i, k, s_size,
                row_index_from_ptr(points, v, max_dim));
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
            size_t pos_v = row_index_from_ptr(points, v, max_dim);
            DEBUG2(printf_point("dominated_by_tree_3d: v=[ ", v, 3, " ], "));
            DEBUG2_PRINT("pos_v=%zu\n", pos_v);
            assert(nondom[pos_v]);
            nondom[pos_v] = false;
            new_size--;
        }
        k++;
    } while (k < s_size);

    free(tnodes);
    return new_size;
}

static size_t
kung_merge_brute_force(const double * points,
                       const double ** restrict r, size_t r_size,
                       const double ** restrict s, size_t s_size,
                       dimension_t dim, dimension_t max_dim, boolvec * restrict nondom)
{
    DEBUG2_PRINT("kung_merge_brute_force: dim=%d, r=%zu, r_size=%zu, s=%zu, s_size=%zu\n",
                 dim, row_index_from_ptr(points, r[0], max_dim), r_size,
                 row_index_from_ptr(points, s[0], max_dim), s_size);

    size_t new_size = s_size;
    for (size_t j = 0; j < s_size; j++) {
        const double * restrict sj = s[j];
        for (size_t i = 0; i < r_size; i++) {
            const double * restrict ri = r[i];
            // Use unsigned instead of bool to allow auto-vectorization.
            unsigned r_leq_s = true;
            for (dimension_t d = 0; d < dim; d++) {
                double cmp = ri[d] - sj[d];
                r_leq_s &= (cmp <= 0.0);
            }
            if (unlikely(r_leq_s)) {
                size_t idx = row_index_from_ptr(points, sj, max_dim);
                assert(nondom[idx]);
                nondom[idx] = false;
                new_size--;
                break;
            }
        }
    }
    assert(check_nondom(points, r, r_size, max_dim, nondom));
    return new_size;
}

// Move to the next dimension and sort the rows again.
static void
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

static size_t
kung_merge_nobase(const double * points,
                  const double ** restrict r, size_t r_size,
                  const double ** restrict s, size_t s_size,
                  dimension_t dim, dimension_t max_dim, boolvec * restrict nondom);

/**
   Recurse one dimension before merging.
*/
static size_t
kung_merge_rec(const double * points,
               const double ** restrict r, size_t r_size,
               const double ** restrict s, size_t s_size,
               dimension_t dim, dimension_t max_dim, boolvec * restrict nondom)
{
    DEBUG2(printf_rows("kung_merge_rec: R", r, r_size, dim+1, "r_size"));
    DEBUG2(printf_rows("kung_merge_rec: S", s, s_size, dim+1, "s_size"));
    assert(check_nondom(points, r, r_size, max_dim, nondom));
    assert(check_nondom(points, s, s_size, max_dim, nondom));
    const double ** r1 = (const double **) malloc((r_size + s_size) * sizeof(*r));
    shift_to_next_dimension(r1, r, r_size);
    const double ** s1 = r1 + r_size;
    shift_to_next_dimension(s1, s, s_size);
    const double * const points_shifted = points + 1;
    DEBUG2(printf_rows("kung_merge_rec2: R", r1, r_size, dim, "r_size"));
    DEBUG2(printf_rows("kung_merge_rec2: S", s1, s_size, dim, "s_size"));
    assert(check_nondom(points_shifted, r1, r_size, max_dim, nondom));
    assert(check_nondom(points_shifted, s1, s_size, max_dim, nondom));
    /* FIXME: It is unclear whether kung_merge_dim3() is faster than
       kung_merge_brute_force() when r_size * s_size <= KUNG_MERGE_THRESHOLD.
       It is probably true that kung_merge_brute_force() is faster when
       r_size == 1 || s_size == 1 because we avoid sorting.
    */
    size_t new_size;
    if (r_size == 1 || s_size == 1 || r_size * s_size <= KUNG_MERGE_THRESHOLD) {
        // kung_merge_brute_force() does not need sorting.
        new_size = kung_merge_brute_force(points_shifted, r1, r_size, s1, s_size, dim, max_dim, nondom);
    } else {
        // We shifted to next dimension, so we need to sort.
        qsort_typesafe(r1, r_size, cmp_ppdouble_asc_x_nonzero_stable);
        qsort_typesafe(s1, s_size, cmp_ppdouble_asc_x_nonzero_stable);
        if (dim == 3) {
            new_size = kung_merge_dim3(points_shifted, r1, r_size, s1, s_size, max_dim, nondom);
            DEBUG2_PRINT("kung_merge_dim3: new_size=%zu\n", new_size);
        } else {
            // FIXME: kung_merge_nobase() will filter s1 but that is a waste
            // because we will destroy it without looking at it and filter s.
            new_size = kung_merge_nobase(points_shifted, r1, r_size, s1, s_size, dim, max_dim, nondom);
        }
    }
    assert(check_nondom(points_shifted, r1, r_size, max_dim, nondom));
    free(r1);
    return new_size;
}

static size_t
kung_merge_rec_filter_dom(const double * points,
                          const double ** restrict r, size_t r_size,
                          const double ** restrict s, size_t s_size,
                          dimension_t dim, dimension_t max_dim, boolvec * restrict nondom)
{
    size_t new_size = kung_merge_rec(points, r, r_size, s, s_size, dim, max_dim, nondom);
    if (new_size < s_size) {
        filter_dominated(points, s, new_size, dim+1, max_dim, nondom, s_size);
    }
    assert(check_nondom(points, s, new_size, max_dim, nondom));
    return new_size;
}

static size_t
kung_merge(const double * points,
           const double ** restrict r, size_t r_size,
           const double ** restrict s, size_t s_size,
           dimension_t dim, dimension_t max_dim, boolvec * restrict nondom);

// Find the elements of S that are not dominated by any elements of R.
static size_t
kung_merge_nobase(const double * points,
                  const double ** restrict r, size_t r_size,
                  const double ** restrict s, size_t s_size,
                  dimension_t dim, dimension_t max_dim, boolvec * restrict nondom)
{
    DEBUG2_PRINT("kung_merge_nobase: dim=%d, r=%zu, r_size=%zu\n", dim,
                 row_index_from_ptr(points, r[0], max_dim), r_size);
    DEBUG2(print_rows(r, r_size, dim));
    DEBUG2_PRINT("kung_merge_nobase: dim=%d, s=%zu, s_size=%zu\n", dim,
                 row_index_from_ptr(points, s[0], max_dim), s_size);
    DEBUG2(print_rows(s, s_size, dim));
    assert(r_size > 0);
    assert(s_size > 0);
    assert(check_nondom(points, r, r_size, max_dim, nondom));
    assert(check_nondom(points, s, s_size, max_dim, nondom));
    ASSUME(dim > 3);
    ASSUME(r_size > 1 && s_size > 1 && r_size * s_size > KUNG_MERGE_THRESHOLD);

    size_t s1_size = half_size_with_duplicates(s, s_size);
    size_t s2_size = s_size - s1_size;
    // Any element of s1 is smaller than any element of s2, but the elements
    // within each are not sorted.
    assert(s1_size > 0);
    // s2_size may be zero if all values in s are duplicated. In that case, the
    // pivot_value should just be one of the values of s1. Otherwise, it should
    // be the minimum of s2, which is already sorted
    const double pivot_value = (s2_size > 0) ? s[s1_size][0] : s[s1_size - 1][0];
    DEBUG2_PRINT("kung_merge_nobase: maxima_partition: s[%zu][0] = %g\n", s1_size,  pivot_value);
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
        s1_size = kung_merge_rec_filter_dom(points, r, r1_size, s, s1_size, dim - 1, max_dim, nondom);
    } else {
        // Solve sub-problem (R2, S2)
        DEBUG2(printf_rows("R2", r2, r2_size, dim, "r2_size"));
        DEBUG2(printf_rows("S2", s2, s2_size, dim, "s2_size"));
        if (r2_size > 0 && s2_size > 0) {
            DEBUG2_PRINT("Solve sub-problem  (R2, S2)\n");
            assert(check_nondom(points, r2, r2_size, max_dim, nondom));
            assert(check_nondom(points, s2, s2_size, max_dim, nondom));
            s2_size = kung_merge(points, r2, r2_size, s2, s2_size, dim, max_dim, nondom);
            assert(check_nondom(points, r2, r2_size, max_dim, nondom));
            assert(check_nondom(points, s2, s2_size, max_dim, nondom));
        }
        if (r1_size > 0) {
            DEBUG2_PRINT("Solve sub-problem  (R1, S1)\n");
            // Solve sub-problem  (R1, S1)
            assert(check_nondom(points, r, r1_size, max_dim, nondom));
            assert(check_nondom(points, s, s1_size, max_dim, nondom));
            s1_size = kung_merge(points, r, r1_size, s, s1_size, dim, max_dim, nondom);
            assert(check_nondom(points, r, r1_size, max_dim, nondom));
            assert(check_nondom(points, s, s1_size, max_dim, nondom));
            if (s2_size > 0) {
                DEBUG2_PRINT("Solve sub-problem  (R1, S2) in a lower dimension\n");
                s2_size = kung_merge_rec_filter_dom(points, r, r1_size, s2, s2_size, dim - 1, max_dim, nondom);
            }
        }
    }
    size_t new_size = compact_rows(s, s1_size, s2, s2_size, s_size);
    DEBUG2_PRINT("kung_merge: return S = %zu\n", new_size);
    DEBUG2(print_rows(s, new_size, dim));
    assert(check_nondom(points, s, new_size, max_dim, nondom));
    return new_size;
}

// Find the elements of S that are not dominated by any elements of R.
static size_t
kung_merge(const double * points,
           const double ** restrict r, size_t r_size,
           const double ** restrict s, size_t s_size,
           dimension_t dim, dimension_t max_dim, boolvec * restrict nondom)
{
    DEBUG2_PRINT("kung_merge: dim=%d, r=%zu, r_size=%zu\n", dim,
                 row_index_from_ptr(points, r[0], max_dim), r_size);
    DEBUG2(print_rows(r, s_size, dim));
    DEBUG2_PRINT("kung_merge: dim=%d, s=%zu, s_size=%zu\n", dim,
                 row_index_from_ptr(points, s[0], max_dim), s_size);
    DEBUG2(print_rows(s, s_size, dim));
    ASSUME(dim > 3);
    ASSUME(r_size > 0);
    ASSUME(s_size > 0);
    assert(check_nondom(points, r, r_size, max_dim, nondom));
    assert(check_nondom(points, s, s_size, max_dim, nondom));
    if (r_size == 1 || s_size == 1 || r_size * s_size <= KUNG_MERGE_THRESHOLD) {
        size_t new_size = kung_merge_brute_force(points, r, r_size, s, s_size, dim, max_dim, nondom);
        if (new_size < s_size) {
            filter_dominated(points, s, new_size, dim, max_dim, nondom, s_size);
        }
        assert(check_nondom(points, s, new_size, max_dim, nondom));
        return new_size;
    } else {
        // kung_merge_nobase() already filters, so we can return immediately.
        return kung_merge_nobase(points, r, r_size, s, s_size, dim, max_dim, nondom);
    }
}

static size_t
maxima_top(const double * points, const double ** rows, size_t size,
           dimension_t dim, dimension_t max_dim,
           bool keep_weakly, boolvec * restrict nondom);

/**
   This function is called if one dimension has all equal values, so we look at
   the next dimension.
*/
static size_t
maxima_rec_dim(const double * points, const double ** rows, size_t size,
               dimension_t dim, dimension_t max_dim,
               bool keep_weakly, boolvec * restrict nondom)
{
    size_t new_size;
    const double ** r = (const double **) malloc(size * sizeof(*r));
    shift_to_next_dimension(r, rows, size);
    const double * const points_shifted = points + 1;
    if (dim == 4) { // We can reach this base case if one dimension has all equal values.
        // find_nondominated_set_3d_helper() will sort using cmp_ppdouble_asc_rev_3d().
        new_size = keep_weakly // Help GCC generate specialized code for true/false.
            ? find_nondominated_set_3d_helper(points_shifted, r, size, max_dim, true, nondom)
            : find_nondominated_set_3d_helper(points_shifted, r, size, max_dim, false, nondom);
        DEBUG2_PRINT("maxima_dim3: row=%zu, size=%zu, new_size=%zu\n",
                     row_index_from_ptr(points_shifted, r[0], max_dim),
                     size, new_size);
    } else {
        ASSUME(size > KUNG_SMALL_THRESHOLD);
        // We need to sort here because we shifted to next dimension.
        qsort_typesafe(r, size, cmp_ppdouble_asc_x_nonzero_stable);
        new_size = maxima_top(points_shifted, r, size, dim - 1, max_dim, keep_weakly, nondom);
        DEBUG2_PRINT("maxima_rec: s_size == 0: row=%zu, size=%zu, new_size=%zu\n",
                     row_index_from_ptr(points_shifted, rows[0], max_dim),
                     size, new_size);
    }
    free(r);
    return new_size;
}

/**
   Algorithm 4.1
*/
static size_t
maxima_rec(const double * points, const double ** rows, size_t size,
           dimension_t dim, dimension_t max_dim,
           bool keep_weakly, boolvec * restrict nondom)
{
    if (size == 1)
        return size;

    ASSUME(dim > 3);
    if (size <= KUNG_SMALL_THRESHOLD)
        return maxima_brute_force_filter_dom(points, rows, size, dim, max_dim, keep_weakly, nondom);

    DEBUG2_PRINT("maxima_rec: dim=%d, row=%zu, size=%zu\n",
            dim, row_index_from_ptr(points, rows[0], max_dim), size);
    DEBUG2(printf_rows("maxima_rec: rows", rows, size, dim, "size"));
    size_t r_size = half_size_with_duplicates(rows, size);
    size_t s_size = size - r_size;
    DEBUG2(printf_rows("maxima_rec: R", rows, r_size, dim, "r_size"));

    if (s_size == 0) {
        size_t new_size = maxima_rec_dim(points, rows, size, dim, max_dim, keep_weakly, nondom);
        if (new_size < size) {
            filter_dominated(points, rows, new_size, dim, max_dim, nondom, size);
        }
        assert(check_nondom(points, rows, new_size, max_dim, nondom));
        return new_size;
    }

    const double ** s = rows + r_size;
    DEBUG2(printf_rows("maxima_rec: S", s, s_size, dim, "s_size"));

    r_size = maxima_rec(points, rows, r_size, dim, max_dim, keep_weakly, nondom);
    DEBUG2(printf_rows("maxima_rec2: R", rows, r_size, dim, "r_size"));

    s_size = maxima_rec(points, s, s_size, dim, max_dim, keep_weakly, nondom);
    DEBUG2(printf_rows("maxima_rec2: S", s, s_size, dim, "s_size"));

    s_size = kung_merge_rec_filter_dom(points, rows, r_size, s, s_size, dim - 1, max_dim, nondom);
    size_t new_size = compact_rows(rows, r_size, s, s_size, size);
    DEBUG2_PRINT("maxima_rec2: r U s = %zu\n", new_size);
    DEBUG2(print_rows(rows, new_size, dim));
    assert(check_nondom(points, rows, new_size, max_dim, nondom));
    return new_size;
}

/**
   This is the entry point of the recursion, so we do not need to call
   filter_dominated() before returning.  Apart from that, it is identical to
   maxima_rec().
 */
static size_t
maxima_top(const double * points, const double ** rows, size_t size,
                dimension_t dim, dimension_t max_dim,
                bool keep_weakly, boolvec * restrict nondom)
{
    ASSUME(size > KUNG_SMALL_THRESHOLD);
    ASSUME(dim > 3);
    DEBUG2_PRINT("maxima_top: dim=%d, row=%zu, size=%zu\n",
            dim, row_index_from_ptr(points, rows[0], max_dim), size);
    DEBUG2(printf_rows("maxima_top: rows", rows, size, dim, "size"));
    size_t r_size = half_size_with_duplicates(rows, size);
    size_t s_size = size - r_size;
    DEBUG2(printf_rows("maxima_top: R", rows, r_size, dim, "r_size"));

    if (s_size == 0) {
        // We do not need to filter dominated rows.
        return maxima_rec_dim(points, rows, size, dim, max_dim, keep_weakly, nondom);
    }

    const double ** s = rows + r_size;
    DEBUG2(printf_rows("maxima_top: S", s, s_size, dim, "s_size"));

    r_size = maxima_rec(points, rows, r_size, dim, max_dim, keep_weakly, nondom);
    DEBUG2(printf_rows("maxima_top2: R", rows, r_size, dim, "r_size"));

    s_size = maxima_rec(points, s, s_size, dim, max_dim, keep_weakly, nondom);
    DEBUG2(printf_rows("maxima_top2: S", s, s_size, dim, "s_size"));

    // We do not need to filter dominated rows.
    s_size = kung_merge_rec(points, rows, r_size, s, s_size, dim - 1, max_dim, nondom);
    size_t new_size = r_size + s_size;
    DEBUG2_PRINT("maxima_top2: r U s = %zu\n", new_size);
    DEBUG2(print_rows(rows, new_size, dim));
    return new_size;
}

static inline size_t
find_nondominated_set_agree_kung(const double * restrict points,
                                 size_t size, dimension_t dim,
                                 bool keep_weakly, boolvec * restrict nondom)
{
    ASSUME(size > KUNG_SMALL_THRESHOLD);
    ASSUME(dim > 3);

    const double ** rows = generate_row_pointers(points, size, dim);
    qsort_typesafe(rows, size, cmp_ppdouble_asc_x_nonzero_stable);
    size_t new_size = maxima_top(points, rows, size, dim, dim, keep_weakly, nondom);
    free(rows);
    return new_size;
}

#undef KUNG_MERGE_THRESHOLD

#endif // NONDOMINATED_KUNG_H
