#ifndef RNG_H_
#define RNG_H_

#ifdef _WIN64
#include <intrin.h>
#endif

#include "pcg64/pcg64.h"

typedef pcg64_random_t rng_state;

#include "common.h"

static inline rng_state *
rng_new(uint32_t seed)
{
    rng_state * rng = MOOCORE_MALLOC(1, rng_state);
    pcg64_srandom_r(rng, PCG_128BIT_CONSTANT(0, seed),
                    PCG_128BIT_CONSTANT(0, 54));
    return rng;
}

static inline void
rng_free(rng_state * rng)
{
    free(rng);
}

/* Returns a value between [0, 1) */
static inline double
rng_random(rng_state * rng)
{
    return (double)(pcg64_random_r(rng) >> 11) * (1.0 / 9007199254740992.0);
}

static inline double
rng_uniform(rng_state * rng, double low, double high)
{
    assert(rng != NULL);
    if (low >= high)
        return low;
    return low + (high - low) * rng_random(rng);
}


double rng_standard_normal(rng_state *rng);
void rng_bivariate_normal_fill(rng_state * rng,
                               double mu1, double mu2,
                               double sigma1, double sigma2, double rho,
                               double *out, int n);
#endif // RNG_H_
