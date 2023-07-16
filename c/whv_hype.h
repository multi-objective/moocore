#ifndef WHV_HYPE_H
#define WHV_HYPE_H
#include <stdlib.h>

enum hype_sample_dist_type { HYPE_DIST_UNIFORM, HYPE_DIST_EXPONENTIAL, HYPE_DIST_GAUSSIAN };

typedef struct hype_sample_dist hype_sample_dist;

hype_sample_dist * hype_dist_unif_new(unsigned long seed);
hype_sample_dist * hype_dist_exp_new(double mu, unsigned long seed);
hype_sample_dist * hype_dist_gaussian_new(const double *mu, unsigned long int seed);
void hype_dist_free(hype_sample_dist * d);

enum hype_sample_dist_type hype_dist_get_type(const hype_sample_dist *);

double
whv_hype_estimate(const double *points, size_t n,
                  const double *ideal, const double *ref,
                  hype_sample_dist * dist, size_t nsamples);
#endif
