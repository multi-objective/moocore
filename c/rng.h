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

static inline uint32_t
rng_uint32(rng_state * rng, uint32_t low, uint32_t high)
{
    assert(rng != NULL);
    if (low >= high)
        return low;
    return low + (mt19937_next32(rng) % (high - low));
}

/**
   Returns a random value within [0, n - 1] based on the given CDF.

   The CDF values should be non-decreasing and within [0,1].

   Requires O(log n).
*/
static inline uint32_t
rng_random_wheel_uint32(rng_state * rng, const double * cdf, uint32_t n)
{
    // Check the CDF is correct.
    DEBUG1(for (uint32_t j = 1; j < n; j++) assert(cdf[j-1] <= cdf[j]));
    DEBUG1(for (uint32_t j = 0; j < n; j++) assert(0 <= cdf[j] && cdf[j] <= 1));

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
