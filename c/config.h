#ifndef _MOOCORE_CONFIG_H_
#define _MOOCORE_CONFIG_H_

#if (defined(__MINGW__) || defined(__MINGW32__) || defined(__MINGW64__))       \
    && !defined(_UCRT) && !defined(__USE_MINGW_ANSI_STDIO)
#if !defined(__cplusplus) && !defined(_ISOC99_SOURCE)
#define _ISOC99_SOURCE
#endif
#define __USE_MINGW_ANSI_STDIO 1
#endif

// Silence deprecation warnings from Clang and MSVC on Windows
#if defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
# define _CRT_SECURE_NO_WARNINGS 1
#endif

#if DEBUG == 0 && !defined(NDEBUG)
# define NDEBUG // Disable assert()
#elif DEBUG > 0 && defined(NDEBUG)
# undef NDEBUG // Enable assert()
#endif

#include <stdint.h>
typedef uint_fast8_t dimension_t;
// Maximum number of objectives is 31. It cannot be larger than 255.
#define MOOCORE_DIMENSION_MAX 31

#endif // _MOOCORE_CONFIG_H_
