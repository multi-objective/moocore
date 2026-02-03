#ifndef  LIBMOOCORE_CONFIG_H_
# define LIBMOOCORE_CONFIG_H_

#ifndef R_PACKAGE
# if (defined(_WIN32) || defined(__CYGWIN__)) && !(defined(__MINGW__) || defined(__MINGW32__) || defined(__MINGW64__))
#  ifndef MOOCORE_STATIC_LIB
#   ifdef MOOCORE_SHARED_LIB
#    define MOOCORE_API extern __declspec(dllexport)
#   else
#    define MOOCORE_API extern __declspec(dllimport)
#   endif
#  endif
# elif defined(MOOCORE_SHARED_LIB) && defined(__GNUC__) && __GNUC__ >= 4
#  define MOOCORE_API extern __attribute__((visibility("default")))
# endif
#endif

#ifndef MOOCORE_API
# define MOOCORE_API extern
#endif

#ifdef	__cplusplus
# define BEGIN_C_DECLS extern "C" {
# define END_C_DECLS }
#else
# define BEGIN_C_DECLS // empty
# define END_C_DECLS // empty
#endif

#include "config.h"
#include "gcc_attribs.h"

#endif // ! LIBMOOCORE_CONFIG_H_
