/*************************************************************************

 crossover.c

 Crossover operators for real-valued (and, for uniform and single-point
 crossover, any numerical) decision vectors.  See crossover.h for the
 conventions shared by all functions.

 ---------------------------------------------------------------------

                          Copyright (C) 2026
        Bhupinder Saini <bhupinder.saini2.71@gmail.com>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ----------------------------------------------------------------------


*************************************************************************/
#include <stdlib.h>
#include <string.h> // memcpy(), memmove()
#include <math.h>
#include "common.h"
#include "crossover.h"
#include "rng.h"

// parent_centric_crossover() treats an index parent this close to the
// centroid of its three parents as degenerate.
#define PCX_DEGENERATE 1e-12

static inline double
clamp_to_bounds(double x, double lower, double upper)
{
    return fmin(fmax(x, lower), upper);
}

/* Rows I and I + 1 of PARENTS are crossed over and their offspring go to rows
   I and I + 1 of OFFSPRING.  If I + 1 == N, row I is crossed over with row 0
   and its second offspring goes to SCRATCH, so it is discarded.  */
static inline void
get_pair(const double * parents, double * offspring, double * scratch,
         size_t i, size_t n, size_t nvar,
         const double ** p1, const double ** p2, double ** off1, double ** off2)
{
    const bool last = (i + 1 == n);
    *p1 = parents + i * nvar;
    *p2 = parents + (last ? 0 : i + 1) * nvar;
    *off1 = offspring + i * nvar;
    *off2 = last ? scratch : offspring + (i + 1) * nvar;
}

static inline double *
scratch_new(size_t n, size_t nvar)
{
    return (n % 2 == 1) ? MOOCORE_MALLOC(nvar, double) : NULL;
}

// Offspring of a pair that is not crossed over.
static inline void
copy_parents(double * off1, double * off2, const double * p1, const double * p2,
             size_t nvar)
{
    memcpy(off1, p1, nvar * sizeof(*off1));
    memcpy(off2, p2, nvar * sizeof(*off2));
}

/* Draw K distinct indices in [0, N) that also differ from the NEXCLUDED
   indices in EXCLUDED.  EXCLUDED must be sorted in ascending order and have
   room for K more values.  Each draw picks among the indices not yet excluded
   and shifts the result past the excluded ones, so there is no rejection
   loop.  */
static void
sample_distinct(rng_state * rng, size_t n, size_t * excluded, size_t nexcluded,
                size_t k, size_t * chosen)
{
    for (size_t c = 0; c < k; c++, nexcluded++) {
        ASSUME(n > nexcluded);
        size_t d = rng_integer(rng, n - nexcluded);
        size_t pos = 0;
        while (pos < nexcluded && excluded[pos] <= d) {
            d++;
            pos++;
        }
        // Insert D at POS to keep EXCLUDED sorted.
        memmove(excluded + pos + 1, excluded + pos,
                (nexcluded - pos) * sizeof(*excluded));
        excluded[pos] = d;
        chosen[c] = d;
    }
}

void
uniform_crossover(double * restrict offspring, const double * restrict parents,
                  size_t n, size_t nvar, double swap_prob, double pair_prob,
                  uint32_t seed)
{
    ASSUME(n >= 2 && nvar >= 1);
    rng_state * rng = rng_new(seed);
    double * scratch = scratch_new(n, nvar);
    for (size_t i = 0; i < n; i += 2) {
        const double *p1, *p2;
        double *off1, *off2;
        get_pair(parents, offspring, scratch, i, n, nvar,
                 &p1, &p2, &off1, &off2);
        const bool recombine = rng_random(rng) < pair_prob;
        for (size_t j = 0; j < nvar; j++) {
            const bool swap = recombine && rng_random(rng) < swap_prob;
            off1[j] = swap ? p2[j] : p1[j];
            off2[j] = swap ? p1[j] : p2[j];
        }
    }
    free(scratch);
    rng_free(rng);
}

void
single_point_crossover(double * restrict offspring,
                       const double * restrict parents,
                       size_t n, size_t nvar, double pair_prob, uint32_t seed)
{
    ASSUME(n >= 2 && nvar >= 2);
    rng_state * rng = rng_new(seed);
    double * scratch = scratch_new(n, nvar);
    for (size_t i = 0; i < n; i += 2) {
        const double *p1, *p2;
        double *off1, *off2;
        get_pair(parents, offspring, scratch, i, n, nvar,
                 &p1, &p2, &off1, &off2);
        if (!(rng_random(rng) < pair_prob)) {
            copy_parents(off1, off2, p1, p2, nvar);
            continue;
        }
        // Cut point in [1, nvar - 1], so each offspring takes at least one
        // variable from each parent.
        const size_t cut = 1 + rng_integer(rng, nvar - 1);
        memcpy(off1, p1, cut * sizeof(*off1));
        memcpy(off1 + cut, p2 + cut, (nvar - cut) * sizeof(*off1));
        memcpy(off2, p2, cut * sizeof(*off2));
        memcpy(off2 + cut, p1 + cut, (nvar - cut) * sizeof(*off2));
    }
    free(scratch);
    rng_free(rng);
}

void
blend_alpha_crossover(double * restrict offspring,
                      const double * restrict parents,
                      size_t n, size_t nvar, const double * restrict lower,
                      const double * restrict upper, double alpha,
                      double pair_prob, uint32_t seed)
{
    ASSUME(n >= 2 && nvar >= 1);
    ASSUME(alpha >= 0);
    rng_state * rng = rng_new(seed);
    for (size_t i = 0; i < n; i += 2) {
        const double * p1 = parents + i * nvar;
        const double * p2 = parents + ((i + 1 < n) ? i + 1 : 0) * nvar;
        double * off = offspring + (i / 2) * nvar;
        // A pair that is not crossed over yields a copy of its first parent.
        if (!(rng_random(rng) < pair_prob)) {
            memcpy(off, p1, nvar * sizeof(*off));
            continue;
        }
        for (size_t j = 0; j < nvar; j++) {
            const double lo = fmin(p1[j], p2[j]);
            const double hi = fmax(p1[j], p2[j]);
            const double span = hi - lo;
            // Widen [lo, hi] by ALPHA * SPAN on each side, clip it to the
            // bounds and sample uniformly from it.
            const double a = fmax(lo - alpha * span, lower[j]);
            const double b = fmin(hi + alpha * span, upper[j]);
            off[j] = a + rng_random(rng) * (b - a);
        }
    }
    rng_free(rng);
}

void
single_arithmetic_crossover(double * restrict offspring,
                            const double * restrict parents,
                            size_t n, size_t nvar, double pair_prob,
                            uint32_t seed)
{
    ASSUME(n >= 2 && nvar >= 1);
    rng_state * rng = rng_new(seed);
    double * scratch = scratch_new(n, nvar);
    for (size_t i = 0; i < n; i += 2) {
        const double *p1, *p2;
        double *off1, *off2;
        get_pair(parents, offspring, scratch, i, n, nvar,
                 &p1, &p2, &off1, &off2);
        const bool recombine = rng_random(rng) < pair_prob;
        const size_t k = rng_integer(rng, nvar);
        memcpy(off1, p1, nvar * sizeof(*off1));
        memcpy(off2, p2, nvar * sizeof(*off2));
        if (recombine)
            off1[k] = off2[k] = 0.5 * (p1[k] + p2[k]);
    }
    free(scratch);
    rng_free(rng);
}

void
local_crossover(double * restrict offspring, const double * restrict parents,
                size_t n, size_t nvar, double pair_prob, uint32_t seed)
{
    ASSUME(n >= 2 && nvar >= 1);
    rng_state * rng = rng_new(seed);
    double * scratch = scratch_new(n, nvar);
    for (size_t i = 0; i < n; i += 2) {
        const double *p1, *p2;
        double *off1, *off2;
        get_pair(parents, offspring, scratch, i, n, nvar,
                 &p1, &p2, &off1, &off2);
        if (!(rng_random(rng) < pair_prob)) {
            copy_parents(off1, off2, p1, p2, nvar);
            continue;
        }
        for (size_t j = 0; j < nvar; j++) {
            const double a = rng_random(rng);
            off1[j] = a * p1[j] + (1 - a) * p2[j];
            off2[j] = (1 - a) * p1[j] + a * p2[j];
        }
    }
    free(scratch);
    rng_free(rng);
}

/* Offspring value of parent value X.  It moves X by BETA * SPAN.  BETA is
   negative or positive with probability 1/2 each.  On each side, it follows
   an exponential distribution with scale SCALE, truncated at the bound on
   that side, so the offspring stays within [LOWER, UPPER].  U and R are
   uniform in [0, 1).  */
static inline double
bex_offspring(double x, double span, double lower, double upper, double scale,
          double u, double r)
{
    double beta;
    if (r < 0.5) {
        const double e = exp((lower - x) / (scale * span));
        // 1 - U is in (0, 1], which avoids log(0) when LOWER is -INFINITY.
        beta = scale * log(e + (1 - u) * (1 - e));
    } else {
        const double e = exp((x - upper) / (scale * span));
        beta = -scale * log(1 - u * (1 - e));
    }
    // Rounding may step just outside the bounds.
    return clamp_to_bounds(x + beta * span, lower, upper);
}

void
bounded_exponential_crossover(double * restrict offspring,
                              const double * restrict parents,
                              size_t n, size_t nvar,
                              const double * restrict lower,
                              const double * restrict upper, double scale,
                              double swap_prob, double pair_prob,
                              uint32_t seed)
{
    ASSUME(n >= 2 && nvar >= 1);
    ASSUME(scale > 0);
    rng_state * rng = rng_new(seed);
    double * scratch = scratch_new(n, nvar);
    for (size_t i = 0; i < n; i += 2) {
        const double *p1, *p2;
        double *off1, *off2;
        get_pair(parents, offspring, scratch, i, n, nvar,
                 &p1, &p2, &off1, &off2);
        if (!(rng_random(rng) < pair_prob)) {
            copy_parents(off1, off2, p1, p2, nvar);
            continue;
        }
        for (size_t j = 0; j < nvar; j++) {
            // The distance between the parents scales the move of both
            // offspring, whichever parent is smaller.
            const double span = fabs(p2[j] - p1[j]);
            if (span == 0) {
                off1[j] = p1[j];
                off2[j] = p2[j];
                continue;
            }
            // Both offspring use the same random numbers.
            const double u = rng_random(rng);
            const double r = rng_random(rng);
            off1[j] = bex_offspring(p1[j], span, lower[j], upper[j],
                                    scale, u, r);
            off2[j] = bex_offspring(p2[j], span, lower[j], upper[j],
                                    scale, u, r);
            if (swap_prob > 0 && rng_random(rng) < swap_prob) {
                const double tmp = off1[j];
                off1[j] = off2[j];
                off2[j] = tmp;
            }
        }
    }
    free(scratch);
    rng_free(rng);
}

void
de_rand_1_bin_crossover(double * restrict offspring,
                        const double * restrict population,
                        size_t n, size_t nvar, const size_t * restrict targets,
                        size_t ntargets, const double * restrict lower,
                        const double * restrict upper, double scaling_factor,
                        double crossover_rate, uint32_t seed)
{
    ASSUME(n >= 4 && nvar >= 1);
    rng_state * rng = rng_new(seed);
    for (size_t t = 0; t < ntargets; t++) {
        ASSUME(targets[t] < n);
        const double * x = population + targets[t] * nvar;
        size_t excluded[4] = { targets[t] };
        size_t r[3];
        sample_distinct(rng, n, excluded, 1, 3, r);
        const double * x1 = population + r[0] * nvar;
        const double * x2 = population + r[1] * nvar;
        const double * x3 = population + r[2] * nvar;
        // The offspring takes at least the variable JRAND from the mutant.
        const size_t jrand = rng_integer(rng, nvar);
        double * off = offspring + t * nvar;
        for (size_t j = 0; j < nvar; j++) {
            const double v = (j == jrand || rng_random(rng) < crossover_rate)
                ? x1[j] + scaling_factor * (x2[j] - x3[j])
                : x[j];
            off[j] = clamp_to_bounds(v, lower[j], upper[j]);
        }
    }
    rng_free(rng);
}

void
de_current_1_crossover(double * restrict offspring,
                       const double * restrict population,
                       size_t n, size_t nvar, const size_t * restrict targets,
                       size_t ntargets, const double * restrict lower,
                       const double * restrict upper, double scaling_factor,
                       double crossover_rate, uint32_t seed)
{
    ASSUME(n >= 2 && nvar >= 1);
    rng_state * rng = rng_new(seed);
    for (size_t t = 0; t < ntargets; t++) {
        ASSUME(targets[t] < n);
        const double * x = population + targets[t] * nvar;
        // The two donors differ from each other, but either may be the target.
        size_t excluded[2];
        size_t r[2];
        sample_distinct(rng, n, excluded, 0, 2, r);
        const double * x1 = population + r[0] * nvar;
        const double * x2 = population + r[1] * nvar;
        double * off = offspring + t * nvar;
        for (size_t j = 0; j < nvar; j++) {
            const double v = (rng_random(rng) < crossover_rate)
                ? x[j] + scaling_factor * (x1[j] - x2[j])
                : x[j];
            off[j] = clamp_to_bounds(v, lower[j], upper[j]);
        }
    }
    rng_free(rng);
}

void
parent_centric_crossover(double * restrict offspring,
                         const double * restrict population,
                         size_t n, size_t nvar,
                         const size_t * restrict targets, size_t ntargets,
                         const double * restrict lower,
                         const double * restrict upper,
                         double sigma_zeta, double sigma_eta, uint32_t seed)
{
    ASSUME(n >= 3 && nvar >= 1);
    rng_state * rng = rng_new(seed);
    double * direction = MOOCORE_MALLOC(3 * nvar, double);
    double * unit = direction + nvar;
    double * orthogonal = unit + nvar;
    for (size_t t = 0; t < ntargets; t++) {
        ASSUME(targets[t] < n);
        const double * x = population + targets[t] * nvar;
        size_t excluded[3] = { targets[t] };
        size_t r[2];
        sample_distinct(rng, n, excluded, 1, 2, r);
        const double * others[2] = { population + r[0] * nvar,
                                     population + r[1] * nvar };

        // Direction from the centroid of the three parents to the index
        // parent X.
        double norm = 0;
        for (size_t j = 0; j < nvar; j++) {
            direction[j] = x[j] - (x[j] + others[0][j] + others[1][j]) / 3;
            norm += direction[j] * direction[j];
        }
        norm = sqrt(norm);
        const bool degenerate = norm < PCX_DEGENERATE;
        const double divisor = degenerate ? 1 : norm;
        for (size_t j = 0; j < nvar; j++)
            unit[j] = direction[j] / divisor;

        // Mean distance from the other two parents to the line through X
        // along DIRECTION.
        double spread = 0;
        for (int k = 0; k < 2; k++) {
            const double * o = others[k];
            double along = 0;
            for (size_t j = 0; j < nvar; j++)
                along += (o[j] - x[j]) * unit[j];
            double dist = 0;
            for (size_t j = 0; j < nvar; j++) {
                const double perp = (o[j] - x[j]) - along * unit[j];
                dist += perp * perp;
            }
            spread += sqrt(dist);
        }
        spread /= 2;

        // Gaussian noise with its component along DIRECTION removed.  This
        // has the same distribution as summing independent normal steps
        // along an orthonormal basis of the subspace orthogonal to DIRECTION.
        double proj = 0;
        for (size_t j = 0; j < nvar; j++) {
            orthogonal[j] = sigma_eta * rng_standard_normal(rng);
            proj += orthogonal[j] * unit[j];
        }
        if (!degenerate) {
            for (size_t j = 0; j < nvar; j++)
                orthogonal[j] -= proj * unit[j];
        }
        const double zeta = sigma_zeta * rng_standard_normal(rng);

        double * off = offspring + t * nvar;
        for (size_t j = 0; j < nvar; j++)
            off[j] = clamp_to_bounds(
                x[j] + zeta * direction[j] + spread * orthogonal[j],
                lower[j], upper[j]);
    }
    free(direction);
    rng_free(rng);
}
