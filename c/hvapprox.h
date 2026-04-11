/*************************************************************************

 hv_approx.h

*************************************************************************/
#ifndef HV_APPROX_H_
#define HV_APPROX_H_

#include <stdbool.h>
#include <stdint.h> // uint_fast32_t, uint32_t
#include "libmoocore-config.h"

// C++ needs to know that types and declarations are C, not C++.
BEGIN_C_DECLS

MOOCORE_API double hv_approx_normal(
    const double * restrict data, size_t npoints, dimension_t nobjs,
    const double * restrict ref, const boolvec * restrict maximise,
    uint_fast32_t nsamples, uint32_t random_seed);
/**
   Jingda Deng, Qingfu Zhang (2019). "Approximating Hypervolume and Hypervolume
   Contributions Using Polar Coordinate". IEEE Transactions on Evolutionary
   Computation, 23(5), 913–918. doi:10.1109/tevc.2019.2895108 .
*/
MOOCORE_API double hv_approx_hua_wang(
    const double * restrict data, size_t npoints, dimension_t nobjs,
    const double * restrict ref, const boolvec * restrict maximise,
    uint_fast32_t nsamples);
/**
   Manuel López-Ibáñez (2026). "Approximating the Hypervolume Indicator using
   Fast Quasi-Random Low-Discrepancy Sequences".  Proceeding of the Genetic and
   Evolutionary Computation Conference, GECCO 2026.
   https://doi.org/10.1145/3795095.3805198
*/
MOOCORE_API double hv_approx_rphi_fang_wang_plus(
    const double * restrict data, size_t npoints, dimension_t nobjs,
    const double * restrict ref, const boolvec * restrict maximise,
    uint_fast32_t nsamples);

END_C_DECLS
#endif // HV_APPROX_H_
