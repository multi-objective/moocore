#ifndef   	SORT_H_
# define   	SORT_H_

#include "common.h"

// ---------- Relational functions (return bool) -----------------------------


/*
   x < y, i.e., x is strictly lower than y in all dimensions. Assumes minimization.
*/

static inline bool
strongly_dominates(const double * restrict a, const double * restrict b, dimension_t dim)
{
    ASSUME(dim >= 2);
    for (dimension_t d = 0; d < dim; d++)
        if (a[d] >= b[d])
            return false;
    return true;
}

static inline bool
weakly_dominates(const double * restrict a, const double * restrict b, const dimension_t dim)
{
    ASSUME(dim >= 2);
    /* The code below is a vectorized version of this code:
    for (dimension_t d = 0; d < dim; d++)
        if (a[d] > b[d])
            return false;
    return true;
    */
    // GCC 15 is not able to infer this initialization from ASSUME().
    // unsigned instead of bool to help auto-vectorization.
    unsigned a_leq_b = (a[0] <= b[0]) & (a[1] <= b[1]);
    for (dimension_t d = 2; d < dim; d++)
        a_leq_b &= (a[d] <= b[d]);
    return (bool) a_leq_b;
}

static inline bool
lexicographic_less_3d(const double * restrict a, const double * restrict b)
{
    return a[2] < b[2] || (a[2] == b[2] && (a[1] < b[1] || (a[1] == b[1] && a[0] <= b[0])));
}

static inline bool
all_equal_double(const double * restrict a, const double * restrict b, dimension_t dim)
{
    ASSUME(dim >= 2);
    // The code below is written in a way that helps vectorization.
    // GCC 15 is not able to infer this initialization from ASSUME().
    // unsigned instead of bool to help auto-vectorization.
    unsigned a_eq_b = (a[0] == b[0]) & (a[1] == b[1]);
    for (dimension_t d = 2; d < dim; d++)
        a_eq_b &= (a[d] == b[d]);
    return (bool) a_eq_b;
}

// ---------- Comparison functions (e.g, qsort). Return 'int' ----------------

// General type for comparison functions used in qsort().
typedef int (*cmp_fun_t)(const void *, const void *);

static inline int
cmp_double_asc_rev(const void * restrict pa, const void * restrict pb, dimension_t dim)
{
    const double * restrict a = *((const double **)pa);
    const double * restrict b = *((const double **)pb);
    for (int i = dim - 1; i >= 0; i--) {
        if (a[i] < b[i])
            return -1;
        if (a[i] > b[i])
            return 1;
    }
    return 0;
}

static inline int
cmp_double_asc_rev_2d(const void * restrict pa, const void * restrict pb)
{
    return cmp_double_asc_rev(pa, pb, 2);
}

// Lexicographic order of coordinates (z,y,x)
static inline int
cmp_double_asc_rev_3d(const void * restrict pa, const void * restrict pb)
{
    return cmp_double_asc_rev(pa, pb, 3);
}

static inline int
cmp_double_asc_rev_4d(const void * restrict pa, const void * restrict pb)
{
    return cmp_double_asc_rev(pa, pb, 4);
}

static inline int
cmp_double_asc_only_dim(const void * restrict pa, const void * restrict pb, dimension_t dim)
{
    const double a = *(*(const double **)pa + dim);
    const double b = *(*(const double **)pb + dim);
    return (a < b) ? -1 : (a > b ? 1 : 0);
}

static inline int
cmp_double_asc_only_3d(const void * restrict pa, const void * restrict pb)
{
    return cmp_double_asc_only_dim(pa, pb, 2);
}

static inline int
cmp_double_asc_only_4d(const void * restrict pa, const void * restrict pb)
{
    return cmp_double_asc_only_dim(pa, pb, 3);
}

static inline int
cmp_double_asc_y_des_x(const void * restrict pa, const void * restrict pb)
{
    const double ax = *(const double *)pa;
    const double bx = *(const double *)pb;
    const double ay = *((const double *)pa + 1);
    const double by = *((const double *)pb + 1);
    return (ay < by) ? -1: ((ay > by) ? 1 : (ax > bx ? -1 : 1));
}

static inline int
cmp_double_asc_x_asc_y(const void * restrict pa, const void * restrict pb)
{
    const double ax = *(const double *)pa;
    const double bx = *(const double *)pb;
    const double ay = *((const double *)pa + 1);
    const double by = *((const double *)pb + 1);
    return (ax < bx) ? -1: ((ax > bx) ? 1 : (ay < by ? -1 : 1));
}

static inline int
cmp_double_asc_x_des_y(const void * restrict pa, const void * restrict pb)
{
    const double ax = *(const double *)pa;
    const double bx = *(const double *)pb;
    const double ay = *((const double *)pa + 1);
    const double by = *((const double *)pb + 1);
    return (ax < bx) ? -1: ((ax > bx) ? 1 : (ay > by ? -1 : 1));
}

static inline int
cmp_doublep_x_asc_y_asc(const void * restrict pa, const void * restrict pb)
{
    const double ax = **(const double **)pa;
    const double bx = **(const double **)pb;
    const double ay = *(*(const double **)pa + 1);
    const double by = *(*(const double **)pb + 1);
    return (ax < bx) ? -1 : ((ax > bx) ? 1 :
                             ((ay < by) ? -1 : ((ay > by) ? 1 : 0)));
}

static inline const double **
generate_sorted_doublep_2d(const double * restrict points,
                           size_t * restrict size, const double ref0)
{
    size_t n = *size;
    const double **p = malloc(n * sizeof(*p));
    size_t j = 0;
    for (size_t k = 0; k < n; k++) {
        /* There is no point in checking p[k][1] < ref[1] here because the
           algorithms have to check anyway. */
        if (points[2 * k] < ref0) {
            p[j] = points + 2 * k;
            j++;
        }
    }
    n = j;
    if (unlikely(n == 0)) {
        free(p);
    } else {
        qsort(p, n, sizeof(*p), cmp_doublep_x_asc_y_asc);
    }

    *size = n;
    return p;
}


#endif 	    /* !SORT_H_ */
