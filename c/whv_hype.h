#ifndef WHV_HYPE_H
#define WHV_HYPE_H
#include <stdint.h>

double whv_hype_unif(const double *points, int npoints,
                     const double *ideal, const double *ref,
                     int nsamples, uint32_t seed);
double whv_hype_expo(const double *points, int npoints,
                     const double *ideal, const double *ref,
                     int nsamples, uint32_t seed, double mu);
double whv_hype_gaus(const double *points, int npoints,
                     const double *ideal, const double *ref,
                     int nsamples, uint32_t seed, const double *mu);

#endif
