#ifndef _ARCHIVING_PRIV_H_
#define _ARCHIVING_PRIV_H_

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "archiving.h"

/**
   Returns 0 if OK, negative values if there is a problem.
*/
static inline int
wrong_x_is_null(signed char * restrict x_is_null, const void * restrict x)
{
    if (*x_is_null == 0)
        return x != NULL ? 0 : ARCHIVE_INSERT_X_MUST_BE_NOT_NULL;
    if (*x_is_null == 1)
        return x == NULL ? 0 : ARCHIVE_INSERT_X_MUST_BE_NULL;

    *x_is_null = (x == NULL);
    return 0;
}


#endif // _ARCHIVING_PRIV_H_
