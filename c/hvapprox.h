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

MOOCORE_API double hv_approx_hua_wang(
    const double * restrict data, size_t npoints, dimension_t nobjs,
    const double * restrict ref, const bool * restrict maximise,
    uint_fast32_t nsamples);
MOOCORE_API double hv_approx_normal(
    const double * restrict data, size_t npoints, dimension_t nobjs,
    const double * restrict ref, const bool * restrict maximise,
    uint_fast32_t nsamples, uint32_t random_seed);

MOOCORE_API double hv_approx_rphi_fang_wang_plus(
    const double * restrict data, size_t npoints, dimension_t nobjs,
    const double * restrict ref, const bool * restrict maximise,
    uint_fast32_t nsamples);

END_C_DECLS
#endif // HV_APPROX_H_
