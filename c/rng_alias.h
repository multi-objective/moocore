/******************************************************************************

  O(1)-per-sample discrete distribution sampler built on Walker-Vose's
  algorithm for the alias method.

  Given n outcomes with (possibly unnormalized) weights p[0..n-1], the
  Walker-Vose algorithm builds a table in O(n) time that then supports drawing
  a weighted-random outcome in O(1) time.

  The main interface is:

   rng_alias_sampler_t *
   rng_alias_sampler_new(const double * probabilities, uint32_t n);

   void rng_alias_sampler_free(rng_alias_sampler_t * sampler);

   uint32_t rng_alias_sampler_choose(rng_state * rng,
                                     const rng_alias_sampler_t * sampler);

  Based on Keith Schwarz's Java reference implementation:

     https://www.keithschwarz.com/interesting/code/?dir=alias-method

  but with many significant changes. In particular, this implementation uses
  two 32-bit random values.  It could be even faster if we had a 64-bits RNG.

  See also:

   https://en.wikipedia.org/wiki/Alias_method
   https://jugit.fz-juelich.de/mlz/ransampl
   https://www.keithschwarz.com/darts-dice-coins/

*****************************************************************************/
#ifndef RNG_ALIAS_SAMPLER_H
#define RNG_ALIAS_SAMPLER_H

#include <math.h>
#include <float.h>
#include "rng.h"

/**
   One row of the alias table. Kept as a single struct (rather than two
   parallel arrays) so a sample only ever touches one cache line.
*/
typedef struct rng_alias_entry {
    uint32_t cutoff;
    uint32_t alias; // Outcome to return when the coin flip above fails.
} rng_alias_entry_t;

typedef struct rng_alias_sampler {
    uint32_t n;
    rng_alias_entry_t table[]; // Flexible array.
} rng_alias_sampler_t;

#define RNG_ALIAS_RESIDUAL_TOL (64.0 * DBL_EPSILON)

static inline double
rng_alias_clean_scaled(double q)
{
    if (q < 0.0 && q > -RNG_ALIAS_RESIDUAL_TOL)
        return 0.0;

    if (q < 1.0 && q > 1.0 - RNG_ALIAS_RESIDUAL_TOL)
        return 1.0;

    if (q > 1.0 && q < 1.0 + RNG_ALIAS_RESIDUAL_TOL)
        return 1.0;

    return q;
}

static inline uint32_t
rng_alias_cutoff(double q)
{
    if (q <= 0.0)
        return 0;

    double x = q * 0x1p32; // 2^32
    /* Prevent conversion of an out-of-range floating-point value to uint32_t
      if rounding produces a value >= 2^32 - 1.  */
    return (x >= (double)UINT32_MAX) ?  UINT32_MAX : (uint32_t)x;
}

static inline bool
size_max_mul_overflows(size_t a, size_t b)
{
    return b != 0 && a > SIZE_MAX / b;
}

/**
   Construct an alias sampler using Vose's algorithm.

   The caller retains ownership of probabilities.

   probabilities do not need to sum up to 1, but must be >= 0.
*/
static inline rng_alias_sampler_t *
rng_alias_sampler_new(const double * probabilities, uint32_t n)
{
    if (probabilities == NULL || n == 0)
        return NULL;

    double sum = kahan_sum_of_vector_double(probabilities, n);
    if (sum <= 0 || !is_finite(sum))
        return NULL;

    if (size_max_mul_overflows(n, sizeof(rng_alias_entry_t))
        || size_max_mul_overflows(n, sizeof(double))) {
        return NULL;
    }

    uint32_t small_end = 0, large_begin = n;
    /* Temporary Vose double stack:
         [0, small_end)    small entries
         [large_begin, n)  large entries
    */
    uint32_t * stack = (uint32_t *) malloc(n * sizeof(*stack));
    double * scaled = (double *) malloc(n * sizeof(*scaled));
    rng_alias_sampler_t * sampler = malloc(sizeof(*sampler) + n * sizeof(rng_alias_entry_t));
    if (stack == NULL || sampler == NULL || scaled == NULL) {
        free(stack);
        free(scaled);
        free(sampler);
        return NULL;
    }
    sampler->n = n;

    for (uint32_t i = 0; i < n; i++) {
        double p = probabilities[i];
        if (p < 0 || !is_finite(p)) {
            free(stack);
            free(scaled);
            free(sampler);
            return NULL;
        }
        double q = (p / sum) * (double)n;
        q = rng_alias_clean_scaled(q);
        scaled[i] = q;

        if (q < 1.0)
            stack[small_end++] = i;
        else
            stack[--large_begin] = i;
    }

    while (small_end != 0 && large_begin != n) {
        uint32_t small_index = stack[--small_end];
        uint32_t large_index = stack[large_begin++];
        double q = scaled[small_index];

        sampler->table[small_index].cutoff = rng_alias_cutoff(q);
        sampler->table[small_index].alias = large_index;
        scaled[large_index] += q - 1.0;
        scaled[large_index] = rng_alias_clean_scaled(scaled[large_index]);

        if (scaled[large_index] < 1.0)
            stack[small_end++] = large_index;
        else
            stack[--large_begin] = large_index;
    }

    while (large_begin != n) {
        uint32_t i = stack[large_begin++];
        sampler->table[i].cutoff = UINT32_MAX;
        sampler->table[i].alias = i;
    }

    while (small_end != 0) {
        uint32_t i = stack[--small_end];
        sampler->table[i].cutoff = UINT32_MAX;
        sampler->table[i].alias = i;
    }

    free(stack);
    free(scaled);
    return sampler;
}


static inline void
rng_alias_sampler_free(rng_alias_sampler_t * sampler)
{
    free(sampler);
}


/**
   Sample an integer between [0, sampler->n).

   It consumes two 32-bits random values:

    1. One for unbiased column selection.
    2. One for the fixed-point alias decision.

    Column selection may very rarely consume another 32-bits value when
    rejection is required.

    No floating-point operations are performed.
*/
static inline uint32_t
rng_alias_sampler_choose(rng_state * rng, const rng_alias_sampler_t * sampler)
{
    assert(rng != NULL && sampler != NULL);
    uint32_t column = rng_uniform_u32_ubound(rng, sampler->n);
    uint32_t u = mt19937_next32(rng);
    const rng_alias_entry_t * entry = sampler->table + column;
    return u < entry->cutoff ? column : entry->alias;
}

#endif /* RNG_ALIAS_SAMPLER_H */
