#ifndef NONDOMINATED_H
#define NONDOMINATED_H

#include <string.h> // memcpy
#include "common.h"

static inline bool *
nondom_init (size_t size)
{
    bool * nondom = malloc (sizeof(bool) * size);
    for (size_t n = 0; n <  size; n++)
        nondom[n] = true;
    return nondom;
}

static inline const double *
force_agree_minimize (const double *points, int dim_, size_t size,
                      const signed char *minmax, _attr_maybe_unused const signed char agree)
{
    assert(agree != AGREE_MINIMISE);
    dimension_t dim = (dimension_t) dim_;
    bool no_copy = true;
    for (dimension_t d = 0; d < dim; d++) {
        if (minmax[d] > 0) {
            no_copy = false;
            break;
        }
    }
    if (no_copy)
        return points;

    double *pnew = malloc(dim * size * sizeof(double));
    memcpy(pnew, points, dim * size * sizeof(double));

    for (dimension_t d = 0; d < dim; d++) {
        assert(minmax[d] != 0);
        if (minmax[d] > 0)
            for (size_t k = 0; k < size; k++)
                pnew[k * dim + d] = -pnew[k * dim + d];
    }
    return pnew;
}

static inline int compare_y_asc_x_asc (const void *p1, const void *p2)
{
    const double x1 = **(const double **)p1;
    const double x2 = **(const double **)p2;
    const double y1 = *(*(const double **)p1 + 1);
    const double y2 = *(*(const double **)p2 + 1);
    return (y1 < y2) ? -1 : ((y1 > y2) ? 1 :
                             ((x1 < x2) ? -1 : ((x1 > x2) ? 1 : 0)));
}

static inline
const double ** generate_sorted_pp_2d(const double *points, size_t size)
{
    const double **p = malloc (size * sizeof(*p));
    for (size_t k = 0; k < size; k++)
        p[k] = points + 2 * k;

    qsort(p, size, sizeof(*p), &compare_y_asc_x_asc);
    return p;
}

/*
   Stop as soon as one dominated point is found and return its position (or SIZE_MAX if no dominated point found).
*/
static inline size_t
find_dominated_2d_(const double *points, size_t size, const bool keep_weakly)
{
    ASSUME(size >= 2);
    const double **p = generate_sorted_pp_2d(points, size);
    // In this context, it means "no dominated solution found".
    size_t pos_first_dom = SIZE_MAX, k = 0, j = 1;
    do {
        while (j < size && p[k][0] <= p[j][0]) {
            if (!keep_weakly || p[k][0] != p[j][0] || p[k][1] != p[j][1]) {
                // In this context, it means "position of the first dominated solution found".
                pos_first_dom = (p[j] - points) / 2;
                goto early_end;
            }
            j++;
        }
        k = j;
        j++;
    } while (j < size);

early_end:
    free(p);
    return pos_first_dom;
}


/*
   Store which points are nondominated in nondom and return the number of
   nondominated points.
*/

static inline size_t
find_nondominated_set_2d_(const double * points, size_t size, bool * nondom,
                          const bool keep_weakly)
{
    ASSUME(size >= 2);
    // When compiling with -O3, GCC is able to create two versions of this loop
    // and move keep_weakly out.
    const double **p = generate_sorted_pp_2d(points, size);
    size_t n_nondom = size, k = 0, j = 1;
    do {
        while (j < size && p[k][0] <= p[j][0]) {
            if (!keep_weakly || p[k][0] != p[j][0] || p[k][1] != p[j][1]) {
                /* Map the order in p[], which is sorted, to the original order
                   in points. */
                nondom[(p[j] - points) / 2] = false;
                n_nondom--;
            }
            j++;
        }
        k = j;
        j++;
    } while (j < size);

    free(p);
    return n_nondom;
}

/* When find_dominated_p == true, then stop as soon as one dominated point is
   found and return its position.

   When find_dominated_p == false, store which points are nondominated in nondom
   and return the number of nondominated points.

*/
static inline size_t
find_nondominated_set_ (const double * points, int dim_, size_t size,
                        const signed char * minmax, signed char agree,
                        bool * nondom, bool find_dominated_p, bool keep_weakly)
{
    if (size < 2)
        return size;

    ASSUME(dim_ >= 2);
    dimension_t dim = (dimension_t) dim_;

    if (dim == 2) {
        const double *pp = force_agree_minimize (points, 2, size, minmax, agree);
        size_t res = (find_dominated_p)
            ? find_dominated_2d_(pp, size, keep_weakly)
            : find_nondominated_set_2d_(pp, size, nondom, keep_weakly);
        if (pp != points)
            free((void*)pp);
        return res;
    }

    if (agree == AGREE_NONE)
        agree = (signed char) check_all_minimize_maximize(minmax, dim);

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

            // k is removed if it is weakly dominated by j (unless keep_weakly == FALSE).
            nondom[k] = !j_leq_k || (keep_weakly && k_leq_j);
            // j is removed if it is dominated by k.
            nondom[j] = (!k_leq_j || j_leq_k);

            assert(nondom[k] || nondom[j]); /* both cannot be removed.  */

            if (find_dominated_p && (!nondom[k] || !nondom[j])) {
                return nondom[k] ? j : k;
            }
        }
    }

    if (find_dominated_p) return SIZE_MAX;

    size_t new_size = nondom[0];
    for (k = 1; k < size; k++)
        new_size += (size_t) nondom[k];
    return new_size;
}

static inline size_t
find_dominated_point (const double *points, int dim, size_t size,
                      const signed char *minmax)
{
    bool *nondom = nondom_init (size);
    size_t pos = find_nondominated_set_ (points, dim, size, minmax,
                                         AGREE_NONE, nondom,
                                         /* find_dominated_p = */true,
                                         /* keep_weakly = */false);
    free (nondom);
    return pos;
}

static inline size_t
find_nondominated_set_agree (const double *points, int dim, size_t size,
                             const signed char *minmax, const signed char agree,
                             bool *nondom)
{
    return find_nondominated_set_ (points, dim, size, minmax, agree, nondom,
                                   /* find_dominated_p = */false,
                                   /* keep_weakly = */false);
}

static inline size_t
find_nondominated_set (const double *points, int dim, size_t size,
                       const signed char *minmax, bool *nondom)
{
    return find_nondominated_set_ (points, dim, size, minmax, AGREE_NONE, nondom,
                                   /* find_dominated_p = */false,
                                   /* keep_weakly = */false);
}

static inline size_t
find_weak_nondominated_set (const double *points, int dim, size_t size,
                            const signed char *minmax, bool *nondom)
{
    return find_nondominated_set_ (points, dim, size, minmax, AGREE_NONE, nondom,
                                   /* find_dominated_p = */false,
                                   /* keep_weakly = */true);
}

static inline size_t
get_nondominated_set (double **pareto_set_p,
                      const double *points, int dim, size_t size,
                      const signed char *minmax)
{
    ASSUME(dim >= 1);
    ASSUME(dim <= 32);
    ASSUME(size > 0);

    bool *nondom = nondom_init(size);
    size_t new_size = find_nondominated_set (points, dim, size, minmax, nondom);
    if (new_size > size || new_size == 0 || new_size == SIZE_MAX) { /* This can't happen.  */
        fprintf (stderr,
                 "# size\tnondom\tdom\n"
                 "  %zu\t%zu\t%zd\n",  size, new_size, size - new_size);
        fatal_error ("%s:%d: a bug happened: new_size > old_size!\n",
                     __FILE__, __LINE__);
    }

    double *pareto_set = malloc (sizeof (double) * new_size * dim);
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
    ASSUME(dim >= 1);
    ASSUME(dim <= 32);
    ASSUME(size > 0);
    if (size == 1)
        return 1;

    bool *nondom = nondom_init(size);
    size_t new_size = find_nondominated_set (points, dim, size, minmax, nondom);
    if (new_size > size || new_size == 0 || new_size == SIZE_MAX) { /* This can't happen.  */
        fprintf (stderr,
                 "# size\tnondom\tdom\n"
                 "  %zu\t%zu\t%zd\n",  size, new_size, size - new_size);
        fatal_error ("%s:%d: a bug happened: new_size > old_size!\n",
                     __FILE__, __LINE__);
    }

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

_attr_maybe_unused static bool *
is_nondominated (const double * data, int nobj, int npoint, const bool * maximise, bool keep_weakly)
{
    bool * nondom = nondom_init(npoint);
    const signed char * minmax = minmax_from_bool(nobj, maximise);
    find_nondominated_set_ (data, nobj, (size_t) npoint, minmax, AGREE_NONE, nondom,
                            /* find_dominated_p = */false,
                            /* keep_weakly = */keep_weakly);
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
    int k, d;
    const double range = upper_range - lower_range;

    double *diff = malloc (dim * sizeof(double));
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
    const signed char * minmax = minmax_from_bool(nobj, maximise);
    // We have to make the objectives agree before normalisation.
    // FIXME: Do normalisation and agree in one step.
    agree_objectives (data, nobj, npoint, minmax, AGREE_MINIMISE);
    normalise(data, nobj, npoint, minmax, AGREE_MINIMISE,
              lower_range, upper_range, lbound, ubound);
    free ((void *)minmax);
}

int * pareto_rank (const double *points, int dim, int size);


#endif /* NONDOMINATED_H */
