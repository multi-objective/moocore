#ifndef GCC_ATTRIBUTES
#define GCC_ATTRIBUTES

/* FIXME: does this handle C++? */
#ifndef __pure_func
# define __pure_func	__attribute__((__pure__))
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
   feof in a multithreading environment).  */
#ifndef __const_func
# define __const_func	__attribute__((__const__))
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
#ifndef _no_warn_unused
# define _no_warn_unused __attribute__((__unused__))
#endif
/* This attribute, attached to a function, means that the function is meant to be possibly unused. GCC does not produce a warning for this function. */
#ifndef __packed
# define __packed	__attribute__((__packed__))
#endif
/* FIXME: add the explanation from the GCC documentation to each attribute. */

#if __GNUC__ >= 3
# define likely(x)	__builtin_expect (!!(x), 1)
/* FIXME: add the explanation from the GCC documentation to each attribute. */
# define unlikely(x)	__builtin_expect (!!(x), 0)
/* FIXME: add the explanation from the GCC documentation to each attribute. */
#else
# define  __attribute__(x) /* If we're not using GNU C, elide __attribute__ */
# define likely(x)	(x)
# define unlikely(x)	(x)
#endif

#endif
