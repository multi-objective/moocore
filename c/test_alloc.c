/* Unit tests for the allocation wrappers in common.h.

   Without arguments, check moocore_mul_overflow() and that the wrappers
   return usable memory.

   With one argument ("malloc", "calloc" or "realloc"), call that wrapper with
   a size that overflows size_t.  The wrapper must print an error and exit.
   The 'unittest' target in the Makefile checks the error message.
*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"

static int failures = 0;

#define CHECK(EXPR) do {                                                       \
        if (!(EXPR)) {                                                         \
            fprintf(stderr, "%s:%d: check failed: %s\n",                       \
                    __FILE__, __LINE__, #EXPR);                                \
            failures++;                                                        \
        }                                                                      \
    } while (0)

static int
overflow(const char * fn)
{
    // n * sizeof(double) does not fit in size_t.
    const size_t n = SIZE_MAX / 2 + 1;
    void * p = NULL;
    if (strcmp(fn, "malloc") == 0)
        p = MOOCORE_MALLOC(n, double);
    else if (strcmp(fn, "calloc") == 0)
        p = MOOCORE_CALLOC(n, double);
    else if (strcmp(fn, "realloc") == 0)
        p = MOOCORE_REALLOC(NULL, n, double);
    free(p);
    // Only reached if the overflow was not detected.
    fprintf(stderr, "%s: overflow not detected\n", fn);
    return EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
    if (argc > 1)
        return overflow(argv[1]);

    // sqrt(SIZE_MAX + 1)
    const size_t half = (size_t)1 << (sizeof(size_t) * 4);
    CHECK(!moocore_mul_overflow(0, 0));
    CHECK(!moocore_mul_overflow(0, SIZE_MAX));
    CHECK(!moocore_mul_overflow(SIZE_MAX, 0));
    CHECK(!moocore_mul_overflow(1, SIZE_MAX));
    CHECK(!moocore_mul_overflow(SIZE_MAX, 1));
    CHECK(!moocore_mul_overflow(half - 1, half - 1));
    CHECK(!moocore_mul_overflow(half, half - 1));
    CHECK(!moocore_mul_overflow(2, SIZE_MAX / 2));
    CHECK(moocore_mul_overflow(2, SIZE_MAX / 2 + 1));
    CHECK(moocore_mul_overflow(SIZE_MAX / 2 + 1, 2));
    CHECK(moocore_mul_overflow(half, half));
    CHECK(moocore_mul_overflow(SIZE_MAX, SIZE_MAX));

    int * a = MOOCORE_MALLOC(10, int);
    for (int i = 0; i < 10; i++)
        a[i] = i;
    a = MOOCORE_REALLOC(a, 1000, int);
    for (int i = 0; i < 10; i++)
        CHECK(a[i] == i);
    a[999] = 999;
    free(a);

    double * z = MOOCORE_CALLOC(10, double);
    for (int i = 0; i < 10; i++)
        CHECK(z[i] == 0.0);
    free(z);

    // malloc(0) may return NULL, which is not an allocation failure.
    free(MOOCORE_MALLOC(0, double));
    free(MOOCORE_CALLOC(0, double));

    if (failures > 0) {
        fprintf(stderr, "test_alloc: %d checks failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("test_alloc: all checks passed\n");
    return EXIT_SUCCESS;
}
