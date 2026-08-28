
/*
 * test-harness.c
 * Statistical and correctness tests for a Walker-Vose alias sampler.
 * The random-wheel reference is assumed to be provided by rng.h.
 */
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rng.h"
#include "rng_alias.h"

#define MASTER_SEED             UINT32_C(0x5EED1234)
#define STANDARD_SAMPLES        UINT64_C(1000000)
#define RANDOM_SAMPLES          UINT64_C(250000)
#define SERIAL_SAMPLES          UINT64_C(500000)
#define LARGE_TABLE_SAMPLES     UINT64_C(750000)
#define LIFECYCLE_ITERATIONS    UINT32_C(20000)
#define RANDOM_CASES            UINT32_C(500)
#define RANDOM_MAX_N            UINT32_C(257)
#define STAT_P_MIN              1.0e-4
#define RANDOM_INDIVIDUAL_MIN   1.0e-10
#define FISHER_P_MIN            1.0e-4
#define MIN_EXPECTED            5.0

static unsigned g_tests_run;
static unsigned g_tests_failed;

struct fisher_accumulator {
    double statistic;
    unsigned count;
};

struct sample_result {
    uint64_t *counts;
    uint64_t samples;
    uint64_t out_of_range;
    uint64_t zero_hits;
};

struct gof_result {
    double pearson;
    double pearson_p;
    double g;
    double g_p;
    int df;
};

struct two_sample_result {
    double chi2;
    double pvalue;
    double max_diff;
    int df;
};

static void record_result(const char *name, int ok)
{
    ++g_tests_run;
    if (!ok)
        ++g_tests_failed;
    printf("%-42s %s\n", name, ok ? "PASS" : "FAIL");
}

static int is_weight_vector(const double *w, uint32_t n)
{
    double sum = 0.0;
    uint32_t i;

    if (w == NULL || n == 0)
        return 0;
    for (i = 0; i < n; ++i) {
        if (!isfinite(w[i]) || w[i] < 0.0)
            return 0;
        sum += w[i];
    }
    return isfinite(sum) && sum > 0.0;
}

static int normalize_weights(const double *w, double *p, uint32_t n)
{
    double sum = 0.0;
    uint32_t i;

    if (!is_weight_vector(w, n) || p == NULL)
        return 0;
    for (i = 0; i < n; ++i)
        sum += w[i];
    if (!(sum > 0.0) || !isfinite(sum))
        return 0;
    for (i = 0; i < n; ++i)
        p[i] = w[i] / sum;
    return 1;
}

static int make_cdf(const double *w, double *cdf, uint32_t n)
{
    double *p;
    double c = 0.0;
    uint32_t i;

    if (!is_weight_vector(w, n) || cdf == NULL)
        return 0;
    p = (double *)malloc((size_t)n * sizeof(*p));
    if (p == NULL)
        return 0;
    if (!normalize_weights(w, p, n)) {
        free(p);
        return 0;
    }
    for (i = 0; i < n; ++i) {
        c += p[i];
        cdf[i] = c;
    }
    cdf[n - 1] = 1.0;
    free(p);
    return 1;
}

static uint64_t sum_counts(const uint64_t *counts, uint32_t n)
{
    uint64_t total = 0;
    uint32_t i;

    for (i = 0; i < n; ++i)
        total += counts[i];
    return total;
}

static double u32_to_open01(uint32_t x)
{
    return ((double)x + 0.5) / 4294967296.0;
}

static double gamma_q(double a, double x)
{
    const int max_iter = 10000;
    const double eps = 3.0e-14;
    const double fpmin = DBL_MIN / eps;
    int i;

    if (a <= 0.0 || x < 0.0)
        return NAN;
    if (x == 0.0)
        return 1.0;
    if (x < a + 1.0) {
        double ap = a;
        double sum = 1.0 / a;
        double del = sum;

        for (i = 1; i <= max_iter; ++i) {
            ++ap;
            del *= x / ap;
            sum += del;
            if (fabs(del) < fabs(sum) * eps)
                break;
        }
        return 1.0 - sum * exp(-x + a * log(x) - lgamma(a));
    }
    else {
        double b = x + 1.0 - a;
        double c = 1.0 / fpmin;
        double d = 1.0 / b;
        double h = d;

        for (i = 1; i <= max_iter; ++i) {
            double an = -(double)i * ((double)i - a);
            double del;

            b += 2.0;
            d = an * d + b;
            if (fabs(d) < fpmin)
                d = fpmin;
            c = b + an / c;
            if (fabs(c) < fpmin)
                c = fpmin;
            d = 1.0 / d;
            del = d * c;
            h *= del;
            if (fabs(del - 1.0) < eps)
                break;
        }
        return exp(-x + a * log(x) - lgamma(a)) * h;
    }
}

static double chi_square_pvalue(double stat, int df)
{
    if (df <= 0 || !isfinite(stat))
        return NAN;

    /*
     * Pearson and G statistics are non-negative mathematically, but the
     * accumulated G statistic can end up as a tiny negative number from
     * floating-point roundoff when observed and expected counts are very
     * close.  Treat that as exact zero rather than as an invalid statistic.
     */
    if (stat < 0.0) {
        if (stat > -1.0e-10)
            stat = 0.0;
        else
            return NAN;
    }

    return gamma_q(0.5 * (double)df, 0.5 * stat);
}

static void fisher_init(struct fisher_accumulator *f)
{
    f->statistic = 0.0;
    f->count = 0;
}

static void fisher_add(struct fisher_accumulator *f, double p)
{
    if (!isfinite(p) || p <= 0.0)
        p = DBL_MIN;
    if (p > 1.0)
        p = 1.0;
    f->statistic += -2.0 * log(p);
    ++f->count;
}

static double fisher_pvalue(const struct fisher_accumulator *f)
{
    if (f->count == 0)
        return NAN;
    return chi_square_pvalue(f->statistic, 2 * (int)f->count);
}

static int alloc_sample_result(struct sample_result *r, uint32_t n)
{
    r->counts = (uint64_t *)calloc(n, sizeof(*r->counts));
    r->samples = 0;
    r->out_of_range = 0;
    r->zero_hits = 0;
    return r->counts != NULL;
}

static void free_sample_result(struct sample_result *r)
{
    free(r->counts);
    r->counts = NULL;
}

static int sample_vose(rng_state *rng, rng_alias_sampler_t *sampler,
                       const double *p, uint32_t n,
                       uint64_t samples, struct sample_result *r)
{
    uint64_t i;

    memset(r->counts, 0, (size_t)n * sizeof(*r->counts));
    r->samples = samples;
    r->out_of_range = 0;
    r->zero_hits = 0;
    for (i = 0; i < samples; ++i) {
        uint32_t x = rng_alias_sampler_choose(rng, sampler);

        if (x >= n) {
            ++r->out_of_range;
            continue;
        }
        if (p[x] == 0.0)
            ++r->zero_hits;
        ++r->counts[x];
    }
    return r->out_of_range == 0 && sum_counts(r->counts, n) == samples;
}

static int sample_wheel(rng_state *rng, const double *w, const double *p,
                        uint32_t n, uint64_t samples,
                        struct sample_result *r)
{
    double *cdf;
    uint64_t i;

    cdf = (double *)malloc((size_t)n * sizeof(*cdf));
    if (cdf == NULL)
        return 0;
    if (!make_cdf(w, cdf, n)) {
        free(cdf);
        return 0;
    }
    memset(r->counts, 0, (size_t)n * sizeof(*r->counts));
    r->samples = samples;
    r->out_of_range = 0;
    r->zero_hits = 0;
    for (i = 0; i < samples; ++i) {
        uint32_t x = rng_random_wheel_uint32(rng, cdf, n);

        if (x >= n) {
            ++r->out_of_range;
            continue;
        }
        if (p[x] == 0.0)
            ++r->zero_hits;
        ++r->counts[x];
    }
    free(cdf);
    return r->out_of_range == 0 && sum_counts(r->counts, n) == samples;
}

static struct gof_result goodness_of_fit(const double *p,
                                         const uint64_t *obs,
                                         uint32_t n,
                                         uint64_t samples)
{
    struct gof_result r;
    double tail_e = 0.0;
    double tail_o = 0.0;
    uint32_t bins = 0;
    uint32_t i;

    r.pearson = 0.0;
    r.g = 0.0;
    for (i = 0; i < n; ++i) {
        double e = p[i] * (double)samples;
        double o = (double)obs[i];

        if (e >= MIN_EXPECTED) {
            double d = o - e;
            r.pearson += d * d / e;
            if (o > 0.0)
                r.g += 2.0 * o * log(o / e);
            ++bins;
        }
        else {
            tail_e += e;
            tail_o += o;
        }
    }
    /*
     * Do not discard the pooled small-expected tail.  Dropping it breaks
     * the equality between total observed and total expected counts over
     * the retained bins, and can make the likelihood-ratio statistic
     * negative, which then produces a NaN p-value.
     */
    if (tail_e > 0.0 || tail_o > 0.0) {
        if (tail_e > 0.0) {
            double d = tail_o - tail_e;
            r.pearson += d * d / tail_e;
            if (tail_o > 0.0)
                r.g += 2.0 * tail_o * log(tail_o / tail_e);
            ++bins;
        }
        else {
            r.pearson = INFINITY;
            r.g = INFINITY;
            ++bins;
        }
    }
    if (r.pearson < 0.0 && r.pearson > -1.0e-10)
        r.pearson = 0.0;
    if (r.g < 0.0 && r.g > -1.0e-10)
        r.g = 0.0;

    r.df = bins > 1 ? (int)bins - 1 : 0;
    if (r.df == 0) {
        r.pearson_p = 1.0;
        r.g_p = 1.0;
    }
    else {
        r.pearson_p = chi_square_pvalue(r.pearson, r.df);
        r.g_p = chi_square_pvalue(r.g, r.df);
    }
    return r;
}

static struct two_sample_result two_sample_chi_square(const uint64_t *a,
                                                      uint64_t na,
                                                      const uint64_t *b,
                                                      uint64_t nb,
                                                      uint32_t n)
{
    struct two_sample_result r;
    double tail_a = 0.0;
    double tail_b = 0.0;
    uint32_t bins = 0;
    uint32_t i;

    r.chi2 = 0.0;
    r.max_diff = 0.0;
    r.df = 0;
    if (na == 0 || nb == 0) {
        r.pvalue = NAN;
        return r;
    }
    for (i = 0; i < n; ++i) {
        double ai = (double)a[i];
        double bi = (double)b[i];
        double total = ai + bi;
        double diff = fabs(ai / (double)na - bi / (double)nb);

        if (diff > r.max_diff)
            r.max_diff = diff;
        if (total > 0.0) {
            double ea = (double)na * total / (double)(na + nb);
            double eb = (double)nb * total / (double)(na + nb);

            if (ea >= MIN_EXPECTED && eb >= MIN_EXPECTED) {
                double da = ai - ea;
                double db = bi - eb;

                r.chi2 += da * da / ea + db * db / eb;
                ++bins;
            }
            else {
                tail_a += ai;
                tail_b += bi;
            }
        }
    }
    if (tail_a + tail_b > 0.0) {
        double total = tail_a + tail_b;
        double ea = (double)na * total / (double)(na + nb);
        double eb = (double)nb * total / (double)(na + nb);

        if (ea >= MIN_EXPECTED && eb >= MIN_EXPECTED) {
            double da = tail_a - ea;
            double db = tail_b - eb;

            r.chi2 += da * da / ea + db * db / eb;
            ++bins;
        }
    }
    if (r.chi2 < 0.0 && r.chi2 > -1.0e-10)
        r.chi2 = 0.0;

    r.df = bins > 1 ? (int)bins - 1 : 0;
    if (r.df == 0)
        r.pvalue = 1.0;
    else
        r.pvalue = chi_square_pvalue(r.chi2, r.df);
    return r;
}

static int support_ok(const struct sample_result *r)
{
    return r->out_of_range == 0 && r->zero_hits == 0;
}

static int make_uniform(double *w, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; ++i)
        w[i] = 1.0;
    return 1;
}

static int make_linear(double *w, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; ++i)
        w[i] = (double)(i + 1);
    return 1;
}

static int make_alternating_zeros(double *w, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; ++i)
        w[i] = (i & 1U) ? 0.0 : 1.0;
    return 1;
}

static int make_dynamic_range(double *w, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; ++i)
        w[i] = exp(-0.75 * (double)i);
    return 1;
}

static int make_random_weights(rng_state *rng, double *w, uint32_t n)
{
    uint32_t i;
    int positive = 0;

    for (i = 0; i < n; ++i) {
        uint32_t r = mt19937_next32(rng);
        double u = u32_to_open01(r);
        double z = u32_to_open01(mt19937_next32(rng));

        if ((r & UINT32_C(15)) == 0) {
            w[i] = 0.0;
        }
        else {
            w[i] = exp(-30.0 * u) * (1.0 + 1000.0 * z);
            positive = 1;
        }
    }
    if (!positive)
        w[0] = 1.0;
    return 1;
}

static int deterministic_distribution(const double *p, uint32_t n,
                                      uint32_t *value)
{
    uint32_t i;
    uint32_t seen = 0;

    *value = UINT32_MAX;
    for (i = 0; i < n; ++i) {
        if (p[i] > 0.0) {
            *value = i;
            ++seen;
        }
    }
    return seen == 1;
}

static int run_distribution_case(const char *name, const double *w,
                                 uint32_t n, uint64_t samples,
                                 uint32_t seed_base,
                                 struct fisher_accumulator *fisher,
                                 int random_case)
{
    double *p;
    rng_state *vose_rng = NULL;
    rng_state *wheel_rng = NULL;
    rng_alias_sampler_t *sampler = NULL;
    struct sample_result vose;
    struct sample_result wheel;
    struct gof_result gv;
    struct gof_result gw;
    struct two_sample_result diff;
    uint32_t det_value;
    int ok = 1;
    int allocated_vose = 0;
    int allocated_wheel = 0;
    double cutoff = random_case ? RANDOM_INDIVIDUAL_MIN : STAT_P_MIN;

    p = (double *)malloc((size_t)n * sizeof(*p));
    if (p == NULL)
        return 0;
    if (!normalize_weights(w, p, n)) {
        free(p);
        return 0;
    }

    sampler = rng_alias_sampler_new(w, n);
    vose_rng = rng_new(seed_base + UINT32_C(1));
    wheel_rng = rng_new(seed_base + UINT32_C(2));
    if (sampler == NULL || vose_rng == NULL || wheel_rng == NULL) {
        ok = 0;
        goto cleanup;
    }
    if (!alloc_sample_result(&vose, n) || !alloc_sample_result(&wheel, n)) {
        ok = 0;
        goto cleanup;
    }
    allocated_vose = 1;
    allocated_wheel = 1;

    if (!sample_vose(vose_rng, sampler, p, n, samples, &vose))
        ok = 0;
    if (!sample_wheel(wheel_rng, w, p, n, samples, &wheel))
        ok = 0;
    if (!support_ok(&vose) || !support_ok(&wheel))
        ok = 0;

    if (deterministic_distribution(p, n, &det_value)) {
        if (vose.counts[det_value] != samples)
            ok = 0;
        if (wheel.counts[det_value] != samples)
            ok = 0;
        if (!ok) {
            printf("  %-30s deterministic support FAIL\n", name);
            printf("  %-30s Vose count=%llu Wheel count=%llu\n",
                   "",
                   (unsigned long long)vose.counts[det_value],
                   (unsigned long long)wheel.counts[det_value]);
        }
        goto cleanup;
    }

    gv = goodness_of_fit(p, vose.counts, n, samples);
    gw = goodness_of_fit(p, wheel.counts, n, samples);
    diff = two_sample_chi_square(wheel.counts, samples, vose.counts,
                                 samples, n);

    if (!isfinite(gv.pearson_p) || !isfinite(gv.g_p) ||
        gv.pearson_p <= cutoff || gv.g_p <= cutoff)
        ok = 0;
    if (!isfinite(gw.pearson_p) || gw.pearson_p <= cutoff)
        ok = 0;
    if (!isfinite(diff.pvalue) || diff.pvalue <= cutoff)
        ok = 0;

    if (fisher != NULL) {
        fisher_add(fisher, gv.pearson_p);
        fisher_add(fisher, gv.g_p);
        fisher_add(fisher, gw.pearson_p);
        fisher_add(fisher, diff.pvalue);
    }

    if (!ok) {
        printf("  %-30s Vose P=%8.5f G=%8.5f Wheel P=%8.5f\n",
               name, gv.pearson_p, gv.g_p, gw.pearson_p);
        printf("  %-30s Diff P=%8.5f max|dp|=%8.6f FAIL\n",
               "", diff.pvalue, diff.max_diff);
        printf("  %-30s out_of_range V=%llu W=%llu zero_hits V=%llu W=%llu\n",
               "",
               (unsigned long long)vose.out_of_range,
               (unsigned long long)wheel.out_of_range,
               (unsigned long long)vose.zero_hits,
               (unsigned long long)wheel.zero_hits);
    }

cleanup:
    if (allocated_vose)
        free_sample_result(&vose);
    if (allocated_wheel)
        free_sample_result(&wheel);
    rng_alias_sampler_free(sampler);
    rng_free(vose_rng);
    rng_free(wheel_rng);
    free(p);
    return ok;
}

static int serial_independence_case(const char *name, const double *w,
                                    uint32_t n, uint32_t seed)
{
    double *p = NULL;
    uint64_t *pairs = NULL;
    rng_alias_sampler_t *sampler = NULL;
    rng_state *rng = NULL;
    uint32_t prev;
    uint64_t i;
    double stat = 0.0;
    double tail_e = 0.0;
    double tail_o = 0.0;
    uint32_t bins = 0;
    int df;
    double pvalue;
    int ok = 1;

    if (n > 32)
        return 1;
    p = (double *)malloc((size_t)n * sizeof(*p));
    pairs = (uint64_t *)calloc((size_t)n * (size_t)n, sizeof(*pairs));
    sampler = rng_alias_sampler_new(w, n);
    rng = rng_new(seed);
    if (p == NULL || pairs == NULL || sampler == NULL || rng == NULL ||
        !normalize_weights(w, p, n)) {
        ok = 0;
        goto cleanup;
    }

    prev = rng_alias_sampler_choose(rng, sampler);
    if (prev >= n || p[prev] == 0.0) {
        ok = 0;
        goto cleanup;
    }
    for (i = 1; i < SERIAL_SAMPLES; ++i) {
        uint32_t cur = rng_alias_sampler_choose(rng, sampler);
        if (cur >= n || p[cur] == 0.0) {
            ok = 0;
            goto cleanup;
        }
        ++pairs[(size_t)prev * n + cur];
        prev = cur;
    }

    for (i = 0; i < (uint64_t)n * (uint64_t)n; ++i) {
        uint32_t a = (uint32_t)(i / n);
        uint32_t b = (uint32_t)(i % n);
        double e = (double)(SERIAL_SAMPLES - 1) * p[a] * p[b];
        double o = (double)pairs[i];

        if (e >= MIN_EXPECTED) {
            double d = o - e;
            stat += d * d / e;
            ++bins;
        }
        else {
            tail_e += e;
            tail_o += o;
        }
    }
    if (tail_e > 0.0 || tail_o > 0.0) {
        if (tail_e > 0.0) {
            double d = tail_o - tail_e;
            stat += d * d / tail_e;
            ++bins;
        }
        else {
            stat = INFINITY;
            ++bins;
        }
    }
    df = bins > 1 ? (int)bins - 1 : 0;
    pvalue = chi_square_pvalue(stat, df);
    if (!isfinite(pvalue) || pvalue <= STAT_P_MIN)
        ok = 0;

    printf("  %-30s serial P=%8.5f %s\n",
           name, pvalue, ok ? "PASS" : "FAIL");

cleanup:
    free(pairs);
    free(p);
    rng_alias_sampler_free(sampler);
    rng_free(rng);
    return ok;
}

static int test_invalid_inputs(void)
{
    rng_alias_sampler_t *s;
    double p2[] = { 0.5, 0.5 };
    double zero[] = { 0.0, 0.0 };
    double negative[] = { 0.5, -0.1, 0.6 };
    double nanv[] = { 0.5, NAN };
    int ok = 1;

    s = rng_alias_sampler_new(NULL, 0);
    if (s != NULL) { rng_alias_sampler_free(s); ok = 0; }
    s = rng_alias_sampler_new(NULL, 2);
    if (s != NULL) { rng_alias_sampler_free(s); ok = 0; }
    s = rng_alias_sampler_new(p2, 0);
    if (s != NULL) { rng_alias_sampler_free(s); ok = 0; }
    s = rng_alias_sampler_new(zero, 2);
    if (s != NULL) { rng_alias_sampler_free(s); ok = 0; }
    s = rng_alias_sampler_new(negative, 3);
    if (s != NULL) { rng_alias_sampler_free(s); ok = 0; }
    s = rng_alias_sampler_new(nanv, 2);
    if (s != NULL) { rng_alias_sampler_free(s); ok = 0; }
    return ok;
}

static int test_lifecycle(void)
{
    double w[] = { 7.0, 0.0, 3.0, 1.0, 11.0 };
    uint32_t i;

    for (i = 0; i < LIFECYCLE_ITERATIONS; ++i) {
        rng_alias_sampler_t *s = rng_alias_sampler_new(w, 5);
        if (s == NULL)
            return 0;
        rng_alias_sampler_free(s);
    }
    return 1;
}

static int run_named_distribution_tests(void)
{
    struct fisher_accumulator fisher;
    int ok = 1;
    double *w;

    fisher_init(&fisher);
    w = (double *)malloc((size_t)4099 * sizeof(*w));
    if (w == NULL)
        return 0;

#define RUN_CASE(label, nval, maker, samples, seed)                  \
    do {                                                            \
        if (!(maker)(w, (nval)) ||                                  \
            !run_distribution_case((label), w, (nval), (samples),   \
                                   (seed), &fisher, 0))             \
            ok = 0;                                                 \
    } while (0)

    RUN_CASE("n=1", 1, make_uniform, STANDARD_SAMPLES, 1000);
    RUN_CASE("n=2 half", 2, make_uniform, STANDARD_SAMPLES, 2000);
    RUN_CASE("uniform n=3", 3, make_uniform, STANDARD_SAMPLES, 3000);
    RUN_CASE("uniform n=7", 7, make_uniform, STANDARD_SAMPLES, 4000);
    RUN_CASE("linear n=17", 17, make_linear, STANDARD_SAMPLES, 5000);
    RUN_CASE("zeros n=31", 31, make_alternating_zeros,
             STANDARD_SAMPLES, 6000);
    RUN_CASE("dynamic n=64", 64, make_dynamic_range,
             STANDARD_SAMPLES, 7000);
    RUN_CASE("large n=4099", 4099, make_uniform,
             LARGE_TABLE_SAMPLES, 8000);

#undef RUN_CASE

    {
        double boundary2[] = { 0.5 - DBL_EPSILON, 0.5 + DBL_EPSILON };
        double near_zero[] = { 1.0, DBL_EPSILON, 0.0, DBL_MIN };
        double near_one[] = { 1.0 - 1.0e-9, 1.0e-9, 0.0, 0.0 };
        double unnorm[] = { 1000.0, 0.0, 1.0, 3.0, 11.0, 31.0 };
        double extreme[] = { 0.0, 1e-300, 1e-200, 1e-100,
                             1e-50, 1e-20, 1e-10, 1.0,
                             100.0, 1e5, 1e10 };

        if (!run_distribution_case("boundary 0.5", boundary2, 2,
                                   STANDARD_SAMPLES, 9000, &fisher, 0))
            ok = 0;
        if (!run_distribution_case("near zero", near_zero, 4,
                                   STANDARD_SAMPLES, 10000, &fisher, 0))
            ok = 0;
        if (!run_distribution_case("near one", near_one, 4,
                                   STANDARD_SAMPLES, 11000, &fisher, 0))
            ok = 0;
        if (!run_distribution_case("unnormalized", unnorm, 6,
                                   STANDARD_SAMPLES, 12000, &fisher, 0))
            ok = 0;
        if (!run_distribution_case("extreme range", extreme, 11,
                                   STANDARD_SAMPLES, 13000, &fisher, 0))
            ok = 0;
        if (!serial_independence_case("serial uniform n=7", w, 7,
                                      MASTER_SEED + 14000))
            ok = 0;
        if (!serial_independence_case("serial boundary 0.5", boundary2, 2,
                                      MASTER_SEED + 15000))
            ok = 0;
    }
    {
        double fp = fisher_pvalue(&fisher);
        printf("  %-30s Fisher P=%8.5f (%u tests)\n",
               "standard aggregate", fp, fisher.count);
        if (!isfinite(fp) || fp <= FISHER_P_MIN)
            ok = 0;
    }
    free(w);
    return ok;
}

static int run_randomized_tests(void)
{
    struct fisher_accumulator fisher;
    rng_state *master;
    double *w;
    uint32_t case_no;
    int ok = 1;

    fisher_init(&fisher);
    master = rng_new(MASTER_SEED + UINT32_C(0x12345678));
    w = (double *)malloc((size_t)RANDOM_MAX_N * sizeof(*w));
    if (master == NULL || w == NULL) {
        rng_free(master);
        free(w);
        return 0;
    }
    for (case_no = 0; case_no < RANDOM_CASES; ++case_no) {
        uint32_t n = 2 + rng_uniform_u32_ubound(master, RANDOM_MAX_N - 1);
        uint32_t seed = MASTER_SEED + UINT32_C(200000) + case_no * 4U;
        int case_ok;

        if (!make_random_weights(master, w, n)) {
            ok = 0;
            break;
        }
        case_ok = run_distribution_case("random", w, n, RANDOM_SAMPLES,
                                        seed, &fisher, 1);
        if (!case_ok)
            ok = 0;
    }
    {
        double fp = fisher_pvalue(&fisher);
        printf("  %-30s Fisher P=%8.5f (%u tests)\n",
               "random aggregate", fp, fisher.count);
        if (!isfinite(fp) || fp <= FISHER_P_MIN)
            ok = 0;
    }
    rng_free(master);
    free(w);
    return ok;
}

int main(void)
{
    int ok = 1;
    int result;

    printf("Walker-Vose alias sampler test harness\n");
    printf("======================================\n\n");

    result = test_invalid_inputs();
    record_result("invalid inputs", result);
    if (!result)
        ok = 0;

    result = test_lifecycle();
    record_result("repeated construction/destruction", result);
    if (!result)
        ok = 0;

    printf("\nNamed and boundary distributions\n");
    printf("--------------------------------\n");
    result = run_named_distribution_tests();
    record_result("named distribution suite", result);
    if (!result)
        ok = 0;

    printf("\nRandomized distributions\n");
    printf("------------------------\n");
    result = run_randomized_tests();
    record_result("randomized distribution suite", result);
    if (!result)
        ok = 0;

    printf("\n======================================\n");
    if (ok) {
        printf("RESULT: PASS (%u test groups)\n", g_tests_run);
    }
    else {
        printf("RESULT: FAIL (%u test groups, %u failed)\n",
               g_tests_run, g_tests_failed);
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
