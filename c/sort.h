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
any_less_than(const double * restrict a, const double * restrict b, dimension_t dim)
{
    for (dimension_t d = 0; d < dim; d++)
        if (a[d] < b[d])
            return true;
    return false;
}

static inline bool
lexicographic_less_2d(const double * restrict a, const double * restrict b)
{
    return a[1] < b[1] || (a[1] == b[1] && a[0] <= b[0]);
}

static inline bool
lexicographic_less_3d(const double * restrict a, const double * restrict b)
{
    return a[2] < b[2] || (a[2] == b[2] && lexicographic_less_2d(a, b));
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

// See usage below.
#define DEFINE_QSORT_CMP(typed_cmp_fn, elem_type)                              \
    static inline int                                                          \
    typed_cmp_fn(const elem_type restrict, const elem_type restrict);          \
                                                                               \
    static inline int                                                          \
    qsort_##typed_cmp_fn(const void * restrict _a, const void * restrict _b)   \
    {                                                                          \
        const elem_type restrict a = (const elem_type)_a;                      \
        const elem_type restrict b = (const elem_type)_b;                      \
        return typed_cmp_fn(a, b);                                             \
    }                                                                          \
                                                                               \
    static inline int                                                          \
    typed_cmp_fn(const elem_type restrict a, const elem_type restrict b)


/* FIXME: How to automatically check that typeof(*array) is compatible with
   the signature of type_cmp ?

   FIXME: This doesn't work if typed_cmp is not the name of an existing
   function, but an expression or passed as an argument to the caller.
*/
#define qsort_typesafe(array, nmemb, typed_cmp)                                \
    qsort((array), (nmemb), sizeof(*(array)), qsort_##typed_cmp)


static inline int
cmp_double_asc(double a, double b)
{
    // Generates branchless code, thus faster than:
    // return a < b ? -1 : (a > b ? 1 : 0);
    return (a > b) - (a < b);
}

static inline int
cmp_pdouble_asc_rev(const double * restrict a, const double * restrict b, dimension_t dim)
{
    ASSUME(dim >= 2);
    int i = dim - 1;
    int res = cmp_double_asc(a[i], b[i]);
    while (!res && i > 0) {
        i--;
        res = cmp_double_asc(a[i], b[i]);
    }
    return res;
}

// Lexicographic order of coordinates: asc y, then asc x
DEFINE_QSORT_CMP(cmp_pdouble_asc_rev_2d, double *)
{
    int cmpx = cmp_double_asc(a[0], b[0]);
    int cmpy = cmp_double_asc(a[1], b[1]);
    return cmpy ? cmpy : cmpx;
}

DEFINE_QSORT_CMP(cmp_ppdouble_asc_rev_2d, double **)
{
    return cmp_pdouble_asc_rev_2d(*a, *b);
}

// Lexicographic order of coordinates (z,y,x)
DEFINE_QSORT_CMP(cmp_ppdouble_asc_rev_3d, double **)
{
    return cmp_pdouble_asc_rev(*a, *b, 3);
}

DEFINE_QSORT_CMP(cmp_ppdouble_asc_rev_4d, double **)
{
    return cmp_pdouble_asc_rev(*a, *b, 4);
}

static inline int
cmp_pdouble_asc_only_dim(const double * restrict pa, const double * restrict pb, dimension_t dim)
{
    return cmp_double_asc(pa[dim], pb[dim]);
}

DEFINE_QSORT_CMP(cmp_ppdouble_asc_only_3d, double **)
{
    return cmp_pdouble_asc_only_dim(*a, *b, 2);
}

DEFINE_QSORT_CMP(cmp_ppdouble_asc_only_4d, double **)
{
    return cmp_pdouble_asc_only_dim(*a, *b, 3);
}

DEFINE_QSORT_CMP(cmp_pdouble_asc_x_nonzero, double *)
{
    ASSUME(a != b);
    return *a < *b ? -1 : 1;
}

// Deterministic tie-break by pointer value.
DEFINE_QSORT_CMP(cmp_pdouble_asc_x_nonzero_stable, double *)
{
    ASSUME(a != b);
    int cmp = cmp_double_asc(*a, *b);
    // Deterministic tie-break by pointer value.
    uintptr_t pa = (uintptr_t)a;
    uintptr_t pb = (uintptr_t)b;
    return cmp ? cmp : (pa < pb ? -1 : 1);
}

// Deterministic tie-break by pointer value.
DEFINE_QSORT_CMP(cmp_ppdouble_asc_x_nonzero_stable, double * const *)
{
    return cmp_pdouble_asc_x_nonzero_stable(*a, *b);
}

DEFINE_QSORT_CMP(cmp_pdouble_asc_y_des_x_nonzero, double *)
{
    const double ax = a[0];
    const double bx = b[0];
    const double ay = a[1];
    const double by = b[1];
    int cmp = cmp_double_asc(ay, by);
    return cmp ? cmp : (ax > bx ? -1 : 1);
}

static inline int
cmp_double_asc_x_asc_y(double ax, double ay, double bx, double by)
{
    // Faster than:
    // return (ax < bx) ? -1 : ((ax > bx) ? 1 :
    //                          ((ay < by) ? -1 : ((ay > by) ? 1 : 0)));
    int cmpx = cmp_double_asc(ax, bx);
    int cmpy = cmp_double_asc(ay, by);
    return cmpx ? cmpx : cmpy;
}

// Ascending lexicographic 2D (ascending x, then ascending y)
DEFINE_QSORT_CMP(cmp_pdouble_asc_x_asc_y, double *)
{
    return cmp_double_asc_x_asc_y(a[0], a[1], b[0], b[1]);
}

// Ascending lexicographic 2D (ascending x, then ascending y)
DEFINE_QSORT_CMP(cmp_ppdouble_asc_x_asc_y, double **)
{
    return cmp_pdouble_asc_x_asc_y(*a, *b);
}

DEFINE_QSORT_CMP(cmp_pdouble_asc_y_asc_z, double *)
{
    return cmp_double_asc_x_asc_y(a[1], a[2], b[1], b[2]);
}


static inline const double **
generate_row_pointers(const double * restrict points, size_t size, dimension_t dim)
{
    const double ** p = (const double **) malloc(size * sizeof(*p));
    for (size_t k = 0; k < size; k++)
        p[k] = points + dim * k;
    return p;
}

static inline const double **
generate_row_pointers_asc_x_asc_y(const double * restrict points, size_t n)
{
    const double ** p = generate_row_pointers(points, n, 2);
    qsort_typesafe(p, n, cmp_ppdouble_asc_x_asc_y);
    return p;
}


static inline const double **
generate_sorted_doublep_2d_filter_by_ref(const double * restrict points,
                                         size_t * restrict size, const double ref0)
{
    size_t n = *size;
    const double ** p = (const double **) malloc(n * sizeof(*p));
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
        qsort_typesafe(p, n, cmp_ppdouble_asc_x_asc_y);
    }

    *size = n;
    return p;
}

#endif 	    /* !SORT_H_ */
