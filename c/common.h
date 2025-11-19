#ifndef MOOCORE_COMMON_H_
#define MOOCORE_COMMON_H_

#include "config.h"
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

#define MOOCORE_STRINGIFY(name) #name
#define MOOCORE_STRINGIFY_MACRO(macro) MOOCORE_STRINGIFY(macro)

#include <stdbool.h>
static inline void *
moocore_malloc(size_t nmemb, size_t size, const char *file, int line)
{
    // FIXME: Check multiplication overflow.
    // https://github.com/bminor/glibc/blob/e64a1e81aadf6c401174ac9471ced0f0125c2912/malloc/malloc.c#L3709
    // https://github.com/libressl/openbsd/blob/master/src/lib/libc/stdlib/reallocarray.c
    // https://github.com/python/cpython/blob/89df62c12093bfa079860a93032468ebece3774d/Include/internal/mimalloc/mimalloc/internal.h#L323
    void * p = malloc(nmemb * size);
    if (unlikely(!p))
        moocore_perror("%s:%d: malloc (%zu * %zu) failed",
                       file, line, nmemb, size);
    return p;
}

#define MOOCORE_MALLOC(NMEMB, TYPE) moocore_malloc((NMEMB), sizeof(TYPE), __FILE__, __LINE__)

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

#include <stdint.h>
typedef uint_fast8_t dimension_t;

/* FIXME: Move this to a better place: matrix.h ? */
static inline void
matrix_transpose_double(double * restrict dst, const double * restrict src,
                        const size_t nrows, const size_t ncols)
{
    ASSUME(nrows < SIZE_MAX/2 && ncols < SIZE_MAX/2);
    if (nrows <= 0 || ncols <= 0)
        return;

    const size_t len_1 = (nrows * ncols) - 1;
    size_t i = 0, j = 0;
    for (; j <= len_1; i++, j += nrows)
        dst[j] = src[i];

    for (; i <= len_1; i++, j += nrows) {
	    if (j > len_1) j -= len_1;
	    dst[j] = src[i];
	}
}

#endif 	    /* !MOOCORE_COMMON_H_ */
