#include "common.h"

#ifdef __USE_GNU
extern char *program_invocation_short_name;
#else
char *program_invocation_short_name;
#endif

void fatal_error(const char *format,...)
{
    va_list ap;
    fprintf(stderr, "%s: fatal error: ", program_invocation_short_name);
    va_start(ap,format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

void errprintf(const char *format,...)
{
    va_list ap;
    fprintf(stderr, "%s: error: ", program_invocation_short_name);
    va_start(ap,format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

void warnprintf(const char *format,...)
{
    va_list ap;
    fprintf(stderr, "%s: warning: ", program_invocation_short_name);
    va_start(ap,format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}
