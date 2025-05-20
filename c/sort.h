#ifndef   	SORT_H_
# define   	SORT_H_

#include "common.h"

/*
   x < y, i.e., x is strictly lower than y in all dimensions. Assumes minimization.
*/

static bool
strongly_dominates(const double * restrict x, const double * restrict y, dimension_t dim)
{
    ASSUME(dim >= 2);
    for (dimension_t i = 0; i < dim; i++)
        if (x[i] >= y[i])
            return false;
    return true;
}


#endif 	    /* !SORT_H_ */
