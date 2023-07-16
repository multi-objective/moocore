/* Functions that behave differently for library code and command-line code. */

#include "common.h"

#ifndef R_PACKAGE
void fatal_error(const char *format,...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

void errprintf(const char *format,...)
{
    va_list ap;
    fprintf(stderr, "error: ");
    va_start(ap,format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

void warnprintf(const char *format,...)
{
    va_list ap;
    fprintf(stderr, "warning: ");
    va_start(ap,format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

#endif // R_PACKAGE
