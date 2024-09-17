#ifndef   	LIBMISC_COMMON_H_
# define   	LIBMISC_COMMON_H_

#ifdef R_PACKAGE
#define R_NO_REMAP
#include <R.h>
#define fatal_error(...) Rf_error(__VA_ARGS__)
#define assert(EXP)                                                       \
    do { if (!(EXP)) { Rf_error("error: assertion failed: '%s' at %s:%d", \
                                #EXP, __FILE__, __LINE__);}} while(0)
#define errprintf Rf_error
#define warnprintf Rf_warning
#include "gcc_attribs.h"
#else
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "gcc_attribs.h"
#define Rprintf(...) printf(__VA_ARGS__)
void fatal_error(const char * format,...) __attribute__ ((format(printf, 1, 2))) __noreturn _no_warn_unused;
void errprintf(const char * format,...) __attribute__ ((format(printf, 1, 2)));
void warnprintf(const char *format,...)  __attribute__ ((format(printf, 1, 2)));
#endif
#include <stdbool.h>

#define eaf_assert(X) assert(X)

#if __GNUC__ >= 3
#define __cmp_op_min <
#define __cmp_op_max >
#define __cmp(op, x, y) ((x) __cmp_op_##op (y) ? (x) : (y))
#define __careful_cmp(op, x, y) __extension__({       \
            __auto_type _x__ = (x);                   \
            __auto_type _y__ = (y);                   \
            (void) (&_x__ == &_y__);                  \
            __cmp(op, _x__, _y__); })

#define MAX(x,y) __careful_cmp(max, x, y)
#define MIN(x,y) __careful_cmp(min, x, y)

#define CLAMP(x, xmin, xmax) __extension__({                                   \
            __auto_type _x__ = (x);                                            \
            __typeof__(_x__) _xmin__ = (xmin);                                 \
            __typeof__(_x__) _xmax__ = (xmax);                                 \
            _x__ <= _xmin__ ? _xmin__ : _x__ >= _xmax__ ? _xmax__ : _x__; })
#else
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define CLAMP(x, xmin, xmax) (MAX((xim), (MIN((x), (xmax)))))
#endif


#define DEBUG_DO(X)     do{ X;} while(0)
#define DEBUG_NOT_DO(X) while(0){ X;}

#if DEBUG >= 1
#define DEBUG1(X) DEBUG_DO(X)
#else
#define DEBUG1(X) DEBUG_NOT_DO(X)
#endif

#if DEBUG >= 2
#define DEBUG2(X) DEBUG_DO(X)
#else
#define DEBUG2(X) DEBUG_NOT_DO(X)
#endif

#if DEBUG >= 3
#define DEBUG3(X) DEBUG_DO(X)
#else
#define DEBUG3(X) DEBUG_NOT_DO(X)
#endif

#if DEBUG >= 4
#define DEBUG4(X) DEBUG_DO(X)
#else
#define DEBUG4(X) DEBUG_NOT_DO(X)
#endif

#ifndef R_PACKAGE
#define DEBUG2_PRINT(...) DEBUG2 (fprintf (stderr,  __VA_ARGS__))
#else
#define DEBUG2_PRINT(...) DEBUG2 (Rprintf ( __VA_ARGS__))
#endif

#define DEBUG2_FUNPRINT(...)                    \
    do { DEBUG2_PRINT ("%s(): ", __FUNCTION__); \
         DEBUG2_PRINT (__VA_ARGS__); } while(0)


/* This is deprecated. See https://www.gnu.org/software/libc/manual/html_node/Heap-Consistency-Checking.html
#if DEBUG >= 1
#ifndef MALLOC_CHECK_
#define MALLOC_CHECK_ 3
#endif
#endif
*/

#ifndef ignore_unused_result
#define ignore_unused_result(X)  do { if(X) {}} while(0);
#endif

typedef unsigned long ulong;
typedef long long longlong;

/* FIXME: Move this to a better place: matrix.h ? */
/* FIXME: Measure if this is faster than the R implementation of t()  */
static inline void
matrix_transpose_double(double *dst, const double *src,
                        const size_t nrows, const size_t ncols)
{
    size_t j, i, pos = 0;
    for (j = 0; j < ncols; j++) {
        for (i = 0; i < nrows; i++) {
            dst[pos] = src[j + i * ncols];
            pos++;
        }
    }
}

/* FIXME: Move this to nondominated.h */
enum objs_agree_t { AGREE_MINIMISE = -1, AGREE_NONE = 0, AGREE_MAXIMISE = 1 };

/* Convert from bool vector to minmax vector.  */
static inline signed char *
minmax_from_bool(int nobj, const bool * maximise)
{
    // unsigned int to fix -Walloc-larger-than= warning.
    signed char * minmax = malloc(sizeof(signed char) * (unsigned int) nobj);
    for (int k = 0; k < nobj; k++) {
        minmax[k] = (maximise[k]) ? AGREE_MAXIMISE : AGREE_MINIMISE;
    }
    return minmax;
}


#endif 	    /* !LIBMISC_COMMON_H_ */
