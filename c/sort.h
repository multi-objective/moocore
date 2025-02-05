#ifndef   	SORT_H_
# define   	SORT_H_

#include "common.h"

/*
   x < y, i.e., x is strictly lower than y in all dimensions. Assumes minimization.
*/

static inline bool
strongly_dominates(const double * restrict x, const double * restrict y, dimension_t dim)
{
    ASSUME(dim >= 2);
    for (dimension_t i = 0; i < dim; i++)
        if (x[i] >= y[i])
            return false;
    return true;
}

static inline int
cmp_double_asc_rev(const void * restrict p1, const void * restrict p2, dimension_t dim)
{
    const double *x1 = *((const double **)p1);
    const double *x2 = *((const double **)p2);
    for (int i = dim - 1; i >= 0; i--) {
        if (x1[i] < x2[i])
            return -1;
        if (x1[i] > x2[i])
            return 1;
    }
    return 0;
}

static inline int
cmp_double_asc_rev_3d(const void * restrict p1, const void * restrict p2)
{
    return cmp_double_asc_rev(p1, p2, 3);
}


static inline int
cmp_double_asc_y_des_x(const void * restrict p1, const void * restrict p2)
{
    const double x1 = *(const double *)p1;
    const double x2 = *(const double *)p2;
    const double y1 = *((const double *)p1+1);
    const double y2 = *((const double *)p2+1);
    return (y1 < y2) ? -1: ((y1 > y2) ? 1 : (x1 > x2 ? -1 : 1));
}


#endif 	    /* !SORT_H_ */
