#ifndef MOOCORE_COMMON_H_
#define MOOCORE_COMMON_H_

#include "config.h"
#define MOOCORE_STRINGIFY(name) #name
#define MOOCORE_STRINGIFY_MACRO(macro) MOOCORE_STRINGIFY(macro)

#ifdef R_PACKAGE
#define R_NO_REMAP
#include <R.h>
#ifdef NDEBUG
# define assert(EXP) ((void)0)
#else
# define assert(EXP) do { if (unlikely(!(EXP)))                                \
            Rf_error("error: assertion failed: '%s' at %s:%d",                 \
                     #EXP, __FILE__, __LINE__);} while(0)
#endif
#include "gcc_attribs.h"
#define fatal_error(...) Rf_error(__VA_ARGS__)
#define moocore_perror(...) Rf_error(__VA_ARGS__)
#define errprintf Rf_error
#define warnprintf Rf_warning
#else
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "gcc_attribs.h"
__noreturn _attr_maybe_unused void fatal_error(const char * format,...) ATTRIBUTE_FORMAT_PRINTF(1, 2);
void errprintf(const char * format,...) ATTRIBUTE_FORMAT_PRINTF(1, 2);
void warnprintf(const char *format,...) ATTRIBUTE_FORMAT_PRINTF(1, 2);
#define moocore_perror(...) do {                                               \
        char buffer[1024] = "";                                                \
        snprintf(buffer, 1024, __VA_ARGS__);                                   \
        perror(buffer);                                                        \
        exit(EXIT_FAILURE);                                                    \
    } while(0)
#endif // R_PACKAGE

#include <stdbool.h>
#include <errno.h>
#include <stdint.h> // SIZE_MAX
#include <stdlib.h> // malloc, calloc, realloc

/* Return true if NMEMB * SIZE does not fit in size_t.  Same check as
   reallocarray() in OpenBSD:
   https://github.com/libressl/openbsd/blob/master/src/lib/libc/stdlib/reallocarray.c
*/
static inline bool
moocore_mul_overflow(size_t nmemb, size_t size)
{
    // sqrt(SIZE_MAX + 1): if both are smaller, the product cannot overflow.
    const size_t mul_no_overflow = (size_t)1 << (sizeof(size_t) * 4);
    return (nmemb >= mul_no_overflow || size >= mul_no_overflow)
        && nmemb > 0 && SIZE_MAX / nmemb < size;
}

/* The wrappers below stop with an error if NMEMB * SIZE overflows or if the
   allocation fails.  A NULL result is only returned when NMEMB * SIZE is
   zero, because malloc(0) may return NULL.  */
static inline void *
moocore_malloc(size_t nmemb, size_t size, const char *file, int line)
{
    if (unlikely(moocore_mul_overflow(nmemb, size))) {
        errno = ENOMEM;
        moocore_perror("%s:%d: malloc (%zu * %zu) overflows size_t",
                       file, line, nmemb, size);
    }
    void * p = malloc(nmemb * size);
    if (unlikely(!p && nmemb * size > 0))
        moocore_perror("%s:%d: malloc (%zu * %zu) failed",
                       file, line, nmemb, size);
    return p;
}

static inline void *
moocore_calloc(size_t nmemb, size_t size, const char *file, int line)
{
    if (unlikely(moocore_mul_overflow(nmemb, size))) {
        errno = ENOMEM;
        moocore_perror("%s:%d: calloc (%zu * %zu) overflows size_t",
                       file, line, nmemb, size);
    }
    void * p = calloc(nmemb, size);
    if (unlikely(!p && nmemb * size > 0))
        moocore_perror("%s:%d: calloc (%zu * %zu) failed",
                       file, line, nmemb, size);
    return p;
}

static inline void *
moocore_realloc(void * ptr, size_t nmemb, size_t size, const char *file, int line)
{
    if (unlikely(moocore_mul_overflow(nmemb, size))) {
        errno = ENOMEM;
        moocore_perror("%s:%d: realloc (%zu * %zu) overflows size_t",
                       file, line, nmemb, size);
    }
    void * p = realloc(ptr, nmemb * size);
    if (unlikely(!p && nmemb * size > 0))
        moocore_perror("%s:%d: realloc (%zu * %zu) failed",
                       file, line, nmemb, size);
    return p;
}

#define MOOCORE_MALLOC(NMEMB, TYPE) moocore_malloc((NMEMB), sizeof(TYPE), __FILE__, __LINE__)
#define MOOCORE_CALLOC(NMEMB, TYPE) moocore_calloc((NMEMB), sizeof(TYPE), __FILE__, __LINE__)
#define MOOCORE_REALLOC(PTR, NMEMB, TYPE) moocore_realloc((PTR), (NMEMB), sizeof(TYPE), __FILE__, __LINE__)

#include "maxminclamp.h"

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
# define DEBUG1_PRINT(...) DEBUG1(fprintf(stderr,  __VA_ARGS__))
# define DEBUG2_PRINT(...) DEBUG2(fprintf(stderr,  __VA_ARGS__))
#else
# define DEBUG1_PRINT(...) DEBUG1(Rprintf( __VA_ARGS__))
# define DEBUG2_PRINT(...) DEBUG2(Rprintf( __VA_ARGS__))
#endif

#define DEBUG2_FUNPRINT(...)                    \
    do { DEBUG2_PRINT ("%s(): ", __FUNCTION__); \
         DEBUG2_PRINT (__VA_ARGS__); } while(0)

#if DEBUG >= 1
#define ASSERT_OR_DO(COND, X) do{ if (!(COND)) { X;} assert(COND); } while(0)
#else
#define ASSERT_OR_DO(COND, X) while(0){ if (!(COND)) { X;} assert(COND); }
#endif

/* This is deprecated. See https://www.gnu.org/software/libc/manual/html_node/Heap-Consistency-Checking.html
#if DEBUG >= 1
#ifndef MALLOC_CHECK_
#define MALLOC_CHECK_ 3
#endif
#endif
*/

#ifndef ignore_unused_result
#define ignore_unused_result(X)  do { if (X){}} while (0);
#endif

#ifdef __cplusplus
#define STATIC_CAST(TYPE,OP) (static_cast<TYPE>(OP))
#else
#define STATIC_CAST(TYPE,OP) ((TYPE)(OP))
#endif

static inline void
printf_point(const char * prefix, const double * p, dimension_t dim,
             const char * suffix)
{
    fprintf(stderr, "%s%g", prefix, p[0]);
    for (dimension_t d = 1; d < dim; d++)
        fprintf(stderr, " %g", p[d]);
    fprintf(stderr, "%s", suffix);
}

#include <math.h> // sqrt()

_attr_pure_func static inline double
euclidean_norm(const double * restrict w, dimension_t dim)
{
    ASSUME(dim >= 2);
    double norm = (w[0] * w[0]) + (w[1] * w[1]);
    for (dimension_t k = 2; k < dim; k++)
        norm += w[k] * w[k];
    ASSUME(norm >= 0); // GCC is not able to infer this.
    return sqrt(norm);
}

#endif 	    /* !MOOCORE_COMMON_H_ */
