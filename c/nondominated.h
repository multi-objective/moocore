#ifndef NONDOMINATED_H
#define NONDOMINATED_H

#include <string.h> // memcpy
#include <math.h> // INFINITY
#include "common.h"
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

static inline enum objs_agree_t
check_all_minimize_maximize(const signed char * restrict minmax, dimension_t dim)
{
    bool all_minimize = true, all_maximize = true;
    for (dimension_t d = 0; d < dim; d++) {
        if (minmax[d] < 0) {
            all_maximize = false;
        } else if (minmax[d] > 0) {
            all_minimize = false;
        } else {
            all_minimize = false;
            all_maximize = false;
            break;
        }
    }
    assert(!all_maximize || !all_minimize);
    if (all_minimize) return AGREE_MINIMISE;
    if (all_maximize) return AGREE_MAXIMISE;
    return AGREE_NONE;
}

/* Convert from bool vector to minmax vector.  */
static inline signed char *
minmax_from_bool(const bool * restrict maximise, dimension_t nobj)
{
    ASSUME(nobj > 0 && nobj < 128);
    signed char * minmax = malloc(nobj * sizeof(*minmax));
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

static inline const signed char *
default_minmax(dimension_t nobj, signed char default_value)
{
    ASSUME(nobj > 0 && nobj < 128);
    ASSUME(default_value == AGREE_MINIMISE || default_value == AGREE_MAXIMISE);
    signed char * minmax = malloc(nobj * sizeof(*minmax));
    for (dimension_t i = 0; i < nobj; i++)
        minmax[i] = default_value;
    return minmax;
}

static inline const signed char *
minmax_minimise(dimension_t nobj)
{
    return default_minmax(nobj, AGREE_MINIMISE);
}

static inline const signed char *
minmax_maximise(dimension_t nobj)
{
    return default_minmax(nobj, AGREE_MAXIMISE);
}

// FIXME: It would be more efficient to default to false (using calloc) and
// change to true.
static inline bool *
nondom_init (size_t size)
{
    bool * nondom = malloc(size * sizeof(*nondom));
    for (size_t n = 0; n < size; n++)
        nondom[n] = true;
    return nondom;
}

static inline const double *
force_agree_minimize(const double * restrict points, dimension_t dim, size_t size,
                     const signed char * restrict minmax,
                     _attr_maybe_unused const enum objs_agree_t agree)
{
    assert(agree != AGREE_MINIMISE);
    bool no_copy = true;
    for (dimension_t d = 0; d < dim; d++) {
        if (minmax[d] > 0) {
            no_copy = false;
            break;
        }
    }
    if (no_copy)
        return points;

    double * pnew = malloc(dim * size * sizeof(*pnew));
    memcpy(pnew, points, dim * size * sizeof(*pnew));

    for (dimension_t d = 0; d < dim; d++) {
        assert(minmax[d] != 0);
        if (minmax[d] > 0)
            for (size_t k = 0; k < size; k++)
                pnew[k * dim + d] = -pnew[k * dim + d];
    }
    return pnew;
}

static inline const double **
generate_sorted_pp_2d(const double *points, size_t size)
{
    const double ** p = malloc(size * sizeof(*p));
    for (size_t k = 0; k < size; k++)
        p[k] = points + 2 * k;

    // Sort in ascending lexicographic order from the last dimension.
    qsort(p, size, sizeof(*p), &cmp_double_asc_rev_2d);
    return p;
}

static inline const double **
generate_sorted_pp_3d(const double *points, size_t size)
{
    const double ** p = malloc(size * sizeof(*p));
    for (size_t k = 0; k < size; k++)
        p[k] = points + 3 * k;

    // Sort in ascending lexicographic order from the last dimension.
    qsort(p, size, sizeof(*p), &cmp_double_asc_rev_3d);
    return p;
}

static inline size_t
find_nondominated_2d_helper_(const double * restrict points, size_t size,
                             bool * restrict nondom, const bool keep_weakly)
{
    ASSUME(size >= 2);
    const double **p = generate_sorted_pp_2d(points, size);
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

                size_t pos_first_dom = (pj - points) / 2;
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

/*
  Stop as soon as one dominated point is found and return its position (or size
  if no dominated point found).

*/
static inline size_t
find_dominated_2d_(const double * restrict points, size_t size, const bool keep_weakly)
{
    return find_nondominated_2d_helper_(points, size, /*nondom=*/NULL, keep_weakly);
}


/*
   Store which points are nondominated in nondom and return the number of
   nondominated points.

   2D dimension-sweep algorithm by H. T. Kung, F. Luccio, and F. P. Preparata.
   On Finding the Maxima of a Set of Vectors. Journal of the ACM,
   22(4):469–476, 1975.

   Duplicated points may be removed in any order due to qsort not being stable.
*/

static inline size_t
find_nondominated_set_2d_(const double * restrict points, size_t size,
                          bool * restrict nondom, const bool keep_weakly)
{
    ASSUME(nondom != NULL);
    return find_nondominated_2d_helper_(points, size, nondom, keep_weakly);
}


static inline void
print_point(const double * p) {
    printf("%g %g %g", p[0], p[1], p[2]);
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
                                bool * restrict nondom, const bool keep_weakly)
{
    ASSUME(size >= 2);
    const double **p = generate_sorted_pp_3d(points, size);

    avl_tree_t tree;
    avl_init_tree(&tree, cmp_pdouble_asc_x_nonzero);
    avl_node_t * tnodes = malloc((size+1) * sizeof(*tnodes));
    avl_node_t * node = tnodes;
    node->item = p[0];
    avl_insert_top(&tree, node);

    const double sentinel[] = { INFINITY, -INFINITY };
    (++node)->item = sentinel;
    avl_insert_after(&tree, node - 1, node);

    // In this context, size means "no dominated solution found".
    size_t pos_last_dom = size, n_nondom = size, j = 1;
    const double * restrict pk = p[0];
    do {
        const double * restrict pj = p[j];
        // printf("pj: "); print_point(pj); printf("\n");
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
                // printf("res > 0: prev: "); print_point(prev); printf("\n");
                nodeaux = nodeaux->next;
            } else if (nodeaux->prev) {// nodeaux goes after pj, so move to the next one.
                const double * restrict prev = nodeaux->prev->item;
                assert(prev[0] != sentinel[0]);
                assert(prev[0] <= pj[0]);
                dominated = prev[1] <= pj[1];
                // printf("res < 0: prev: "); print_point(prev); printf("\n");
            } else {
                dominated = false;
            }

            if (!dominated) { // pj is NOT dominated by a point in the tree.
                const double * restrict point = nodeaux->item;
                assert(pj[0] <= point[0]);
                // Delete everything in the tree that is dominated by pj.
                while (pj[1] <= point[1]) {
                    // printf("delete point: "); print_point(point); printf("\n");
                    assert(pj[0] <= point[0]);
                    nodeaux = nodeaux->next;
                    point = nodeaux->item;
                    /* FIXME: A possible speed up is to delete without
                       rebalancing the tree because avl_insert_before() will
                       rebalance. */
                    avl_unlink_node(&tree, nodeaux->prev);
                }
                // printf("insert before point: "); print_point(point); printf("\n");
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
                    || pos_last_dom == (size_t) ((pk - points) / 3);
            }
            // printf("dom by prev: "); print_point(prev); printf("\n");
        }
        if (dominated) { // pj is dominated by a point in the tree or by prev.
            /* Map the order in p[], which is sorted, to the original order in
               points. */
            pos_last_dom = (pj - points) / 3;
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
    free(p);
    return n_nondom;
}

static inline size_t
find_dominated_3d_(const double * points, size_t size, const bool keep_weakly)
{
    return find_nondominated_set_3d_helper(points, size, /* nondom=*/NULL,
                                           keep_weakly);
}

/*
   Store which points are nondominated in nondom and return the number of
   nondominated points.
*/
static inline size_t
find_nondominated_set_3d_(const double * restrict points, size_t size, bool * restrict nondom, const bool keep_weakly)
{
    ASSUME(nondom != NULL);
    return find_nondominated_set_3d_helper(points, size, nondom, keep_weakly);
}

static inline size_t
find_nondominated_set_agree_(const double * restrict points, dimension_t dim, size_t size,
                             bool * restrict nondom, const bool keep_weakly,
                             const enum objs_agree_t agree, const signed char * restrict minmax)
{
    ASSUME(dim > 3);
    ASSUME(agree == AGREE_MINIMISE || agree == AGREE_MAXIMISE || agree == AGREE_NONE);
    ASSUME(minmax == NULL || agree != AGREE_NONE);
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

            if (unlikely(nondom == NULL)) {
                return dom_k ? k : j;
            }
            if (dom_j) {
                assert(nondom[j]);
                nondom[j] = false;
                new_size--;
                break;
            }
            nondom[k] = false;
            new_size--;
        }
    }
    return new_size;
}

static inline size_t
find_dominated_point_agree_(const double * restrict points, dimension_t dim, size_t size,
                            const bool keep_weakly, const enum objs_agree_t agree,
                            const signed char * restrict minmax)
{
    return find_nondominated_set_agree_(points, dim, size, /* nondom=*/NULL,
                                        keep_weakly, agree, minmax);
}

/* Stop as soon as one dominated point is found and return its position.
**/
static inline size_t
find_dominated_point_(const double * restrict points, dimension_t dim, size_t size,
                      const signed char * restrict minmax, enum objs_agree_t agree,
                      const bool keep_weakly)
{
    ASSUME(minmax != NULL);
    if (size < 2)
        return size;

    ASSUME(dim >= 2);
    if (dim <= 3) {
        const double *pp = force_agree_minimize(points, dim, size, minmax, agree);
        size_t res;
        if (dim == 2) {
            res = find_dominated_2d_(pp, size, keep_weakly);
        } else {
            res = find_dominated_3d_(pp, size, keep_weakly);
        }
        if (pp != points)
            free((void*)pp);
        return res;
    }

    /* FIXME: Do not handle agree here, assume that objectives have been fixed
       already to agree on minimization/maximization.  */
    if (agree == AGREE_NONE)
        agree = check_all_minimize_maximize(minmax, dim);

    switch (agree) {
      case AGREE_NONE:
          return find_dominated_point_agree_(
              points, dim, size, keep_weakly, AGREE_NONE, minmax);
      case AGREE_MINIMISE:
          return find_dominated_point_agree_(
              points, dim, size, keep_weakly, AGREE_MINIMISE, /*minmax=*/NULL);
      case AGREE_MAXIMISE:
          return find_dominated_point_agree_(
              points, dim, size, keep_weakly, AGREE_MAXIMISE, /*minmax=*/NULL);
      default:
          unreachable();
    }
}

/* Store which points are nondominated in nondom and return the number of
   nondominated points.
**/
static inline size_t
find_nondominated_set_(const double * restrict points, dimension_t dim, size_t size,
                       const signed char * restrict minmax, enum objs_agree_t agree,
                       bool * nondom, const bool keep_weakly)
{
    if (size < 2)
        return size;

    ASSUME(dim >= 2);
    ASSUME(minmax != NULL);
    ASSUME(nondom != NULL);

    if (dim <= 3) {
        const double *pp = force_agree_minimize (points, dim, size, minmax, agree);
        size_t res;
        if (dim == 2) {
            res = find_nondominated_set_2d_(pp, size, nondom, keep_weakly);
        } else {
            res = find_nondominated_set_3d_(pp, size, nondom, keep_weakly);
        }
        if (pp != points)
            free((void*)pp);
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
          return find_nondominated_set_agree_(
              points, dim, size, nondom, keep_weakly, AGREE_NONE, minmax);
      case AGREE_MINIMISE:
          return find_nondominated_set_agree_(
              points, dim, size, nondom, keep_weakly, AGREE_MINIMISE, /*minmax=*/NULL);
      case AGREE_MAXIMISE:
          return find_nondominated_set_agree_(
              points, dim, size, nondom, keep_weakly, AGREE_MAXIMISE, /*minmax=*/NULL);
      default:
          unreachable();
    }
}

static inline size_t
find_dominated_point_agree(const double * restrict points, int dim, size_t size,
                           const signed char * restrict minmax,
                           enum objs_agree_t agree)
{
    ASSUME(dim >= 2 && dim <= 32);
    return find_dominated_point_(points, (dimension_t) dim, size, minmax,
                                 agree, /* keep_weakly = */false);
}

static inline size_t
find_dominated_point(const double * restrict points, int dim, size_t size,
                     const signed char * restrict minmax)
{
    ASSUME(dim >= 2 && dim <= 32);
    return find_dominated_point_(points, (dimension_t) dim, size, minmax,
                                 AGREE_NONE, /* keep_weakly = */false);
}

static inline size_t
find_weakly_dominated_point(const double * restrict points, int dim, size_t size,
                            const bool * restrict maximise)
{
    ASSUME(dim >= 2 && dim <= 32);
    const signed char * minmax = minmax_from_bool(maximise, (dimension_t) dim);
    size_t pos = find_dominated_point_(points, (dimension_t) dim, size, minmax,
                                       AGREE_NONE, /* keep_weakly = */false);
    free((void *)minmax);
    return pos;
}

static inline size_t
find_nondominated_set_agree(const double * restrict points, int dim, size_t size,
                            const signed char * restrict minmax, const signed char agree,
                            bool * restrict nondom)
{
    ASSUME(dim >= 2 && dim <= 32);
    ASSUME(agree == AGREE_MINIMISE || agree == AGREE_MAXIMISE || agree == AGREE_NONE);
    return find_nondominated_set_(points, (dimension_t) dim, size, minmax,
                                  (enum objs_agree_t) agree, nondom,
                                  /* keep_weakly = */false);
}

static inline size_t
find_nondominated_set(const double * restrict points, dimension_t dim, size_t size,
                      const signed char * restrict minmax, bool * restrict nondom)
{
    ASSUME(dim >= 2 && dim <= 32);
    size_t new_size = find_nondominated_set_(
        points, dim, size, minmax, AGREE_NONE, nondom,
        /* keep_weakly = */false);

    if (new_size > size || new_size == 0 || new_size == SIZE_MAX) { /* This can't happen.  */
        fatal_error ("%s:%d: a bug happened: new_size > old_size!\n"
                     "# size\tnondom\tdom\n"
                     "  %zu\t%zu\t%zd\n",
                     __FILE__, __LINE__, size, new_size, size - new_size);
    }
    return new_size;
}

static inline size_t
find_weak_nondominated_set(const double * restrict points, dimension_t dim, size_t size,
                           const signed char * restrict minmax,
                           bool * restrict nondom)
{
    ASSUME(dim >= 2 && dim <= 32);
    return find_nondominated_set_(points, dim, size, minmax, AGREE_NONE, nondom,
                                  /* keep_weakly = */true);
}

static inline size_t
find_weak_nondominated_set_minimise(const double * restrict points, dimension_t dim,
                                    size_t size, bool * restrict nondom)
{
    ASSUME(dim >= 2 && dim <= 32);
    const signed char * minmax = minmax_minimise(dim);
    ASSUME(minmax != NULL);
    size_t new_size = find_weak_nondominated_set(points, dim, size, minmax,
                                                 nondom);
    free((void *)minmax);
    return new_size;
}


static inline size_t
get_nondominated_set (double **pareto_set_p,
                      const double *points, int dim, size_t size,
                      const signed char *minmax)
{
    ASSUME(dim >= 2 && dim <= 32);

    bool * nondom = nondom_init(size);
    size_t new_size = find_nondominated_set(points, (dimension_t) dim, size, minmax, nondom);
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
filter_dominated_set(double * restrict points, int dim, size_t size,
                     const signed char * restrict minmax)
{
    ASSUME(dim >= 2 && dim <= 32);
    ASSUME(size > 0);

    bool * nondom = nondom_init(size);
    size_t new_size = find_nondominated_set(points, (dimension_t) dim, size, minmax, nondom);

    if (new_size < size) {
        size_t k = 0;
        while (nondom[k]) k++; // Find first dominated.
        size_t n = k;
        while (k < new_size) {
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
is_nondominated_minmax(const double * restrict data, dimension_t nobj, size_t npoint,
                       const signed char * restrict minmax, bool keep_weakly)
{
    bool * nondom = nondom_init(npoint);
    find_nondominated_set_(data, nobj, npoint, minmax, AGREE_NONE, nondom, keep_weakly);
    return nondom;
}

static inline bool *
is_nondominated_minimise(const double * data, dimension_t nobj, size_t npoint,
                         bool keep_weakly)
{
    const signed char * minmax = minmax_minimise(nobj);
    bool * nondom = is_nondominated_minmax(data, nobj, npoint, minmax,
                                           keep_weakly);
    free((void *) minmax);
    return nondom;
}

_attr_maybe_unused static bool *
is_nondominated(const double * restrict data, int nobj, size_t npoint,
                const bool * restrict maximise, bool keep_weakly)
{
    ASSUME(nobj >= 2 && nobj <= 32);
    const signed char * minmax = minmax_from_bool(maximise, (dimension_t) nobj);
    bool * nondom = is_nondominated_minmax(data, (dimension_t) nobj, npoint,
                                           minmax, keep_weakly);
    free((void *)minmax);
    return nondom;
}

static inline void
agree_objectives (double * restrict points, int dim, int size,
                  const signed char * restrict minmax, const enum objs_agree_t agree)
{
    for (int d = 0; d < dim; d++)
        if ((agree > 0 && minmax[d] < 0)
            || (agree < 0 && minmax[d] > 0))
            for (int k = 0; k < size; k++)
                points[k * dim + d] = -(points[k * dim + d]);
}


static inline void
normalise(double * restrict points, int dim, int size,
          const signed char * restrict minmax, signed char agree,
          const double lower_range, const double upper_range,
          const double * restrict lbound, const double * restrict ubound)
{
    const double range = upper_range - lower_range;
    double * diff = malloc(dim * sizeof(*diff));
    int k, d;
    for (d = 0; d < dim; d++) {
        diff[d] = ubound[d] - lbound[d];
        if (diff[d] == 0.0) // FIXME: Should we use approximate equality?
            diff[d] = 1; // FIXME: Do we need to handle agree?
    }

    for (k = 0; k < size; k++) {
        double * p = points + k * dim;
        for (d = 0; d < dim; d++)
            if ((agree > 0 && minmax[d] < 0) || (agree < 0 && minmax[d] > 0))
                p[d] = lower_range + range * (ubound[d] + p[d]) / diff[d];
            else
                p[d] = lower_range + range * (p[d] - lbound[d]) / diff[d];
    }

    free(diff);
}

_attr_maybe_unused static void
agree_normalise (double * restrict data, int nobj, int npoint,
                 const bool * restrict maximise,
                 const double lower_range, const double upper_range,
                 const double * restrict lbound, const double * restrict ubound)
{
    const signed char * minmax = minmax_from_bool(maximise, (dimension_t)nobj);
    // We have to make the objectives agree before normalisation.
    // FIXME: Do normalisation and agree in one step.
    agree_objectives(data, nobj, npoint, minmax, AGREE_MINIMISE);
    normalise(data, nobj, npoint, minmax, AGREE_MINIMISE,
              lower_range, upper_range, lbound, ubound);
    free ((void *)minmax);
}

int * pareto_rank(const double * restrict points, size_t size, int dim) _attr_malloc;

#endif /* NONDOMINATED_H */
