// Must be consistent with the definition in moocore.
typedef uint_fast8_t dimension_t;

// From stdlib.h
void free(void *);

// io.h
int read_datasets(const char * filename, double ** restrict data_p, int * restrict ncols_p, int * restrict datasize_p);
// hv.h
double fpli_hv(const double * restrict data, size_t n, dimension_t d, const double * restrict ref);
void hv_contributions(double * restrict hvc, double * restrict points, size_t n, dimension_t d, const double * restrict ref, bool ignore_dominated);
// igd.h
double IGD(const double * restrict data, size_t n, dimension_t d, const double * restrict ref, size_t ref_size, const bool * restrict maximise);
double IGD_plus(const double * restrict data, size_t n, dimension_t d, const double * restrict ref, size_t ref_size, const bool * restrict maximise);
double avg_Hausdorff_dist(const double * restrict data, size_t n, dimension_t d, const double * restrict ref, size_t ref_size, const bool * restrict maximise, unsigned int p);
// epsilon.h
double epsilon_additive(const double * restrict data, size_t n, dimension_t d, const double * restrict ref, size_t ref_size, const bool * restrict maximise);
double epsilon_mult(const double * restrict data, size_t n, dimension_t d, const double * restrict ref, size_t ref_size, const bool * restrict maximise);

// nondominated.h
size_t find_weakly_dominated_point(const double * restrict points, size_t n, dimension_t d,
                                   const bool * restrict maximise);
bool * is_nondominated(const double * restrict data, size_t n, dimension_t d,
                       const bool * restrict maximise, bool keep_weakly);
int * pareto_rank(const double * restrict points, size_t size, dimension_t dim);
void agree_normalise(double * restrict data, size_t size, dimension_t dim,
                     const bool * restrict maximise,
                     const double lower_range, const double upper_range,
                     const double * restrict lbound, const double * restrict ubound);

double * eaf_compute_matrix (int *eaf_npoints, double * data, int nobj, const int *cumsizes,
                             int nruns, const double * percentile, int nlevels);
double * eafdiff_compute_rectangles(int *eaf_npoints, double * data, int nobj,
                                    const int *cumsizes, int nruns, int intervals);
double *
eafdiff_compute_matrix(int *eaf_npoints, double * data, int nobj,
                       const int *cumsizes, int nruns, int intervals);

/* whv_hype.h */
double whv_hype_unif(const double *points, int npoints,
                     const double *ideal, const double *ref,
                     int nsamples, uint32_t seed);
double whv_hype_expo(const double *points, int npoints,
                     const double *ideal, const double *ref,
                     int nsamples, uint32_t seed, double mu);
double whv_hype_gaus(const double *points, int npoints,
                     const double *ideal, const double *ref,
                     int nsamples, uint32_t seed, const double *mu);
/* whv.h */
double rect_weighted_hv2d(double *data, int n, double * rectangles, int rectangles_nrow, const double * reference);

// hvapprox.h
double hv_approx_hua_wang(const double * restrict data,
                          size_t npoints, dimension_t nobjs,
                          const double * restrict ref,
                          const bool * restrict maximise,
                          uint_fast32_t nsamples);
double hv_approx_normal(const double * restrict data,
                        size_t npoints, dimension_t nobjs,
                        const double * restrict ref,
                        const bool * restrict maximise,
                        uint_fast32_t nsamples, uint32_t random_seed);

/*
typedef ... hype_sample_dist;
hype_sample_dist * hype_dist_unif_new(unsigned long seed);
hype_sample_dist * hype_dist_exp_new(double mu, unsigned long seed);
hype_sample_dist * hype_dist_gaussian_new(const double *mu, unsigned long int seed);
void hype_dist_free(hype_sample_dist * d);
double whv_hype_estimate(const double *points, size_t n, const double *ideal, const double *ref, hype_sample_dist * dist, size_t nsamples);
*/
