#ifndef _MOOCORE_CONFIG_H_
#define _MOOCORE_CONFIG_H_

#if (defined(__MINGW__) || defined(__MINGW32__) || defined(__MINGW64__))       \
    && !defined(_UCRT) && !defined(__USE_MINGW_ANSI_STDIO)
#if !defined(__cplusplus) && !defined(_ISOC99_SOURCE)
#define _ISOC99_SOURCE
#endif
#define __USE_MINGW_ANSI_STDIO 1
#endif

#endif // _MOOCORE_CONFIG_H_
