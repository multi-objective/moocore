#ifndef _MAXMINCLAMP_H_
#define _MAXMINCLAMP_H_

/* ---------- STATIC_ASSERT ---------- */
// Prefer _Static_assert (C11). Otherwise use typedef negative-size fallback.
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
  #define STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)
#else
  #define STATIC_ASSERT_GLUE(a, b) a##b
  #define STATIC_ASSERT_JOIN(a, b) STATIC_ASSERT_GLUE(a, b)
  // Unique name using __LINE__
  #define STATIC_ASSERT(cond, msg) \
      typedef char STATIC_ASSERT_JOIN(static_assertion_at_line_, __LINE__)[(cond) ? 1 : -1]
#endif

#if (defined(__GNUC__) && __GNUC__ >= 3) || defined(__clang__)
#define STATIC_ASSERT_TYPES_COMPATIBLE(x,y, msg)                               \
    STATIC_ASSERT(__builtin_types_compatible_p(__typeof__(x), __typeof__(y)), msg)
#else
#define STATIC_ASSERT_TYPES_COMPATIBLE(x,y, msg) ((void) (&x == &y))
#endif

#if (defined(__GNUC__) && __GNUC__ >= 3) || defined(__clang__)
#define __cmp_op_MIN <
#define __cmp_op_MAX >
#define __cmp(op, x, y) ((x) __cmp_op_##op (y) ? (x) : (y))
#define __careful_cmp(op, x, y) __extension__({                                \
            __auto_type _x__ = (x);                                            \
            __auto_type _y__ = (y);                                            \
            STATIC_ASSERT_TYPES_COMPATIBLE(                                    \
                _x__, _y__, #op ": incompatible argument types");              \
            __cmp(op, _x__, _y__);                                             \
        })

#define MAX(x,y) __careful_cmp(MAX, x, y)
#define MIN(x,y) __careful_cmp(MIN, x, y)
#define CLAMP(x, xmin, xmax) __extension__({                                   \
            __auto_type _x__ = (x);                                            \
            __auto_type _xmin__ = (xmin);                                      \
            __auto_type _xmax__ = (xmax);                                      \
            STATIC_ASSERT_TYPES_COMPATIBLE(                                    \
                _x__, _xmin__,                                                 \
                "CLAMP: incompatible argument types 'x' and 'xmin'");          \
            STATIC_ASSERT_TYPES_COMPATIBLE(                                    \
                _x__, _xmax__,                                                 \
                "CLAMP: incompatible argument types 'x' and 'xmax'");          \
            _x__ <= _xmin__ ? _xmin__ : _x__ >= _xmax__ ? _xmax__ : _x__; })

#else
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define CLAMP(x, xmin, xmax) (MAX((xim), (MIN((x), (xmax)))))
#endif

#define SWAP(x,y) do {                                                         \
        __typeof__(x) _tmp__ = (x);                                            \
        STATIC_ASSERT_TYPES_COMPATIBLE(x, y, "SWAP: incompatible argument types"); \
        x = y;                                                                 \
        y = _tmp__; } while(0)

#endif // _MAXMINCLAMP_H_
