#ifndef MOOCORE_COMMON_H_
#define MOOCORE_COMMON_H_

#include "config.h"
#ifdef R_PACKAGE
# define R_NO_REMAP
# include <R.h>
# define assert(EXP)                                                  \
     do {                                                             \
         if (unlikely(!(EXP))) {                                      \
             Rf_error("error: assertion failed: '%s' at %s:%d", #EXP, \
                      __FILE__, __LINE__);                            \
         }                                                            \
     } while (0)
# include "gcc_attribs.h"
# define fatal_error(...) Rf_error(__VA_ARGS__)
# define moocore_perror(...) Rf_error(__VA_ARGS__)
# define errprintf Rf_error
# define warnprintf Rf_warning
#else
# include <stdarg.h>
# include <stdlib.h>
# include <assert.h>
# include <stdio.h>
# include "gcc_attribs.h"
__noreturn _attr_maybe_unused void fatal_error(const char * format, ...)
    ATTRIBUTE_FORMAT_PRINTF(1, 2);
void errprintf(const char * format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);
void warnprintf(const char * format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);
# define moocore_perror(...)                  \
     do {                                     \
         char buffer[1024] = "";              \
         snprintf(buffer, 1024, __VA_ARGS__); \
         perror(buffer);                      \
         exit(EXIT_FAILURE);                  \
     } while (0)
#endif // R_PACKAGE

#define MOOCORE_STRINGIFY(name) #name
#define MOOCORE_STRINGIFY_MACRO(macro) MOOCORE_STRINGIFY(macro)

#include <stdbool.h>
static inline void *
moocore_malloc(size_t nmemb, size_t size, const char * file, int line)
{
    // FIXME: Check multiplication overflow.
    // https://github.com/bminor/glibc/blob/e64a1e81aadf6c401174ac9471ced0f0125c2912/malloc/malloc.c#L3709
    // https://github.com/libressl/openbsd/blob/master/src/lib/libc/stdlib/reallocarray.c
    // https://github.com/python/cpython/blob/89df62c12093bfa079860a93032468ebece3774d/Include/internal/mimalloc/mimalloc/internal.h#L323
    void * p = malloc(nmemb * size);
    if (unlikely(!p))
        moocore_perror("%s:%d: malloc (%zu * %zu) failed", file, line, nmemb,
                       size);
    return p;
}

#define MOOCORE_MALLOC(NMEMB, TYPE) \
    moocore_malloc((NMEMB), sizeof(TYPE), __FILE__, __LINE__)

#if (defined(__GNUC__) && __GNUC__ >= 3) || defined(__clang__)
# define __cmp_op_min <
# define __cmp_op_max >
# define __cmp(op, x, y) ((x)__cmp_op_##op(y) ? (x) : (y))
# define __careful_cmp(op, x, y) \
     __extension__({             \
      __auto_type _x__ = (x);    \
      __auto_type _y__ = (y);    \
      (void)(&_x__ == &_y__);    \
      __cmp(op, _x__, _y__);     \
     })

# define MAX(x, y) __careful_cmp(max, x, y)
# define MIN(x, y) __careful_cmp(min, x, y)

# define CLAMP(x, xmin, xmax)                                       \
     __extension__({                                                \
      __auto_type _x__ = (x);                                       \
      __typeof__(_x__) _xmin__ = (xmin);                            \
      __typeof__(_x__) _xmax__ = (xmax);                            \
      _x__ <= _xmin__ ? _xmin__ : _x__ >= _xmax__ ? _xmax__ : _x__; \
     })
#else
# define MAX(x, y) ((x) > (y) ? (x) : (y))
# define MIN(x, y) ((x) < (y) ? (x) : (y))
# define CLAMP(x, xmin, xmax) (MAX((xim), (MIN((x), (xmax)))))
#endif


#define DEBUG_DO(X) \
    do {            \
        X;          \
    } while (0)
#define DEBUG_NOT_DO(X) \
    while (0) {         \
        X;              \
    }

#if DEBUG >= 1
# define DEBUG1(X) DEBUG_DO(X)
#else
# define DEBUG1(X) DEBUG_NOT_DO(X)
#endif

#if DEBUG >= 2
# define DEBUG2(X) DEBUG_DO(X)
#else
# define DEBUG2(X) DEBUG_NOT_DO(X)
#endif

#if DEBUG >= 3
# define DEBUG3(X) DEBUG_DO(X)
#else
# define DEBUG3(X) DEBUG_NOT_DO(X)
#endif

#if DEBUG >= 4
# define DEBUG4(X) DEBUG_DO(X)
#else
# define DEBUG4(X) DEBUG_NOT_DO(X)
#endif

#ifndef R_PACKAGE
# define DEBUG1_PRINT(...) DEBUG1(fprintf(stderr, __VA_ARGS__))
# define DEBUG2_PRINT(...) DEBUG2(fprintf(stderr, __VA_ARGS__))
#else
# define DEBUG1_PRINT(...) DEBUG1(Rprintf(__VA_ARGS__))
# define DEBUG2_PRINT(...) DEBUG2(Rprintf(__VA_ARGS__))
#endif

#define DEBUG2_FUNPRINT(...)                  \
    do {                                      \
        DEBUG2_PRINT("%s(): ", __FUNCTION__); \
        DEBUG2_PRINT(__VA_ARGS__);            \
    } while (0)

#if DEBUG >= 1
# define ASSERT_OR_DO(COND, X) \
     do {                      \
         if (!(COND)) {        \
             X;                \
         }                     \
         assert(COND);         \
     } while (0)
#else
# define ASSERT_OR_DO(COND, X) \
     while (0) {               \
         if (!(COND)) {        \
             X;                \
         }                     \
         assert(COND);         \
     }
#endif

/* This is deprecated. See https://www.gnu.org/software/libc/manual/html_node/Heap-Consistency-Checking.html
#if DEBUG >= 1
#ifndef MALLOC_CHECK_
#define MALLOC_CHECK_ 3
#endif
#endif
*/

#ifndef ignore_unused_result
# define ignore_unused_result(X) \
     do {                        \
         if (X) {                \
         }                       \
     } while (0);
#endif

#ifdef __cplusplus
# define STATIC_CAST(TYPE, OP) (static_cast<TYPE>(OP))
#else
# define STATIC_CAST(TYPE, OP) ((TYPE)(OP))
#endif

/* FIXME: Move this to a better place: matrix.h ? */
/* FIXME: Measure if this is faster than the R implementation of t()  */
static inline void
matrix_transpose_double(double * dst, const double * src, const size_t nrows,
                        const size_t ncols)
{
    size_t j, i, pos = 0;
    for (j = 0; j < ncols; j++) {
        for (i = 0; i < nrows; i++) {
            dst[pos] = src[j + i * ncols];
            pos++;
        }
    }
}

#include <stdint.h>
typedef uint_fast8_t dimension_t;

/* FIXME: Move this to nondominated.h */
enum objs_agree_t { AGREE_MINIMISE = -1, AGREE_NONE = 0, AGREE_MAXIMISE = 1 };

static inline enum objs_agree_t
check_all_minimize_maximize(const signed char * restrict minmax,
                            dimension_t dim)
{
    bool all_minimize = true, all_maximize = true;
    for (dimension_t d = 0; d < dim; d++) {
        if (minmax[d] < 0) {
            all_maximize = false;
        } else if (minmax[d] > 0) {
            all_minimize = false;
        } else {
            all_minimize = false;
            all_maximize = false;
            break;
        }
    }
    assert(!all_maximize || !all_minimize);
    if (all_minimize) return AGREE_MINIMISE;
    if (all_maximize) return AGREE_MAXIMISE;
    return AGREE_NONE;
}

/* Convert from bool vector to minmax vector.  */
static inline signed char *
minmax_from_bool(dimension_t nobj, const bool * restrict maximise)
{
    signed char * minmax = malloc(sizeof(signed char) * nobj);
    for (dimension_t k = 0; k < nobj; k++) {
        minmax[k] = (maximise[k]) ? AGREE_MAXIMISE : AGREE_MINIMISE;
    }
    return minmax;
}

static inline bool *
new_bool_maximise(dimension_t nobj, bool maximise_all)
{
    ASSUME(nobj > 0);
    ASSUME(nobj < 128);
    bool * maximise = malloc(sizeof(bool) * nobj);
    for (dimension_t k = 0; k < nobj; k++) maximise[k] = maximise_all;
    return maximise;
}

static inline const signed char *
default_minmax(dimension_t nobj, signed char default_value)
{
    ASSUME(nobj > 0);
    ASSUME(default_value == AGREE_MINIMISE || default_value == AGREE_MAXIMISE);
    signed char * minmax = malloc(sizeof(signed char) * nobj);
    for (dimension_t i = 0; i < nobj; i++) minmax[i] = default_value;
    return minmax;
}

static inline const signed char *
minmax_minimise(dimension_t nobj)
{
    return default_minmax(nobj, AGREE_MINIMISE);
}

static inline const signed char *
minmax_maximise(dimension_t nobj)
{
    return default_minmax(nobj, AGREE_MAXIMISE);
}

#endif /* !MOOCORE_COMMON_H_ */
