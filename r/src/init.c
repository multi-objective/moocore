#include <stdlib.h> // for NULL
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

// Supports 1-10 arguments
#define VA_NARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define VA_NARGS(...) VA_NARGS_IMPL(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define DECLARE_CALL(RET_TYPE, NAME, ...)                                      \
    extern RET_TYPE NAME(__VA_ARGS__);
#include "init.h"
#undef DECLARE_CALL


#define DECLARE_CALL(RET_TYPE, NAME, ...)                                      \
    {#NAME, (DL_FUNC) &NAME, VA_NARGS(__VA_ARGS__)},

static const R_CallMethodDef CallEntries[] = {
    #include "init.h"
    {NULL, NULL, 0}
};
#undef DECLARE_CALL

void R_init_moocore(DllInfo *dll)
{
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
