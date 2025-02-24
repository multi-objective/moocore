/*************************************************************************

 hv_approx.h

*************************************************************************/
#ifndef HV_APPROX_H_
#define HV_APPROX_H_

#include <stdbool.h>
#include <stdint.h> // uint_fast32_t, uint32_t

#ifdef __cplusplus
extern "C" {
#endif

double hv_approx_hua_wang(const double * data, int nobjs, int npoints,
                          const double * ref, const bool * maximise,
                          uint_fast32_t nsamples);
double hv_approx_normal(const double * data, int nobjs, int npoints,
                        const double * ref, const bool * maximise,
                        uint_fast32_t nsamples, uint32_t random_seed);
#ifdef __cplusplus
}
#endif
#endif
