#ifndef RNG_H
#define RNG_H

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

/**
   Returns a uniformly distributed 32-bits integer in [0, n).

   Lemire, Daniel. "Fast Random Integer Generation in an Interval", ACM
   Transactions on Modeling and Computer Simulation (TOMACS), 29(1):1-12,
   2019. https://doi.org/10.1145/3230636

*/
static inline uint32_t
rng_uniform_u32_ubound(rng_state * rng, uint32_t n)
{
    uint64_t m = ((uint64_t)mt19937_next32(rng)) * n;
    uint32_t leftover = (uint32_t)m;
    if (leftover < n) {
        // t = 2^32 mod n, expressed without requiring a 64-bit 2^32 value.
        const uint32_t t = (uint32_t)(-n) % n;
        while (leftover < t) {
            m = ((uint64_t)mt19937_next32(rng)) * n;
            leftover = (uint32_t)m;
        }
    }
    return (uint32_t)(m >> 32);
}


static inline uint32_t
rng_uniform_u32_bounded(rng_state * rng, uint32_t low, uint32_t high)
{
    assert(rng != NULL);
    if (low >= high)
        return low;

    return low + rng_uniform_u32_ubound(rng, high - low);
}

static inline void
rng_validate_cdf(const double * cdf _attr_maybe_unused, uint32_t n)
{
    assert(n > 0);
    assert(cdf[0] >= 0.0);
    for (uint32_t i = 1; i < n; ++i) {
        assert(cdf[i - 1] <= cdf[i]);
        assert(0.0 <= cdf[i] && cdf[i] <= 1.0);
    }
    assert(cdf[n - 1] == 1.0);
}

/**
   Returns a random value within [0, n - 1] based on the given CDF using the
   roulette-wheel method.

   The CDF values should be non-decreasing and within [0,1].

   Requires O(log n).
*/
static inline uint32_t
rng_random_wheel_uint32(rng_state * rng, const double * cdf, uint32_t n)
{
    DEBUG1(rng_validate_cdf(cdf, n)); // Check the CDF is correct.

    double r = rng_random(rng);
    if (r < cdf[0])
        return 0;
    if (n == 2 || r >= cdf[n - 2])
        return n - 1;

    // Binary search.
    uint32_t low = 1, high = n - 2;
    while (low < high) {
        uint32_t mid = low  + (high - low) / 2;
        if (r < cdf[mid])
            high = mid;
        else
            low = mid + 1;
    }

    DEBUG1(for (uint32_t j = 0; j < low; j++) assert(cdf[j] < r));
    DEBUG1(for (uint32_t j = low; j < n; j++) assert(r <= cdf[j]));
    return low;
}

double rng_standard_normal(rng_state *rng);
void rng_bivariate_normal_fill(rng_state * rng,
                               double mu1, double mu2,
                               double sigma1, double sigma2, double rho,
                               double *out, int n);
#endif /* RNG_H */
