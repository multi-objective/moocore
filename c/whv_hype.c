#include "whv_hype.h"
#undef DEBUG
#define DEBUG 1
#include "common.h"
#include "nondominated.h"
#include <float.h>
#include <math.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

static gsl_rng *
rng_new(unsigned long int seed)
{
    gsl_rng * rng = gsl_rng_alloc (gsl_rng_taus2);
    gsl_rng_set(rng, seed);
    return rng;
}
static double
rng_double(gsl_rng * rng, double low, double high)
{
    eaf_assert(rng != NULL);
    return(low + (gsl_rng_uniform(rng)*high));
}

struct hype_sample_dist {
    enum hype_sample_dist_type type;
    gsl_rng * rng;
    double * lower;
    double * upper;
    double * mu;
};

enum hype_sample_dist_type
hype_dist_get_type(const hype_sample_dist * d)
{
    eaf_assert(d != NULL);
    return d->type;
}

static hype_sample_dist *
hype_dist_new(unsigned long int seed)
{
    hype_sample_dist * dist = malloc(sizeof(hype_sample_dist));
    dist->rng = rng_new(seed);
    const int nobj = 2;
    dist->lower = malloc(sizeof(double) * nobj);
    dist->upper = malloc(sizeof(double) * nobj);
    for (int i = 0; i < nobj; i++) {
        dist->lower[i] = 0;
        dist->upper[i] = 1;
    }
    dist->mu = NULL;
    return dist;
}

hype_sample_dist *
hype_dist_gaussian_new(const double *mu, unsigned long int seed)
{
    hype_sample_dist *dist = hype_dist_new(seed);
    dist->type = HYPE_DIST_GAUSSIAN;
    const int nobj = 2;
    dist->mu = malloc(sizeof(double) * nobj);
    memcpy(dist->mu, mu, sizeof(double) * nobj);
    return dist;
}

hype_sample_dist *
hype_dist_exp_new(double mu, unsigned long int seed)
{
    hype_sample_dist *dist = hype_dist_new(seed);
    dist->type = HYPE_DIST_EXPONENTIAL;
    dist->mu = malloc(sizeof(double) * 1);
    dist->mu[0] = mu;
    return dist;
}

hype_sample_dist *
hype_dist_unif_new(unsigned long int seed)
{
    hype_sample_dist *dist = hype_dist_new(seed);
    dist->type = HYPE_DIST_UNIFORM;
    return dist;
}

void
hype_dist_free(hype_sample_dist * d)
{
    gsl_rng_free(d->rng);
    if (d->mu) free(d->mu);
    free(d->lower);
    free(d->upper);
    free(d);
}

static double *
exp_dist_sample(hype_sample_dist * dist, size_t nsamples)
{
    const int nobj = 2;
    const double *lower = dist->lower;
    const double *upper = dist->upper;

    double * samples = malloc(sizeof(double) * nsamples * nobj);
    size_t n = 0.5 * nsamples;
    double mu = dist->mu[0];
    for (size_t i = 0; i < n; i++) {
        double x = rng_double(dist->rng, 0, 1);
        samples[i * nobj + 0] = lower[0] - mu * log(x);
        x = rng_double(dist->rng, 0, 1);
        samples[i * nobj + 1] = lower[1] + x * (upper[1] - lower[1]);
    }
    for (size_t i = n; i < nsamples; i++) {
        double x = rng_double(dist->rng, 0, 1);
        samples[i * nobj + 0] = lower[0] + x * (upper[0] - lower[0]);
        x = rng_double(dist->rng, 0, 1);
        samples[i * nobj + 1] = lower[1] - mu * log(x);
    }
    return samples;
}

static double *
gaussian_dist_sample(hype_sample_dist * dist, size_t nsamples)
{
    const int nobj = 2;

    double * samples = malloc(sizeof(double) * nsamples * nobj);
    double sigma_x = 0.25;
    double sigma_y = 0.25;
    for (size_t i = 0; i < nsamples; i++) {
        double x, y;
        gsl_ran_bivariate_gaussian(dist->rng, sigma_x, sigma_y, /*rho=*/1.0,
                                   &x, &y);
        /* FIXME: do we need to use the truncated distribution?
        samples[i * nobj + 0] = CLAMP(dist->mu[0] + x, 0.0, 1.0);
        samples[i * nobj + 1] = CLAMP(dist->mu[1] + y, 0.0, 1.0);
        */
        samples[i * nobj + 0] = dist->mu[0] + x;
        samples[i * nobj + 1] = dist->mu[1] + y;
        //fprintf(stderr, "x = %g (%g), y = %g (%g)\n",
        //        x, samples[i * nobj + 0], y, samples[i * nobj + 1]);
    }
    return samples;
}

static double *
uniform_dist_sample(hype_sample_dist * dist, size_t nsamples)
{
    const int nobj = 2;
    const double *lower = dist->lower;
    const double *upper = dist->upper;

    double * samples = malloc(sizeof(double) * nsamples * nobj);
    for (size_t i = 0; i < nsamples; i++) {
        for (int d = 0; d < nobj; d++) {
            double x = rng_double(dist->rng, 0, 1);
            samples[i * nobj + d] = x * (upper[d] - lower[d]);
        }
        //printf("sample: { %g, %g }\n", samples[i * nobj + 0], samples[i * nobj + 1]);
    }
    return samples;
}


typedef double *(*hype_sample_fn)(hype_sample_dist *, size_t);

static hype_sample_fn
hype_dist_get_sample_fn(const hype_sample_dist * d)
{
    switch(d->type) {
      case HYPE_DIST_UNIFORM:
          return uniform_dist_sample;

      case HYPE_DIST_EXPONENTIAL:
          return exp_dist_sample;

      case HYPE_DIST_GAUSSIAN:
          return gaussian_dist_sample;

      default:
          fatal_error("%s:%d: unknown hype_sample_dist type: %d\n", __FILE__, __LINE__, d->type);
          return NULL;
    }
}


static double
estimate_whv(const double *points, size_t npoints,
             const double * samples, size_t nsamples)
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
    for (size_t s = 0; s < nsamples; s++) {
        const double *sample = samples + s * nobj;
        // compute amount of dominators in p for each sample:
        for (size_t j = 0; j < npoints; j++) {
            bool dom = true;
            const double *p = points + j * nobj;
            for (int d = 0; d < nobj; d++) {
                if (sample[d] < p[d]) {
                    dom = false;
                    break;
                }
            }
            if (dom) dominated[s]++;
        }
        // sum up alpha values of each dominated sample:
        for (size_t j = 0; j < npoints; j++) {
            bool dom = true;
            const double *p = points + j * nobj;
            for (int d = 0; d < nobj; d++) {
                if (sample[d] < p[d]) {
                    dom = false;
                    break;
                }
            }
            if (dom) {
                eaf_assert(dominated[s] > 0);
                whv += 1.0 / dominated[s];
                //fprintf(stderr, "whv = %g\n", whv);
            }
        }
    }
    free(dominated);
    //free(alpha);
    return whv;
}

static double
calculate_volume_between_points(const double *p1, const double * p2, size_t dim)
{
    double volume = 1.0;
    for (size_t k = 0; k < dim; k++) volume *= (p2[k] - p1[k]);
    return volume;
}

static void
normalise01_inplace(double *points, size_t dim, size_t npoints,
                    const double *lbound, const double *ubound)
{
    signed char * minmax = malloc(sizeof(signed char) * dim);
    memset(minmax, -1, sizeof(signed char) * dim);
    normalise(points, dim, npoints, minmax, /*agree=*/-1, 0.0, 1.0,
              lbound, ubound);
    free(minmax);
}

static double *
normalise01(const double *points, size_t dim, size_t npoints,
            const double *lbound, const double *ubound)
{
    double * points2 = malloc(sizeof(double) * dim * npoints);
    memcpy(points2, points, sizeof(double) * dim * npoints);
    normalise01_inplace(points2, dim, npoints, lbound, ubound);
    return points2;
}

double
whv_hype_estimate(const double *points, size_t npoints,
                  const double *ideal, const double *ref,
                  hype_sample_dist * dist, size_t nsamples)
{
    hype_sample_fn create_samples = hype_dist_get_sample_fn(dist);

    if (!create_samples) return NAN;
    const int nobj = 2;
    /* FIXME: this modifies mu, it would be better to keep mu and use a copy */
    if (dist->type == HYPE_DIST_GAUSSIAN) {
        normalise01_inplace(dist->mu, nobj, 1, ideal, ref);
        //fprintf(stderr, "mu = %g, %g\n", dist->mu[0], dist->mu[1]);
    }

    const double * samples = create_samples(dist, nsamples);
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
