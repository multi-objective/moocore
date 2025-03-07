#ifndef GCC_ATTRIBUTES
#define GCC_ATTRIBUTES

/* When passing the -Wunused flag, entities that are unused by the program may
   be diagnosed. The _attr_maybe_unused attribute can be used to silence such
   diagnostics when the entity cannot be removed. For instance, a local
   variable may exist solely for use in an assert() statement, which makes the
   local variable unused when NDEBUG is defined.

   The attribute may be applied to the declaration of a class, a typedef, a
   variable, a function or method, a function parameter, an enumeration, an
   enumerator, a non-static data member, or a label. */
#if defined(__GNUC__) || defined(__clang__ ) || defined(__ICC)
#  define _attr_maybe_unused __attribute__ ((unused))
#else
#  define _attr_maybe_unused
#  define __attribute__(x) /* Elide __attribute__ */
#endif

/* Many functions have no effects except the return value and their
   return value depends only on the parameters and/or global
   variables. Such a function can be subject to common subexpression
   elimination and loop optimization just as an arithmetic operator
   would be.

   Some of common examples of pure functions are strlen or
   memcmp. Interesting non-pure functions are functions with infinite
   loops or those depending on volatile memory or other system
   resource, that may change between two consecutive calls (such as
   feof in a multithreading environment).

   Note that a function that has pointer arguments and examines the data
   pointed to must not be declared const if the pointed-to data might change
   between successive invocations of the function. In general, since a function
   cannot distinguish data that might change from data that cannot, const
   functions should never take pointer or, in C++, reference arguments.  */
#if defined(__GNUC__) || defined(__clang__ ) || defined(__ICC)
#  define _attr_const_func __attribute__((const))
#else
#  define _attr_const_func
#endif

#if defined(__GNUC__) || defined(__clang__) || defined(__ICC)
    #define _attr_aligned(x) __attribute__ ((aligned (x)))
#elif defined(_MSC_VER)
    #define _attr_aligned(x) __declspec(align(x))
#else
    #define _attr_aligned(x)
#endif

/* FIXME: does this handle C++? */
#ifndef __pure_func
# define __pure_func	__attribute__((__pure__))
#endif
/* Many functions do not examine any values except their arguments,
   and have no effects except the return value. Basically this is just
   slightly more strict class than the pure attribute below, since
   function is not allowed to read global memory.

   Note that a function that has pointer arguments and examines the
   data pointed to must not be declared const. Likewise, a function
   that calls a non-const function usually must not be const. It does
   not make sense for a const function to return void.  */
#ifndef __noreturn
# define __noreturn	__attribute__((__noreturn__))
#endif
/* The noreturn keyword tells the compiler to assume that function
   cannot return. It can then optimize without regard to what would
   happen if fatal ever did return. This makes slightly better
   code. More importantly, it helps avoid spurious warnings of
   uninitialized variables. */
#ifndef __malloc
# define __malloc	__attribute__((__malloc__))
#endif
/* The malloc attribute is used to tell the compiler that a function
   may be treated as if any non-NULL pointer it returns cannot alias
   any other pointer valid when the function returns. This will often
   improve optimization. Standard functions with this property include
   malloc and calloc. realloc-like functions have this property as
   long as the old pointer is never referred to (including comparing
   it to the new pointer) after the function returns a non-NULL
   value.  */
#ifndef __must_check
# define __must_check	__attribute__((__warn_unused_result__))
#endif
/* The warn_unused_result attribute causes a warning to be emitted if
   a caller of the function with this attribute does not use its
   return value.  */
#ifndef __deprecated
# define __deprecated	__attribute__((__deprecated__))
#endif
/* The deprecated attribute results in a warning if the function is
   used anywhere in the source file.  */
#ifndef __used
# define __used		__attribute__((__used__))
#endif
/* FIXME: add the explanation from the GCC documentation to each attribute. */
#ifndef __packed
# define __packed	__attribute__((__packed__))
#endif
/* FIXME: add the explanation from the GCC documentation to each attribute. */

#if (defined(__GNUC__) && __GNUC__ >= 3) || defined(__clang__)
# define likely(x)	__builtin_expect(!!(x), 1)
# define unlikely(x)	__builtin_expect(!!(x), 0)
#elif (defined(__cplusplus) && __cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
# define likely(x)	(x) [[likely]]
# define unlikely(x)	(x) [[unlikely]]
#else
# define likely(x)	(x)
# define unlikely(x)	(x)
#endif

#ifdef _MSC_VER
# define __force_inline__ __forceinline
#elif defined(__GNUC__) || defined(__clang__)
# define __force_inline__ __attribute__((always_inline)) inline
#elif defined(_MS_VER) || defined(WIN32)
# define __force_inline__ __ForceInline
#else
# define __force_inline__ inline
#endif

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L  // C99
/* "restrict" is a known keyword */
#elif defined(__GNUC__) || defined(__clang__) // GCC or Clang
  #define restrict __restrict__
#elif defined(_MSC_VER) // MSVC
  #define restrict __restrict
#else // Fallback
  #define restrict
#endif

// C++ standard attribute
#ifdef __has_cpp_attribute
#  if __has_cpp_attribute(assume) >= 202207L
#    define INTERNAL_ASSUME(EXPR) [[assume(EXPR)]]
#  endif
#endif
#ifndef INTERNAL_ASSUME
#  if defined(__clang__)
#    define INTERNAL_ASSUME(EXPR) __builtin_assume(EXPR)
#  elif defined(_MSC_VER)
#    define INTERNAL_ASSUME(EXPR) __assume(EXPR)
#  elif defined(__GNUC__)
#    if __GNUC__ >= 13
#      define INTERNAL_ASSUME(EXPR) __attribute__((__assume__(EXPR)))
#    endif
#  endif
#endif
#ifndef INTERNAL_ASSUME
#  define INTERNAL_ASSUME(EXPR)
#endif

/* Allow to redefine assert, for example, for R packages */
#ifndef assert
#include <assert.h>
#endif
#define ASSUME(EXPR) do { assert(EXPR); INTERNAL_ASSUME(EXPR); } while(0)

/* unreachable() is sometimes defined by stddef.h */
#ifndef unreachable
# if defined(__GNUC__) || defined(__clang__)
#   define unreachable() __builtin_unreachable()
# elif defined(_MSC_VER) // MSVC
#   define unreachable() __assume(0)
# else
#   include <stdlib.h>
#   define unreachable() do { assert(0); abort(); } while(0)
# endif
#endif


#endif /* GCC_ATTRIBUTES */
