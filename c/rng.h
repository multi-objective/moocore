#include "mt19937/mt19937.h"

typedef mt19937_state rng_state;

#include "common.h"

static inline rng_state *
rng_new(uint32_t seed)
{
    rng_state * rng = malloc(sizeof(rng_state));
    mt19937_seed(rng, seed);
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
    return mt19937_next_double(rng);
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
