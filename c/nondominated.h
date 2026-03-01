#ifndef NONDOMINATED_H
#define NONDOMINATED_H

#include "config.h"

#include <string.h> // memcpy
#include <math.h> // INFINITY
#include "sort.h"

typedef const double avl_item_t;
typedef struct avl_node_t {
    struct avl_node_t *next;
    struct avl_node_t *prev;
    struct avl_node_t *parent;
    struct avl_node_t *left;
    struct avl_node_t *right;
    avl_item_t *item;
    unsigned char depth;
} avl_node_t;

#include "avl_tiny.h"

enum objs_agree_t { AGREE_MINIMISE = -1, AGREE_NONE = 0, AGREE_MAXIMISE = 1 };

static inline void
printf_point(const char * prefix, const double * p, dimension_t dim, const char * suffix)
{
    fprintf(stderr, "%s%g", prefix, p[0]);
    for (dimension_t d = 1; d < dim; d++)
        fprintf(stderr, " %g", p[d]);
    fprintf(stderr, "%s", suffix);
}

static inline void
print_rows(const double ** rows, size_t size, dimension_t dim)
{
    for (size_t k = 0; k < size; k++)
        printf_point("", rows[k], dim, "\n");
}

static inline void
printf_rows(const char * prefix,
            const double ** rows, size_t size, dimension_t dim,
            const char * size_lab)
{
    if (size > 0) {
        fprintf(stderr, "%s", prefix);
        printf_point(" = [ ", rows[0], dim, " ], ");
        // We cannot use %zu for size_t because of MingW compiler used by CRAN.
        fprintf(stderr, "%s = %lu\n", size_lab, (unsigned long) size);
        print_rows(rows, size, dim);
    } else {
        fprintf(stderr, "%s = [ ], %s = 0\n", prefix, size_lab);
    }
}

static inline enum objs_agree_t
check_all_minimize_maximize(const int * restrict minmax, dimension_t dim)
{
    bool all_minimize = (minmax[0] < 0);
    bool all_maximize = (minmax[0] > 0);
    assert(!all_maximize || !all_minimize);
    if (all_minimize) {
        for (dimension_t d = 1; d < dim; d++) {
            if (minmax[d] >= 0)
                return AGREE_NONE;
        }
        return AGREE_MINIMISE;
    }
    if (all_maximize) {
        for (dimension_t d = 1; d < dim; d++) {
            if (minmax[d] <= 0)
                return AGREE_NONE;
        }
        return AGREE_MAXIMISE;
    }
    return AGREE_NONE;
}

/** Convert from bool vector to minmax vector.

    minmax needs to be an int (int64 in 64 bits) so that GCC can vectorize
    loops that are conditional on minmax[d].  Using 'signed char' does not work
    in some targets, e.g, -march=tigerlake.
*/
static inline int *
minmax_from_bool(const bool * restrict maximise, dimension_t nobj)
{
    ASSUME(nobj > 0 && nobj < 128);
    int * minmax = malloc(nobj * sizeof(*minmax));
    for (dimension_t k = 0; k < nobj; k++) {
        minmax[k] = maximise[k] ? AGREE_MAXIMISE : AGREE_MINIMISE;
    }
    return minmax;
}

static inline bool *
new_bool_maximise(dimension_t nobj, bool maximise_all)
{
    ASSUME(nobj > 0 && nobj < 128);
    bool * maximise = malloc(nobj * sizeof(*maximise));
    for (dimension_t k = 0; k < nobj; k++)
        maximise[k] = maximise_all;
    return maximise;
}

static inline const int *
default_minmax(dimension_t nobj, int default_value)
{
    ASSUME(nobj > 0 && nobj < 128);
    ASSUME(default_value == AGREE_MINIMISE || default_value == AGREE_MAXIMISE);
    int * minmax = malloc(nobj * sizeof(*minmax));
    for (dimension_t i = 0; i < nobj; i++)
        minmax[i] = default_value;
    return minmax;
}

static inline const int *
minmax_minimise(dimension_t nobj)
{
    return default_minmax(nobj, AGREE_MINIMISE);
}

static inline const int *
minmax_maximise(dimension_t nobj)
{
    return default_minmax(nobj, AGREE_MAXIMISE);
}

static inline bool
minmax_alloc(const int * restrict * minmax, bool maximise_all_flag,
             dimension_t d)
{
    if (*minmax == NULL) {
        *minmax = maximise_all_flag ? minmax_maximise(d) : minmax_minimise(d);
        return true;
    }
    return false;
}


// FIXME: It would be more efficient to default to false (using calloc) and
// change to true.
static inline bool *
nondom_init(size_t size)
{
    bool * nondom = malloc(size * sizeof(*nondom));
    for (size_t n = 0; n < size; n++)
        nondom[n] = true;
    return nondom;
}

/**
   Maybe create a copy of points (with lower dimensionality) depending on the
   value of minmax.
*/
static inline const double *
force_agree_minimize(const double * restrict points, size_t size,
                     dimension_t * restrict dim_p,
                     _attr_maybe_unused enum objs_agree_t agree,
                     const int * restrict minmax)
{
    assert(agree != AGREE_MINIMISE);
    bool copy = false;
    dimension_t true_dim = *dim_p;
    for (dimension_t d = 0; d < true_dim; d++) {
        if (minmax[d] >= 0) {
            copy = true;
            break;
        }
    }
    if (!copy)
        return points;

    dimension_t prefix = 0;
    while (prefix < true_dim && (minmax[prefix] != 0))
        prefix++;

    dimension_t new_dim = prefix;
    for (dimension_t d = prefix + 1; d < true_dim; d++)
        new_dim = (dimension_t) (new_dim + (minmax[d] != 0));

    assert(new_dim > 1);
    double * pnew = malloc(new_dim * size * sizeof(*pnew));

    if (new_dim < true_dim) { // Remove columns
        ASSUME(true_dim < MOOCORE_DIMENSION_MAX);
        uint8_t idx[MOOCORE_DIMENSION_MAX];
        for (dimension_t j = prefix, t = 0; j < true_dim; j++)
            if (minmax[j] != 0)
                idx[t++] = (uint8_t)j;

        size_t tail_active = new_dim - prefix;
        for (size_t i = 0; i < size; i++) {
            const double * restrict src = points + i * true_dim;
            double * restrict dst = pnew + i * new_dim;
            memcpy(dst, src, prefix * sizeof(*dst));
            for (dimension_t t = 0; t < tail_active; t++) {
                uint8_t j = idx[t];
                dst[prefix+t] = src[j];
            }
        }
        for (dimension_t t = 0, d = 0; d < true_dim; d++) {
            if (minmax[d] == 0) continue;
            if (minmax[d] > 0) {
                for (size_t k = 0; k < size; k++)
                    pnew[k * new_dim + t] = -pnew[k * new_dim + t];
            }
            t++;
        }
        *dim_p = new_dim;
    } else {
        memcpy(pnew, points, true_dim * size * sizeof(*pnew));
        for (dimension_t d = 0; d < true_dim; d++) {
            assert(minmax[d] != 0);
            if (minmax[d] > 0)
                for (size_t k = 0; k < size; k++)
                    pnew[k * true_dim + d] = -pnew[k * true_dim + d];
        }
    }
    return pnew;
}

/**
   Get original row index from row pointer. The row pointer points to a vector
   of dimension max_dim and may have been shifted to point to dimension dim.
*/
_attr_const_func static inline size_t
row_index_from_shifted_ptr(const double * points, const double * rowptr,
                           dimension_t dim, dimension_t max_dim)
{
    ASSUME(rowptr >= points);
    size_t diff = (size_t)(rowptr - points);
    ASSUME(max_dim >= dim);
    size_t shift = max_dim - dim;
    ASSUME(diff >= shift);
    return (diff - shift) / max_dim;
}


/**
   Same as row_index_from_shifted_ptr() but assumes that rowptr is not shifted.
*/
_attr_const_func static inline size_t
row_index_from_ptr(const double * points, const double * rowptr, dimension_t dim)
{
    return row_index_from_shifted_ptr(points, rowptr, dim, dim);
}


static inline const double **
generate_row_pointers_asc_rev_2d(const double * restrict points, size_t size)
{
    const double ** p = generate_row_pointers(points, size, 2);
    // Sort in ascending lexicographic order from the last dimension.
    qsort_typesafe(p, size, cmp_ppdouble_asc_rev_2d);
    return p;
}

static inline const double **
generate_row_pointers_asc_rev_3d(const double * restrict points, size_t size)
{
    const double ** p = generate_row_pointers(points, size, 3);
    // Sort in ascending lexicographic order from the last dimension.
    qsort_typesafe(p, size, cmp_ppdouble_asc_rev_3d);
    return p;
}

static inline size_t
find_nondominated_2d_helper_(const double * restrict points, size_t size,
                             const bool keep_weakly, bool * restrict nondom)
{
    ASSUME(size > 1);
    const double ** p = generate_row_pointers_asc_rev_2d(points, size);
    // returning size means "no dominated solution found".
    size_t n_nondom = size, j = 1;
    // When compiling with -O3, GCC is able to create two versions of this loop
    // and move keep_weakly out.
    const double * restrict pk = p[0];
    do {
        const double * restrict pj = p[j];
        if (pk[0] > pj[0]) {
            pk = pj;
        } else {
            const bool k_eq_j = (pk[0] == pj[0]) & (pk[1] == pj[1]);
            if (!keep_weakly || likely(!k_eq_j)) {
                if (k_eq_j && pj < pk) // Only the first duplicated point is kept.
                    SWAP(pj, pk);

                size_t pos_first_dom = row_index_from_ptr(points, pj, 2);
                if (unlikely(nondom == NULL)) {
                    // In this context, it means "position of the first dominated solution found".
                    n_nondom = pos_first_dom;
                    goto early_end;
                }
                nondom[pos_first_dom] = false;
                n_nondom--;
            }
        }
        j++;
    } while (j < size);

early_end:
    free(p);
    return n_nondom;
}

/**
   Stop as soon as one dominated point is found and return its position (or size
   if no dominated point found).
*/
static inline size_t
find_dominated_2d_(const double * restrict points, size_t size, const bool keep_weakly)
{
    return keep_weakly
        ? find_nondominated_2d_helper_(points, size, true, /*nondom=*/NULL)
        : find_nondominated_2d_helper_(points, size, false, /*nondom=*/NULL);
}


/**
   Store which points are nondominated in nondom and return the number of
   nondominated points.

   2D dimension-sweep algorithm by H. T. Kung, F. Luccio, and F. P. Preparata.
   On Finding the Maxima of a Set of Vectors. Journal of the ACM,
   22(4):469–476, 1975.

   Duplicated points may be removed in any order due to qsort not being stable.
*/
static inline size_t
find_nondominated_set_2d_(const double * restrict points, size_t size,
                          const bool keep_weakly, bool * restrict nondom)
{
    ASSUME(nondom != NULL);
    return keep_weakly
        ? find_nondominated_2d_helper_(points, size, true, nondom)
        : find_nondominated_2d_helper_(points, size, false, nondom);
}

/**
   3D dimension-sweep algorithm by H. T. Kung, F. Luccio, and F. P. Preparata.
   On Finding the Maxima of a Set of Vectors. Journal of the ACM,
   22(4):469–476, 1975.

   A different implementation is available from Duarte M. Dias, Alexandre
   D. Jesus, Luís Paquete, A software library for archiving nondominated
   points, GECCO 2021. https://github.com/TLDart/nondLib/blob/main/nondlib.hpp
*/
static inline size_t
find_nondominated_set_3d_helper(const double * restrict points, size_t size,
                                const bool keep_weakly, bool * restrict nondom)
{
    ASSUME(size >= 2);
    const double ** rows = generate_row_pointers_asc_rev_3d(points, size);

    avl_tree_t tree;
    avl_init_tree(&tree, qsort_cmp_pdouble_asc_x_nonzero);
    avl_node_t * tnodes = malloc((size+1) * sizeof(*tnodes));
    avl_node_t * node = tnodes;
    node->item = rows[0];
    avl_insert_top(&tree, node);

    const double sentinel[] = { INFINITY, -INFINITY };
    (++node)->item = sentinel;
    avl_insert_after(&tree, node - 1, node);

    // In this context, size means "no dominated solution found".
    size_t pos_last_dom = size, n_nondom = size, j = 1;
    const double * restrict pk = rows[0];
    do {
        const double * restrict pj = rows[j];
        DEBUG3(printf_point("pj: ", pj, 3, "\n"));
        bool dominated;
        if (pk[0] > pj[0] || pk[1] > pj[1]) {
            avl_node_t * nodeaux;
            int res = avl_search_closest(&tree, pj, &nodeaux);
            assert(res != 0);
            if (res > 0) { // nodeaux goes before pj
                const double * restrict prev = nodeaux->item;
                assert(prev[0] != sentinel[0]);
                assert(prev[0] <= pj[0]);
                dominated = prev[1] <= pj[1];
                DEBUG3(printf_point("res > 0: prev: ", prev, 3, "\n"));
                nodeaux = nodeaux->next;
            } else if (nodeaux->prev) { // nodeaux goes after pj, so move to the next one.
                const double * restrict prev = nodeaux->prev->item;
                assert(prev[0] != sentinel[0]);
                assert(prev[0] <= pj[0]);
                dominated = prev[1] <= pj[1];
                DEBUG3(printf_point("res < 0: prev: ", prev, 3, "\n"));
            } else {
                dominated = false;
            }

            if (!dominated) { // pj is NOT dominated by a point in the tree.
                const double * restrict point = nodeaux->item;
                assert(pj[0] <= point[0]);
                // Delete everything in the tree that is dominated by pj.
                while (pj[1] <= point[1]) {
                    DEBUG3(printf_point("delete point: ", point, 3, "\n"));
                    assert(pj[0] <= point[0]);
                    nodeaux = nodeaux->next;
                    point = nodeaux->item;
                    /* FIXME: A possible speed up is to delete without
                       rebalancing the tree because avl_insert_before() will
                       rebalance. */
                    avl_unlink_node(&tree, nodeaux->prev);
                }
                DEBUG3((point == sentinel)
                       ? printf_point("insert before sentinel: ", sentinel, 2, "\n")
                       : printf_point("insert before point: ", point, 3, "\n"));
                (++node)->item = pj;
                avl_insert_before(&tree, nodeaux, node);
            }
        } else {
            // Handle duplicates and points that are dominated by the immediate
            // previous one.
            const bool k_eq_j = (pk[0] == pj[0]) & (pk[1] == pj[1]) & (pk[2] == pj[2]);
            dominated = !keep_weakly;
            if (dominated) { // We don't keep duplicates;
                if (unlikely(k_eq_j) && pj < pk) // Only the first duplicated point is kept.
                    SWAP(pk, pj);
            } else { // or it is not a duplicate, so it is non-weakly dominated;
                dominated = likely(!k_eq_j)
                    // or pk was dominated, then this one is also dominated.
                    || pos_last_dom == row_index_from_ptr(points, pk, 3);
            }
            DEBUG3(printf_point("weakly dominated by pk: ", pk, 3, "\n"));
        }
        if (dominated) { // pj is dominated by a point in the tree or by prev.
            /* Map the order in p[], which is sorted, to the original order in
               points. */
            pos_last_dom = row_index_from_ptr(points, pj, 3);
            if (unlikely(nondom == NULL)) {
                // In this context, it means "position of the first dominated solution found".
                n_nondom = pos_last_dom;
                goto early_end;
            }
            nondom[pos_last_dom] = false;
            n_nondom--;
        } else {
            pk = pj;
        }
        j++;
    } while (j < size);

early_end:
    free(tnodes);
    free(rows);
    return n_nondom;
}

static inline size_t
find_dominated_3d_(const double * restrict points, size_t size, bool keep_weakly)
{
    return find_nondominated_set_3d_helper(points, size, keep_weakly, /* nondom=*/NULL);
}

/*
   Store which points are nondominated in nondom and return the number of
   nondominated points.
*/
static inline size_t
find_nondominated_set_3d_(const double * restrict points, size_t size, const bool keep_weakly, bool * restrict nondom)
{
    ASSUME(nondom != NULL);
    return find_nondominated_set_3d_helper(points, size, keep_weakly, nondom);
}

/**
   Brute-force algorithm O(size^2 * dim), with a few improvements.

   This function can be used to identify nondominated solutions (nondom !=
   NULL) or find the first dominated one (nondom == NULL).  It also handles
   combinations of min/max objectives.
*/
static inline size_t
find_nondominated_set_agree_bf(const double * restrict points, size_t size, dimension_t dim,
                               const bool keep_weakly,
                               const enum objs_agree_t agree, const int * restrict minmax,
                               bool * restrict nondom)
{
    ASSUME(dim > 3);
    ASSUME(agree == AGREE_MINIMISE || agree == AGREE_MAXIMISE || agree == AGREE_NONE);
    assert((agree == AGREE_NONE) == (minmax != NULL));
    size_t new_size = size;
    size_t min_k = 0;
    for (size_t j = 1; j < size; j++) {
        const double * restrict pj = points + j * dim;
        size_t k = min_k;
        if (likely(nondom != NULL)) {
            assert(nondom[j]);
            while (!nondom[k])
                k++;
            min_k = k;
            ASSUME(k < j);
            ASSUME(nondom[k]);
        }
        for (; k < j; k++) {
            if (likely(nondom != NULL)) {
                assert(nondom[j]);
                if (!nondom[k]) continue;
            }

            const double * restrict pk = points + k * dim;
            // Use unsigned instead of bool to allow auto-vectorization.
            unsigned k_leq_j = true, j_leq_k = true;
            if (agree == AGREE_NONE) {
                for (dimension_t d = 0; d < dim; d++) {
                    double cmp = minmax[d] * (pk[d] - pj[d]);
                    k_leq_j &= (cmp >= 0.0);
                    j_leq_k &= (cmp <= 0.0);
                }
            } else {
                for (dimension_t d = 0; d < dim; d++) {
                    double cmp = (agree == AGREE_MINIMISE) ? pj[d] - pk[d] : pk[d] - pj[d];
                    k_leq_j &= (cmp >= 0.0);
                    j_leq_k &= (cmp <= 0.0);
                }
            }
            // k is removed if it is dominated by j.
            unsigned dom_k = (!k_leq_j) & j_leq_k;
            // j is removed if it is weakly dominated by k (unless keep_weakly).
            // Only the first duplicated point is kept.
            unsigned dom_j = !keep_weakly ? k_leq_j : (k_leq_j & (!j_leq_k));

            /* As soon as j_leq_k and k_leq_j become false, neither k or j will
               be removed, so skip the rest.  */
            if (!(dom_k | dom_j)) continue;

            assert(dom_k ^ dom_j); // At least one but not both can be removed.

            size_t last_dom_pos = dom_j ? j : k;
            if (unlikely(nondom == NULL))
                return last_dom_pos;

            new_size--; // Something dominated.
            assert(nondom[last_dom_pos]);
            nondom[last_dom_pos] = false;
            if (dom_j)
                break;
        }
    }
    return new_size;
}

static inline size_t
find_dominated_point_agree_(const double * restrict points, size_t size, dimension_t dim,
                            const bool keep_weakly,
                            enum objs_agree_t agree, const int * restrict minmax)
{
    return find_nondominated_set_agree_bf(points, size, dim, keep_weakly,
                                          agree, minmax, /* nondom=*/NULL);
}

/* Stop as soon as one dominated point is found and return its position.
**/
static inline size_t
find_dominated_point_(const double * restrict points, size_t size, dimension_t dim,
                      const bool keep_weakly,
                      enum objs_agree_t agree, const int * restrict minmax)
{
    ASSUME(minmax != NULL);
    if (size < 2)
        return size;

    ASSUME(dim >= 2);
    if (dim <= 3) {
        const double * pp = force_agree_minimize(points, size, &dim, agree, minmax);
        size_t res;
        if (dim == 2) {
            res = find_dominated_2d_(pp, size, keep_weakly);
        } else {
            res = find_dominated_3d_(pp, size, keep_weakly);
        }
        if (pp != points)
            free((void *) pp);
        return res;
    }

    /* FIXME: Do not handle agree here, assume that objectives have been fixed
       already to agree on minimization/maximization.  */
    if (agree == AGREE_NONE)
        agree = check_all_minimize_maximize(minmax, dim);

    switch (agree) {
      case AGREE_NONE:
          return find_dominated_point_agree_(
              points, size, dim, keep_weakly, AGREE_NONE, minmax);
      case AGREE_MINIMISE:
          return find_dominated_point_agree_(
              points, size, dim, keep_weakly, AGREE_MINIMISE, /*minmax=*/NULL);
      case AGREE_MAXIMISE:
          return find_dominated_point_agree_(
              points, size, dim, keep_weakly, AGREE_MAXIMISE, /*minmax=*/NULL);
      default: // LCOV_EXCL_LINE # nocov
          unreachable();
    }
}

/**
   Store which points are nondominated in nondom and return the number of
   nondominated points.
*/
static inline size_t
find_nondominated_set_(const double * restrict points, size_t size, dimension_t dim,
                       const bool keep_weakly,
                       enum objs_agree_t agree, const int * restrict minmax,
                       bool * restrict nondom)
{
    if (size < 2)
        return size;

    ASSUME(dim >= 2);
    ASSUME(minmax != NULL);
    ASSUME(nondom != NULL);

    if (dim <= 3) {
        const double * pp = force_agree_minimize(points, size, &dim, agree, minmax);
        ASSUME(dim >= 2);
        size_t res;
        if (dim == 2) {
            res = find_nondominated_set_2d_(pp, size, keep_weakly, nondom);
        } else {
            res = find_nondominated_set_3d_(pp, size, keep_weakly, nondom);
        }
        if (pp != points)
            free((void *) pp);
        return res;
    }

    /* FIXME: Do not handle agree here, assume that objectives have been fixed
       already to agree on minimization/maximization.  */
    if (agree == AGREE_NONE)
        agree = check_all_minimize_maximize(minmax, dim);

    ASSUME(minmax != NULL);
    ASSUME(nondom != NULL);

    switch (agree) {
      case AGREE_NONE:
          return find_nondominated_set_agree_bf(
              points, size, dim, keep_weakly, AGREE_NONE, minmax, nondom);
      case AGREE_MINIMISE:
          return find_nondominated_set_agree_bf(
              points, size, dim, keep_weakly, AGREE_MINIMISE, /* minmax=*/NULL, nondom);
      case AGREE_MAXIMISE:
          return find_nondominated_set_agree_bf(
              points, size, dim, keep_weakly, AGREE_MAXIMISE, /* minmax=*/NULL, nondom);
      default: // LCOV_EXCL_LINE # nocov
          unreachable();
    }
}

static inline size_t
find_dominated_point_agree(const double * restrict points, size_t size, dimension_t dim,
                           enum objs_agree_t agree, const int * restrict minmax)
{
    ASSUME(dim >= 2);
    return find_dominated_point_(points, size, dim, /* keep_weakly=*/false,
                                 agree, minmax);
}

static inline size_t
find_dominated_point(const double * restrict points, size_t size, dimension_t dim,
                     const int * restrict minmax)
{
    ASSUME(dim >= 2);
    return find_dominated_point_(points, size, dim, /* keep_weakly=*/false,
                                 AGREE_NONE, minmax);
}

static inline size_t
find_weakly_dominated_point(const double * restrict points, size_t size, dimension_t dim,
                            const bool * restrict maximise)
{
    ASSUME(dim >= 2);
    const int * minmax = minmax_from_bool(maximise, dim);
    size_t pos = find_dominated_point_(points, size, dim, /* keep_weakly=*/false,
                                       AGREE_NONE, minmax);
    free((void *)minmax);
    return pos;
}

static inline size_t
find_nondominated_set_agree(const double * restrict points, size_t size, dimension_t dim,
                            const int agree, const int * restrict minmax,
                            bool * restrict nondom)
{
    ASSUME(dim >= 2);
    ASSUME(agree == AGREE_MINIMISE || agree == AGREE_MAXIMISE || agree == AGREE_NONE);
    return find_nondominated_set_(points, size, dim, /* keep_weakly=*/false,
                                  (enum objs_agree_t) agree, minmax, nondom);
}

static inline size_t
find_nondominated_set(const double * restrict points, size_t size, dimension_t dim,
                      const int * restrict minmax, bool * restrict nondom)
{
    ASSUME(dim >= 2);
    size_t new_size = find_nondominated_set_(
        points, size, dim, /* keep_weakly=*/false, AGREE_NONE, minmax, nondom);

    if (new_size > size || new_size == 0 || new_size == SIZE_MAX) { /* This can't happen.  */
        fatal_error ("%s:%d: a bug happened: new_size > old_size!\n"
                     "# size\tnondom\tdom\n"
                     "  %zu\t%zu\t%zd\n",
                     __FILE__, __LINE__, size, new_size, size - new_size);
    }
    return new_size;
}

static inline size_t
find_weak_nondominated_set(const double * restrict points, size_t size, dimension_t dim,
                           const int * restrict minmax, bool * restrict nondom)
{
    ASSUME(dim >= 2);
    return find_nondominated_set_(points, size, dim, /* keep_weakly=*/true,
                                  AGREE_NONE, minmax, nondom);
}

static inline size_t
find_weak_nondominated_set_minimise(const double * restrict points,
                                    size_t size, dimension_t dim,
                                    bool * restrict nondom)
{
    ASSUME(dim >= 2);
    const int * minmax = minmax_minimise(dim);
    ASSUME(minmax != NULL);
    size_t new_size = find_weak_nondominated_set(points, size, dim, minmax,
                                                 nondom);
    free((void *)minmax);
    return new_size;
}


static inline size_t
get_nondominated_set(double **pareto_set_p,
                     const double * restrict points, size_t size, dimension_t dim,
                     const int * restrict minmax)
{
    ASSUME(dim >= 2);
    bool * nondom = nondom_init(size);
    size_t new_size = find_nondominated_set(points, size, dim, minmax, nondom);
    double * pareto_set = malloc(new_size * dim * sizeof(*pareto_set));

    if (new_size < size) {
        size_t n = 0, k = 0;
        do {
            while (!nondom[n]) n++;
            memcpy(pareto_set + dim * k, points + dim * n, sizeof(points[0]) * dim);
            k++;
            n++;
        } while (k < new_size);
    } else {
        // Nothing is dominated. Copy everything in one go.
        memcpy(pareto_set, points, sizeof(points[0]) * dim * size);
    }
    free (nondom);
    *pareto_set_p = pareto_set;
    return new_size;
}

static inline size_t
filter_dominated_set(double * restrict points, size_t size, dimension_t dim,
                     const int * restrict minmax)
{
    ASSUME(dim >= 2);
    ASSUME(size > 0);

    bool * nondom = nondom_init(size);
    size_t new_size = find_nondominated_set(points, size, dim, minmax, nondom);

    if (new_size < size) {
        size_t k = 0;
        while (nondom[k]) k++; // Find first dominated.
        size_t n = k;
        while (k < new_size) { // Otherwise, all the nondominated are at the top.
            do {
                n++;
            } while (!nondom[n]); // Find next nondominated.
            memcpy(points + dim * k, points + dim * n, sizeof(points[0]) * dim);
            k++;
        }
    }
    free (nondom);
    return new_size;
}

static inline bool *
is_nondominated_minmax(const double * restrict data, size_t npoint, dimension_t nobj,
                       bool keep_weakly, const int * restrict minmax)
{
    bool * nondom = nondom_init(npoint);
    find_nondominated_set_(data, npoint, nobj, keep_weakly, AGREE_NONE, minmax, nondom);
    return nondom;
}

static inline bool *
is_nondominated_minimise(const double * restrict data, size_t npoint, dimension_t nobj,
                         bool keep_weakly)
{
    const int * minmax = minmax_minimise(nobj);
    bool * nondom = is_nondominated_minmax(data, npoint, nobj, keep_weakly, minmax);
    free((void *) minmax);
    return nondom;
}

_attr_maybe_unused static bool *
is_nondominated(const double * restrict data, size_t npoint, dimension_t nobj,
                bool keep_weakly, const bool * restrict maximise)
{
    ASSUME(nobj >= 2);
    const int * minmax = minmax_from_bool(maximise, nobj);
    bool * nondom = is_nondominated_minmax(data, npoint, nobj, keep_weakly, minmax);
    free((void *)minmax);
    return nondom;
}

static inline void
agree_objectives (double * restrict points, size_t size, dimension_t dim,
                  const enum objs_agree_t agree, const int * restrict minmax)
{
    for (dimension_t d = 0; d < dim; d++)
        if ((agree > 0 && minmax[d] < 0)
            || (agree < 0 && minmax[d] > 0))
            for (size_t k = 0; k < size; k++)
                points[k * dim + d] = -(points[k * dim + d]);
}


static inline void
normalise(double * restrict points, size_t size, dimension_t dim,
          int agree, const int * restrict minmax,
          const double lower_range, const double upper_range,
          const double * restrict lbound, const double * restrict ubound)
{
    const double range = upper_range - lower_range;
    double * diff = malloc(dim * sizeof(*diff));
    for (dimension_t d = 0; d < dim; d++) {
        diff[d] = ubound[d] - lbound[d];
        if (diff[d] == 0.0) // FIXME: Should we use approximate equality?
            diff[d] = 1; // FIXME: Do we need to handle agree?
    }

    for (size_t k = 0; k < size; k++) {
        double * p = points + k * dim;
        for (dimension_t d = 0; d < dim; d++)
            if ((agree > 0 && minmax[d] < 0) || (agree < 0 && minmax[d] > 0))
                p[d] = lower_range + range * (ubound[d] + p[d]) / diff[d];
            else
                p[d] = lower_range + range * (p[d] - lbound[d]) / diff[d];
    }

    free(diff);
}

_attr_maybe_unused static void
agree_normalise(double * restrict data, size_t npoint, dimension_t nobj,
                const bool * restrict maximise,
                const double lower_range, const double upper_range,
                const double * restrict lbound, const double * restrict ubound)
{
    const int * minmax = minmax_from_bool(maximise, nobj);
    // We have to make the objectives agree before normalisation.
    // FIXME: Do normalisation and agree in one step.
    agree_objectives(data, npoint, nobj, AGREE_MINIMISE, minmax);
    normalise(data, npoint, nobj, AGREE_MINIMISE, minmax,
              lower_range, upper_range, lbound, ubound);
    free ((void *)minmax);
}

int * pareto_rank(const double * restrict points, size_t size, dimension_t dim) _attr_malloc;

#endif /* NONDOMINATED_H */
