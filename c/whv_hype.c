#include "whv_hype.h"
#undef DEBUG
#define DEBUG 1
#include "common.h"
#include "nondominated.h"
#include "rng.h"
#include <float.h>
#include <math.h>

enum hype_sample_dist_type { HYPE_DIST_UNIFORM, HYPE_DIST_EXPONENTIAL, HYPE_DIST_GAUSSIAN };

typedef struct hype_sample_dist hype_sample_dist;

typedef double *(*hype_sample_fn)(hype_sample_dist *, int);

struct hype_sample_dist {
    enum hype_sample_dist_type type;
    rng_state * rng;
    double * lower;
    double * range;
    double * mu;
    hype_sample_fn create_samples;
};

enum hype_sample_dist_type
hype_dist_get_type(const hype_sample_dist * d)
{
    assert(d != NULL);
    return d->type;
}

static double *
gaussian_dist_sample(hype_sample_dist * dist, int nsamples)
{
    const int nobj = 2;
    double * samples = malloc(sizeof(double) * nsamples * nobj);
    /* FIXME: Dimo's paper uses a t vector instead of rho */
    double sigma_x = 0.25;
    double sigma_y = 0.25;
    rng_bivariate_normal_fill(dist->rng,
                              dist->mu[0], dist->mu[1],
                              sigma_x, sigma_y, /*rho=*/1.0,
                              samples, nsamples);
    /* FIXME: do we need to use the truncated distribution?
       samples[i * nobj + 0] = CLAMP(dist->mu[0] + x, 0.0, 1.0);
       samples[i * nobj + 1] = CLAMP(dist->mu[1] + y, 0.0, 1.0);
    */
    return samples;
}

static double *
exp_dist_sample(hype_sample_dist * dist, int nsamples)
{
    const int nobj = 2;
    const double *lower = dist->lower;
    const double *range = dist->range;

    double * samples = malloc(sizeof(double) * nsamples * nobj);
    int n = (int)(0.5 * nsamples);
    double mu = dist->mu[0];
    rng_state * rng = dist->rng;
    for (int i = 0; i < n; i++) {
        double x = rng_random(rng);
        assert(x > 0);
        samples[i * nobj + 0] = lower[0] - mu * log(x);
        x = rng_random(rng);
        samples[i * nobj + 1] = lower[1] + x * range[1];
    }
    for (int i = n; i < nsamples; i++) {
        double x = rng_random(rng);
        samples[i * nobj + 0] = lower[0] + x * range[0];
        x = rng_random(rng);
        assert(x > 0);
        samples[i * nobj + 1] = lower[1] - mu * log(x);
    }
    return samples;
}

static double *
uniform_dist_sample(hype_sample_dist * dist, int nsamples)
{
    const int nobj = 2;
    const double *lower = dist->lower;
    const double *range = dist->range;
    rng_state * rng = dist->rng;
    double * samples = malloc(sizeof(double) * nsamples * nobj);
    for (int i = 0; i < nsamples; i++) {
        for (int d = 0; d < nobj; d++) {
            samples[i * nobj + d] = lower[d] + rng_random(rng) * range[d];
        }
        //printf("sample: { %g, %g }\n", samples[i * nobj + 0], samples[i * nobj + 1]);
    }
    return samples;
}

static hype_sample_dist *
hype_dist_new(uint32_t seed)
{
    hype_sample_dist * dist = malloc(sizeof(hype_sample_dist));
    dist->rng = rng_new(seed);
    const int nobj = 2;
    dist->lower = malloc(sizeof(double) * nobj);
    dist->range = malloc(sizeof(double) * nobj);
    for (int i = 0; i < nobj; i++) {
        dist->lower[i] = 0;
        dist->range[i] = 1;
    }
    dist->mu = NULL;
    return dist;
}

static hype_sample_dist *
hype_dist_gaussian_new(uint32_t seed, const double *mu)
{
    hype_sample_dist *dist = hype_dist_new(seed);
    dist->type = HYPE_DIST_GAUSSIAN;
    const int nobj = 2;
    dist->mu = malloc(sizeof(double) * nobj);
    memcpy(dist->mu, mu, sizeof(double) * nobj);
    dist->create_samples = gaussian_dist_sample;
    return dist;
}

static hype_sample_dist *
hype_dist_exp_new(uint32_t seed, double mu)
{
    hype_sample_dist *dist = hype_dist_new(seed);
    dist->type = HYPE_DIST_EXPONENTIAL;
    dist->mu = malloc(sizeof(double) * 1);
    dist->mu[0] = mu;
    dist->create_samples = exp_dist_sample;
    return dist;
}

static hype_sample_dist *
hype_dist_unif_new(uint32_t seed)
{
    hype_sample_dist *dist = hype_dist_new(seed);
    dist->type = HYPE_DIST_UNIFORM;
    dist->create_samples = uniform_dist_sample;
    return dist;
}

void
hype_dist_free(hype_sample_dist * d)
{
    rng_free(d->rng);
    if (d->mu) free(d->mu);
    free(d->lower);
    free(d->range);
    free(d);
}

static double
estimate_whv(const double *points, int npoints,
             const double * samples, int nsamples)
{
    const int nobj = 2;
    /* // compute alpha factor of HypE fitness: */
    /* double * alpha = malloc(npoints * sizeof(double)); */
    /* for (int i = 1; i <= npoints; i++) { */
    /*     alpha[i - 1] = 1.0 / i; */
    /* } */
    double whv = 0.0;
    // compute amount of dominators in p for each sample:
    unsigned int * dominated = calloc(nsamples, sizeof(unsigned int));
    bool * is_dominator = malloc(npoints * sizeof(bool));
    for (int s = 0; s < nsamples; s++) {
        const double *sample = samples + s * nobj;
        // compute amount of dominators in p for each sample:
        for (int j = 0; j < npoints; j++) {
            bool dom = true;
            const double *p = points + j * nobj;
            for (int d = 0; d < nobj; d++) {
                if (sample[d] < p[d]) {
                    dom = false;
                    break;
                }
            }
            if (dom) dominated[s]++;
            is_dominator[j] = dom;
        }
        // sum up alpha values of each dominated sample:
        for (int j = 0; j < npoints; j++) {
            if (is_dominator[j]) {
                assert(dominated[s] > 0);
                whv += 1.0 / dominated[s];
                //fprintf(stderr, "whv = %g\n", whv);
            }
        }
    }
    free(dominated);
    free(is_dominator);
    //free(alpha);
    return whv;
}

static double
calculate_volume_between_points(const double *p1, const double * p2, int dim)
{
    double volume = 1.0;
    for (int k = 0; k < dim; k++) volume *= (p2[k] - p1[k]);
    return volume;
}

static void
normalise01_inplace(double *points, int dim, int npoints,
                    const double *lbound, const double *ubound)
{
    signed char * minmax = malloc(sizeof(signed char) * dim);
    memset(minmax, -1, sizeof(signed char) * dim);
    normalise(points, dim, npoints, minmax, /*agree=*/-1, 0.0, 1.0,
              lbound, ubound);
    free(minmax);
}

static double *
normalise01(const double *points, int dim, int npoints,
            const double *lbound, const double *ubound)
{
    double * points2 = malloc(sizeof(double) * dim * npoints);
    memcpy(points2, points, sizeof(double) * dim * npoints);
    normalise01_inplace(points2, dim, npoints, lbound, ubound);
    return points2;
}


static double
whv_hype_sample(const double *points, int npoints,
                const double *ideal, const double *ref,
                int nsamples, hype_sample_dist * dist)
{
    const int nobj = 2;
    const double * samples = dist->create_samples(dist, nsamples);
    const double * points2 = normalise01(points, nobj, npoints, ideal, ref);
    double whv = estimate_whv(points2, npoints, samples, nsamples);
    free((void *)samples);
    free((void *)points2);
    /* Eq 18 */
    //fprintf(stderr, "whv = %g\n", whv);
    whv *= calculate_volume_between_points(ideal, ref, nobj) / nsamples;
    //fprintf(stderr, "whv = %g\n", whv);
    return whv;
}

double
whv_hype_estimate(const double *points, int npoints,
                  const double *ideal, const double *ref,
                  hype_sample_dist * dist, int nsamples)
{
    const int nobj = 2;
    /* FIXME: this modifies mu, it would be better to keep mu and use a copy */
    if (dist->type == HYPE_DIST_GAUSSIAN) {
        normalise01_inplace(dist->mu, nobj, 1, ideal, ref);
        //fprintf(stderr, "mu = %g, %g\n", dist->mu[0], dist->mu[1]);
    }
    return whv_hype_sample(points, npoints, ideal, ref, nsamples, dist);
}

double
whv_hype_unif(const double *points, int npoints,
              const double *ideal, const double *ref,
              int nsamples, uint32_t seed)
{
    hype_sample_dist * dist = hype_dist_unif_new(seed);
    double whv = whv_hype_sample(points, npoints, ideal, ref, nsamples, dist);
    hype_dist_free(dist);
    return whv;
}

double
whv_hype_expo(const double *points, int npoints,
              const double *ideal, const double *ref,
              int nsamples, uint32_t seed, double mu)
{
    hype_sample_dist * dist = hype_dist_exp_new(seed, mu);
    double whv = whv_hype_sample(points, npoints, ideal, ref, nsamples, dist);
    hype_dist_free(dist);
    return whv;
}

double
whv_hype_gaus(const double *points, int npoints,
              const double *ideal, const double *ref,
              int nsamples, uint32_t seed, const double *mu)
{
    hype_sample_dist * dist = hype_dist_gaussian_new(seed, mu);
    const int nobj = 2;
    /* FIXME: this modifies mu, it would be better to keep mu and use a copy */
    normalise01_inplace(dist->mu, nobj, 1, ideal, ref);
    double whv = whv_hype_sample(points, npoints, ideal, ref, nsamples, dist);
    hype_dist_free(dist);
    return whv;
}
