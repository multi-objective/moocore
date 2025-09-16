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
                     _attr_maybe_unused const signed char agree)
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

static inline int
cmp_double_asc_x_des_y(const void * restrict p1, const void * restrict p2)
{
    const double x1 = *(const double *)p1;
    const double x2 = *(const double *)p2;
    const double y1 = *((const double *)p1+1);
    const double y2 = *((const double *)p2+1);
    return (x1 < x2) ? -1: ((x1 > x2) ? 1 : (y1 > y2 ? -1 : 1));
}

static inline int
cmp_double_asc_x_asc_y(const void * restrict p1, const void * restrict p2)
{
    const double x1 = *(const double *)p1;
    const double x2 = *(const double *)p2;
    const double y1 = *((const double *)p1+1);
    const double y2 = *((const double *)p2+1);
    return (x1 < x2) ? -1: ((x1 > x2) ? 1 : (y1 < y2 ? -1 : 1));
}


static inline const double **
generate_sorted_pp_2d(const double *points, size_t size)
{
    const double **p = malloc (size * sizeof(*p));
    for (size_t k = 0; k < size; k++)
        p[k] = points + 2 * k;

    qsort(p, size, sizeof(*p), &cmp_double_2d_asc);
    return p;
}

static inline const double **
generate_sorted_pp_3d(const double *points, size_t size)
{
    const double **p = malloc (size * sizeof(*p));
    for (size_t k = 0; k < size; k++)
        p[k] = points + 3 * k;

    qsort(p, size, sizeof(*p), &cmp_double_3d_asc);
    return p;
}


/*
  Stop as soon as one dominated point is found and return its position (or
  SIZE_MAX if no dominated point found).

*/
static inline size_t
find_dominated_2d_(const double *points, size_t size, const bool keep_weakly)
{
    ASSUME(size >= 2);
    const double **p = generate_sorted_pp_2d(points, size);
    // SIZE_MAX means "no dominated solution found".
    size_t pos_first_dom = SIZE_MAX, k = 0, j = 1;
    do {
        if (p[k][0] > p[j][0]) {
            k = j;
        } else if (!keep_weakly || p[k][0] != p[j][0] || p[k][1] != p[j][1]) {
            // In this context, it means "position of the first dominated solution found".
            pos_first_dom = (p[j] - points) / 2;
            goto early_end;
        }
        j++;
    } while (j < size);

early_end:
    free(p);
    return pos_first_dom;
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
    ASSUME(size >= 2);
    // When compiling with -O3, GCC is able to create two versions of this loop
    // and move keep_weakly out.
    const double **p = generate_sorted_pp_2d(points, size);
    size_t n_nondom = size, k = 0, j = 1;
    do {
        if (p[k][0] > p[j][0]) {
            k = j;
        } else if (!keep_weakly || p[k][0] != p[j][0] || p[k][1] != p[j][1]) {
            /* Map the order in p[], which is sorted, to the original order
               in points. */
            nondom[(p[j] - points) / 2] = false;
            n_nondom--;
        }
        j++;
    } while (j < size);

    free(p);
    return n_nondom;
}

/*
   3D dimension-sweep algorithm by H. T. Kung, F. Luccio, and F. P. Preparata.
   On Finding the Maxima of a Set of Vectors. Journal of the ACM,
   22(4):469–476, 1975.

   A different implementation is available from Duarte M. Dias, Alexandre
   D. Jesus, Luís Paquete, A software library for archiving nondominated
   points, GECCO 2021. https://github.com/TLDart/nondLib/blob/main/nondlib.hpp

   Duplicated points may be removed in any order due to qsort not being stable.
*/

static inline size_t
find_nondominated_set_3d_helper(const double * restrict points, size_t size,
                                bool * restrict nondom,
                                const bool find_dominated_p,
                                const bool keep_weakly)
{
    ASSUME (size >= 2);
    const double **p = generate_sorted_pp_3d(points, size);

    avl_tree_t tree;
    avl_init_tree(&tree, cmp_double_asc_x_asc_y);
    avl_node_t * tnodes = malloc((size+1) * sizeof(*tnodes));
    avl_node_t * node = tnodes;
    node->item = p[0];
    avl_insert_top(&tree, node);

    const double sentinel[2] = { INFINITY, INFINITY };
    (++node)->item = sentinel;
    avl_insert_after(&tree, node - 1, node);
    const avl_node_t * end = node;

    // In this context, SIZE_MAX means "no dominated solution found".
    size_t pos_last_dom = SIZE_MAX, n_nondom = size, j = 1;
    const double * prev = p[0];
    do {
        const double * pj = p[j];
        if (prev[0] > pj[0] || prev[1] > pj[1]) {
            avl_node_t * nodeaux;
            // == 1 means that nodeaux goes before pj, so move to the next one.
            if (avl_search_closest(&tree, pj, &nodeaux) == 1) {
                assert(nodeaux != end);
                prev = nodeaux->item;
                nodeaux = nodeaux->next;
            } else {
                assert(nodeaux->prev != end);
                // nodeaux goes after pj
                prev = nodeaux->prev ? nodeaux->prev->item : NULL;
            }
            const double * point = nodeaux->item;
            assert(pj[0] <= point[0]);
            // If pj[0] == point[0], then pj[1] < point[1]
            assert(pj[0] < point[0] || pj[1] < point[1]);

            if (prev && prev[1] <= pj[1]) { // pj is dominated by a point in the tree.
                assert(prev[0] <= pj[0]);
                // It cannot be a duplicate because we already handled them above.
                assert(prev[0] != pj[0] || prev[1] != pj[1] || prev[2] != pj[2]);
                /* Map the order in p[], which is sorted, to the original order
                   in points. */
                pos_last_dom = (pj - points) / 3;
                if (find_dominated_p)
                    goto early_end;
                nondom[pos_last_dom] = false;
                n_nondom--;
            } else {
                // Delete everything in the tree that is dominated by pj.
                while (nodeaux != end && pj[1] <= point[1]) {
                    assert(pj[0] <= nodeaux->item[0]);
                    nodeaux = nodeaux->next;
                    point = nodeaux->item;
                    /* FIXME: A possible speed up is to delete without
                       rebalancing the tree because avl_insert_before() will
                       rebalance. */
                    avl_unlink_node(&tree, nodeaux->prev);
                }
                (++node)->item = pj;
                avl_insert_before(&tree, nodeaux, node);
            }
        } // Handle duplicates and points that are dominated by the immediate previous one.
        else if (!keep_weakly // we don't keep duplicates;
                 // or it is not a duplicate, so it is non-weakly dominated;
                 || prev[0] != pj[0] || prev[1] != pj[1] || prev[2] != pj[2]
                 // or the prev was dominated, then this one is also dominated.
                 || pos_last_dom == (size_t) ((prev - points) / 3)) {
            /* Map the order in p[], which is sorted, to the original order
               in points. */
            pos_last_dom = (pj - points) / 3;
            if (find_dominated_p)
                goto early_end;
            nondom[pos_last_dom] = false;
            n_nondom--;
        }
        prev = pj;
        j++;
    } while (j < size);

early_end:
    free(tnodes);
    free(p);
    return find_dominated_p ? pos_last_dom : n_nondom;
}

static inline size_t
find_dominated_3d_(const double * points, size_t size, const bool keep_weakly)
{
    return find_nondominated_set_3d_helper(points, size, /* nondom=*/NULL,
                                           /*find_dominated_p=*/true, keep_weakly);
}

/*
   Store which points are nondominated in nondom and return the number of
   nondominated points.
*/
static inline size_t
find_nondominated_set_3d_(const double * points, size_t size, bool * nondom, const bool keep_weakly)
{
    return find_nondominated_set_3d_helper(points, size, nondom,
                                           /*find_dominated_p=*/false, keep_weakly);
}

/* When find_dominated_p == true, then stop as soon as one dominated point is
   found and return its position.

   When find_dominated_p == false, store which points are nondominated in nondom
   and return the number of nondominated points.

*/
static inline size_t
find_nondominated_set_ (const double * restrict points, dimension_t dim, size_t size,
                        const signed char * restrict minmax, signed char agree,
                        bool * nondom, const bool find_dominated_p,
                        const bool keep_weakly)
{
    ASSUME(minmax != NULL);
    if (size < 2)
        return size;

    ASSUME(dim >= 2);

    if (dim <= 3) {
        const double *pp = force_agree_minimize (points, dim, size, minmax, agree);
        size_t res;
        if (dim == 2) {
            res = (find_dominated_p)
                ? find_dominated_2d_(pp, size, keep_weakly)
                : find_nondominated_set_2d_(pp, size, nondom, keep_weakly);
        } else {
            res = (find_dominated_p)
                ? find_dominated_3d_(pp, size, keep_weakly)
                : find_nondominated_set_3d_(pp, size, nondom, keep_weakly);
        }
        if (pp != points)
            free((void*)pp);
        return res;
    }

    if (agree == AGREE_NONE)
        agree = (signed char) check_all_minimize_maximize(minmax, dim);

    if (find_dominated_p) {
        assert(nondom == NULL);
        nondom = nondom_init(size);
    }

    size_t j, k;
    for (k = 0; k < size - 1; k++) {
        for (j = k + 1; j < size; j++) {

            if (!nondom[k]) break;
            if (!nondom[j]) continue;

            bool k_leq_j = true;
            bool j_leq_k = true;

            const double *pk = points + k * dim;
            const double *pj = points + j * dim;

            /* FIXME: As soon as j_leq_k and k_leq_j become false,
               neither k or j will be removed, so break.  */
            /* FIXME: Do not handle agree here, assume that objectives
               have been fixed already to agree on
               minimization/maximization.  */
            switch (agree) {
              case AGREE_NONE:
                  for (dimension_t d = 0; d < dim; d++) {
                      if (minmax[d] < 0) {
                          j_leq_k = j_leq_k && (pj[d] <= pk[d]);
                          k_leq_j = k_leq_j && (pk[d] <= pj[d]);
                      } else if (minmax[d] > 0) {
                          j_leq_k = j_leq_k && (pj[d] >= pk[d]);
                          k_leq_j = k_leq_j && (pk[d] >= pj[d]);
                      }
                  }
                  break;

              case AGREE_MINIMISE:
                  for (dimension_t d = 0; d < dim; d++) {
                      j_leq_k = j_leq_k && (pj[d] <= pk[d]);
                      k_leq_j = k_leq_j && (pk[d] <= pj[d]);
                  }
                  break;

              case AGREE_MAXIMISE:
                  for (dimension_t d = 0; d < dim; d++) {
                      j_leq_k = j_leq_k && (pj[d] >= pk[d]);
                      k_leq_j = k_leq_j && (pk[d] >= pj[d]);
                  }
                  break;

              default:
                  unreachable();
            }

            /* Only the last duplicated point is kept. The 2D and 3D versions
               depend on the order generated by qsort, which is not stable.  */

            // k is removed if it is weakly dominated by j (unless keep_weakly == FALSE).
            nondom[k] = !j_leq_k || (keep_weakly && k_leq_j);
            // j is removed if it is dominated by k.
            nondom[j] = (!k_leq_j || j_leq_k);

            assert(nondom[k] || nondom[j]); /* both cannot be removed.  */

            if (find_dominated_p && (!nondom[k] || !nondom[j])) {
                size_t res = nondom[k] ? j : k;
                free(nondom);
                return res;
            }
        }
    }

    if (find_dominated_p) {
        free(nondom);
        return SIZE_MAX;
    }

    size_t new_size = nondom[0];
    for (k = 1; k < size; k++)
        new_size += nondom[k];
    return new_size;
}

static inline size_t
find_dominated_point (const double * restrict points, int dim, size_t size,
                      const signed char * restrict minmax)
{
    ASSUME(dim >= 2);
    ASSUME(dim <= 32);
    size_t pos = find_nondominated_set_ (points, (dimension_t) dim, size, minmax,
                                         AGREE_NONE, /*nondom=*/NULL,
                                         /* find_dominated_p = */true,
                                         /* keep_weakly = */false);
    return pos;
}

static inline size_t
find_weakly_dominated_point(const double * restrict points, int dim, size_t size,
                            const bool * restrict maximise)
{
    ASSUME(dim >= 2);
    ASSUME(dim <= 32);
    const signed char * minmax = minmax_from_bool((dimension_t) dim, maximise);
    size_t pos = find_nondominated_set_ (points, (dimension_t) dim, size, minmax,
                                         AGREE_NONE, /*nondom=*/NULL,
                                         /* find_dominated_p = */true,
                                         /* keep_weakly = */false);
    free((void *)minmax);
    return pos;
}

static inline size_t
find_nondominated_set_agree (const double *points, int dim, size_t size,
                             const signed char *minmax, const signed char agree,
                             bool *nondom)
{
    ASSUME(dim >= 2);
    ASSUME(dim <= 32);
    return find_nondominated_set_(points, (dimension_t) dim, size, minmax, agree, nondom,
                                  /* find_dominated_p = */false,
                                  /* keep_weakly = */false);
}

static inline size_t
find_nondominated_set (const double *points, dimension_t dim, size_t size,
                       const signed char *minmax, bool *nondom)
{
    ASSUME(dim >= 2);
    ASSUME(dim <= 32);
    size_t new_size = find_nondominated_set_(
        points, dim, size, minmax, AGREE_NONE, nondom,
        /* find_dominated_p = */false,
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
    ASSUME(dim >= 2);
    ASSUME(dim <= 32);
    return find_nondominated_set_(points, dim, size, minmax, AGREE_NONE, nondom,
                                  /* find_dominated_p = */false,
                                  /* keep_weakly = */true);
}

static inline size_t
find_weak_nondominated_set_minimise(const double * restrict points, dimension_t dim,
                                    size_t size, bool * restrict nondom)
{
    const signed char * minmax = minmax_minimise(dim);
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
    ASSUME(dim >= 2);
    ASSUME(dim <= 32);
    ASSUME(size > 0);

    bool *nondom = nondom_init(size);
    size_t new_size = find_nondominated_set (points, (dimension_t) dim, size, minmax, nondom);
    double *pareto_set = malloc(sizeof (double) * new_size * dim);

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
filter_dominated_set (double *points, int dim, size_t size,
                      const signed char *minmax)
{
    ASSUME(dim >= 2);
    ASSUME(dim <= 32);
    ASSUME(size > 0);

    bool *nondom = nondom_init(size);
    size_t new_size = find_nondominated_set (points, (dimension_t) dim, size, minmax, nondom);

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
is_nondominated_minmax(const double * data, dimension_t nobj, size_t npoint,
                       const signed char * minmax, bool keep_weakly)
{
    bool * nondom = nondom_init(npoint);
    find_nondominated_set_(data, nobj, npoint, minmax, AGREE_NONE, nondom,
                           /* find_dominated_p = */false,
                           /* keep_weakly = */keep_weakly);
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
is_nondominated(const double * data, int nobj, size_t npoint,
                const bool * maximise, bool keep_weakly)
{
    ASSUME(nobj >= 2);
    ASSUME(nobj <= 32);
    const signed char * minmax = minmax_from_bool((dimension_t) nobj, maximise);
    bool * nondom = is_nondominated_minmax(data, (dimension_t) nobj, npoint,
                                           minmax, keep_weakly);
    free((void *)minmax);
    return nondom;
}

static inline void
agree_objectives (double *points, int dim, int size,
                  const signed char *minmax, const signed char agree)
{
    for (int d = 0; d < dim; d++)
        if ((agree > 0 && minmax[d] < 0)
            || (agree < 0 && minmax[d] > 0))
            for (int k = 0; k < size; k++)
                points[k * dim + d] = -(points[k * dim + d]);
}


static inline void
normalise (double *points, int dim, int size,
           const signed char *minmax, signed char agree,
           const double lower_range, const double upper_range,
           const double *lbound, const double *ubound)
{
    const double range = upper_range - lower_range;

    double *diff = malloc (dim * sizeof(double));
    int k, d;
    for (d = 0; d < dim; d++) {
        diff[d] = ubound[d] - lbound[d];
        if (diff[d] == 0.0) // FIXME: Should we use approximate equality?
            diff[d] = 1; // FIXME: Do we need to handle agree?
    }

    for (k = 0; k < size; k++) {
        double *p = points + k * dim;
        for (d = 0; d < dim; d++)
            if ((agree > 0 && minmax[d] < 0)
                || (agree < 0 && minmax[d] > 0))
                p[d] = lower_range + range * (ubound[d] + p[d]) / diff[d];
            else
                p[d] = lower_range + range * (p[d] - lbound[d]) / diff[d];
    }

    free (diff);
}

_attr_maybe_unused static void
agree_normalise (double *data, int nobj, int npoint,
                 const bool * maximise,
                 const double lower_range, const double upper_range,
                 const double *lbound, const double *ubound)
{
    const signed char * minmax = minmax_from_bool((dimension_t)nobj, maximise);
    // We have to make the objectives agree before normalisation.
    // FIXME: Do normalisation and agree in one step.
    agree_objectives (data, nobj, npoint, minmax, AGREE_MINIMISE);
    normalise(data, nobj, npoint, minmax, AGREE_MINIMISE,
              lower_range, upper_range, lbound, ubound);
    free ((void *)minmax);
}

int * pareto_rank (const double *points, int dim, int size);


#endif /* NONDOMINATED_H */
