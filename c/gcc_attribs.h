#ifndef GCC_ATTRIBUTES
#define GCC_ATTRIBUTES

#if !defined(__GNUC__) && !defined(__clang__ ) && !defined(__ICC)
#  define __attribute__(x) /* Elide __attribute__ */
#endif

/* When passing the -Wunused flag, entities that are unused by the program may
   be diagnosed. The _attr_maybe_unused attribute can be used to silence such
   diagnostics when the entity cannot be removed. For instance, a local
   variable may exist solely for use in an assert() statement, which makes the
   local variable unused when NDEBUG is defined.

   The attribute may be applied to the declaration of a class, a typedef, a
   variable, a function or method, a function parameter, an enumeration, an
   enumerator, a non-static data member, or a label. */
#ifndef _attr_maybe_unused
#  define _attr_maybe_unused  __attribute__((unused))
#endif

/* Calls to functions whose return value is not affected by changes to the
   observable state of the program and that have no observable effects on such
   state other than to return a value may lend themselves to optimizations such
   as common subexpression elimination. Declaring such functions with the const
   attribute allows GCC to avoid emitting some calls in repeated invocations of
   the function with the same argument values.

   The const attribute imposes greater restrictions on a function's definition
   than the similar pure attribute.

   A function that has pointer arguments and examines the data pointed to must
   not be declared const if the pointed-to data might change between successive
   invocations of the function. In general, since a function cannot distinguish
   data that might change from data that cannot, const functions should never
   take pointer or, in C++, reference arguments.  */
#if defined(__GNUC__) || defined(__clang__ ) || defined(__ICC)
#  define _attr_const_func __attribute__((const))
#else
#  define _attr_const_func
#endif

/* The pure attribute prohibits a function from modifying the state of the
   program that is observable by means other than inspecting the function's
   return value. However, functions declared with the pure attribute can safely
   read any non-volatile objects, and modify the value of objects in a way that
   does not affect their return value or the observable state of the
   program.

   Some common examples of pure functions are strlen or memcmp. Interesting
   non-pure functions are functions with infinite loops or those depending on
   volatile memory or other system resource, that may change between
   consecutive calls (such as the standard C feof function in a multithreading
   environment).

   The pure attribute imposes similar but looser restrictions on a function’s
   definition than the const attribute: pure allows the function to read any
   non-volatile memory, even if it changes in between successive invocations of
   the function.  */
#ifndef __pure_func
# define __pure_func __attribute__((__pure__))
#endif

/* The noreturn keyword tells the compiler to assume that function cannot
   return. It can then optimize without regard to what would happen if fatal
   ever did return. This makes slightly better code. More importantly, it helps
   avoid spurious warnings of uninitialized variables. */
#ifndef __noreturn
# if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202301L
// gcc 15-, LLVM clang 19- and Apple clang 17
#  define __noreturn [[noreturn]]
# elif defined(_MSC_VER) && _MSC_VER >= 1200
#  define __noreturn __declspec(noreturn)
# else
#  define __noreturn __attribute__((__noreturn__))
# endif
#endif

/* The malloc attribute is used to tell the compiler that a function
   may be treated as if any non-NULL pointer it returns cannot alias
   any other pointer valid when the function returns. This will often
   improve optimization. Standard functions with this property include
   malloc and calloc. realloc-like functions have this property as
   long as the old pointer is never referred to (including comparing
   it to the new pointer) after the function returns a non-NULL
   value.  */
#ifndef _attr_malloc
# define _attr_malloc	__attribute__((__malloc__))
#endif

/* The warn_unused_result attribute causes a warning to be emitted if
   a caller of the function with this attribute does not use its
   return value.  */
#ifndef __must_check
# define __must_check	__attribute__((__warn_unused_result__))
#endif

/* The deprecated attribute results in a warning if the function is
   used anywhere in the source file.  */
#ifndef __deprecated
# define __deprecated	__attribute__((__deprecated__))
#endif

/* FIXME: add the explanation from the GCC documentation to each attribute. */
#ifndef __used
# define __used		__attribute__((__used__))
#endif
#ifndef __packed
# define __packed	__attribute__((__packed__))
#endif

#if defined(__GNUC__) || defined(__clang__) || defined(__ICC)
# define _attr_aligned(x) __attribute__((aligned (x)))
#elif defined(_MSC_VER)
# define _attr_aligned(x) __declspec(align(x))
#else
# define _attr_aligned(x)
#endif


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
# define restrict __restrict__
#elif defined(_MSC_VER) // MSVC
# define restrict __restrict
#else // Fallback
# define restrict
#endif

/*
   See https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-format-function-attribute
   The correct format archetype to use for MinGW varies.
*/
#ifdef __MINGW_PRINTF_FORMAT // Defined by MinGW in stdio.h
#define ATTRIBUTE_FORMAT_PRINTF_ARCHETYPE __MINGW_PRINTF_FORMAT
#elif defined(__MINGW__) || defined(__MINGW32__) || defined(__MINGW64__)
// Otherwise, __gnu_printf__ is a good default for MinGW.
#define ATTRIBUTE_FORMAT_PRINTF_ARCHETYPE __gnu_printf__
#else
#define ATTRIBUTE_FORMAT_PRINTF_ARCHETYPE __printf__
#endif

#define ATTRIBUTE_FORMAT_PRINTF(STRING_INDEX, FIRST_TO_CHECK)                  \
    __attribute__((__format__(ATTRIBUTE_FORMAT_PRINTF_ARCHETYPE, STRING_INDEX, FIRST_TO_CHECK)))

#ifndef INTERNAL_ASSUME
// C++ standard attribute
# if defined(__has_cpp_attribute)
#  if __has_cpp_attribute(assume) >= 202207L
#   define INTERNAL_ASSUME(EXPR) [[assume(EXPR)]]
#  endif
# endif
#endif

#ifndef INTERNAL_ASSUME
// C++ standard attribute
# if defined(__clang__)
#  define INTERNAL_ASSUME(EXPR) __builtin_assume(EXPR)
# elif defined(_MSC_VER) || defined(__ICC)
#  define INTERNAL_ASSUME(EXPR) __assume(EXPR)
# elif defined(__GNUC__) && __GNUC__ >= 13
#  define INTERNAL_ASSUME(EXPR) __attribute__((__assume__(EXPR)))
# else
#  define INTERNAL_ASSUME(EXPR) ((void)0)
# endif
#endif


/* Allow to redefine assert, for example, for R packages */
#ifndef assert
#include <assert.h>
#endif
#define ASSUME(EXPR) do { assert(EXPR); INTERNAL_ASSUME(EXPR); } while(0)

/* unreachable() is sometimes defined by stddef.h */
#ifndef unreachable
# if defined(__GNUC__) || defined(__clang__)
#  define unreachable() __builtin_unreachable()
# elif defined(_MSC_VER) // MSVC
#  define unreachable() __assume(0)
# else
#  include <stdlib.h>
#  define unreachable() do { assert(0); abort(); } while(0)
# endif
#endif


#define PRAGMA(m_token_sequence) _Pragma(#m_token_sequence)

#ifdef __ICC
#define PRAGMA_ASSUME_NO_VECTOR_DEPENDENCY PRAGMA(ivdep)
#elif defined(__GNUC__)  && __GNUC__ >= 5
#define PRAGMA_ASSUME_NO_VECTOR_DEPENDENCY PRAGMA(GCC ivdep)
#else
#define PRAGMA_ASSUME_NO_VECTOR_DEPENDENCY
#endif

// Earlier versions miscompile with this attribute.
#if defined(__GNUC__)  && __GNUC__ >= 13 && defined(__OPTIMIZE__)
#define _attr_optimize_finite_math                                             \
    __attribute__((optimize("no-signed-zeros", "finite-math-only")))
#else
#define _attr_optimize_finite_math /* nothing */
#endif

#endif /* GCC_ATTRIBUTES */
