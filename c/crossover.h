/*************************************************************************

 crossover.h

 ---------------------------------------------------------------------

                          Copyright (C) 2026
        Bhupinder Saini <bhupinder.saini2.71@gmail.com>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ----------------------------------------------------------------------


*************************************************************************/
#ifndef CROSSOVER_H_
#define CROSSOVER_H_

#include <stdbool.h>
#include <stdint.h> // uint32_t
#include "libmoocore-config.h"

// C++ needs to know that types and declarations are C, not C++.
BEGIN_C_DECLS

/*
  All matrices are stored by rows, one decision vector per row, with NVAR
  variables each.  OFFSPRING must be allocated by the caller.

  Functions that take PARENTS (N rows) cross over rows 2i and 2i+1.  If N is
  odd, the last row is crossed over with row 0.  Operators that produce two
  offspring per pair write them to rows 2i and 2i+1 of OFFSPRING (N rows),
  dropping the second offspring of the last pair when N is odd.
  blend_alpha_crossover() produces one offspring per pair, so OFFSPRING has
  (N + 1) / 2 rows.

  Functions that take POPULATION (N rows) produce one offspring per row index in
  TARGETS (NTARGETS values in [0, N)), so OFFSPRING has NTARGETS rows.  They
  choose the other parents at random from POPULATION.

  LOWER and UPPER give the bounds of each variable.  Use -INFINITY and
  INFINITY for unbounded variables.
*/

MOOCORE_API void uniform_crossover(
    double * restrict offspring, const double * restrict parents,
    size_t n, size_t nvar, double swap_prob, double pair_prob, uint32_t seed);

MOOCORE_API void single_point_crossover(
    double * restrict offspring, const double * restrict parents,
    size_t n, size_t nvar, uint32_t seed);

MOOCORE_API void blend_alpha_crossover(
    double * restrict offspring, const double * restrict parents,
    size_t n, size_t nvar, const double * restrict lower,
    const double * restrict upper, double alpha, uint32_t seed);

MOOCORE_API void single_arithmetic_crossover(
    double * restrict offspring, const double * restrict parents,
    size_t n, size_t nvar, double pair_prob, uint32_t seed);

MOOCORE_API void local_crossover(
    double * restrict offspring, const double * restrict parents,
    size_t n, size_t nvar, uint32_t seed);

MOOCORE_API void bounded_exponential_crossover(
    double * restrict offspring, const double * restrict parents,
    size_t n, size_t nvar, const double * restrict lower,
    const double * restrict upper, double scale, double pair_prob,
    double swap_prob, uint32_t seed);

MOOCORE_API void de_rand_1_bin_crossover(
    double * restrict offspring, const double * restrict population,
    size_t n, size_t nvar, const size_t * restrict targets, size_t ntargets,
    const double * restrict lower, const double * restrict upper,
    double scaling_factor, double crossover_rate, uint32_t seed);

MOOCORE_API void de_current_1_crossover(
    double * restrict offspring, const double * restrict population,
    size_t n, size_t nvar, const size_t * restrict targets, size_t ntargets,
    const double * restrict lower, const double * restrict upper,
    double scaling_factor, double crossover_rate, uint32_t seed);

MOOCORE_API void parent_centric_crossover(
    double * restrict offspring, const double * restrict population,
    size_t n, size_t nvar, const size_t * restrict targets, size_t ntargets,
    const double * restrict lower, const double * restrict upper,
    double sigma_zeta, double sigma_eta, uint32_t seed);

END_C_DECLS

#endif // CROSSOVER_H_
