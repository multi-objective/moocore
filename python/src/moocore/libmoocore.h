/* From stdlib.h */
void free(void *);
int read_datasets(const char * filename, double **data_p, int *ncols_p, int *datasize_p);
double fpli_hv(const double *data, int d, int n, const double *ref);
double IGD (const double *data, int nobj, int npoints, const double *ref, int ref_size, const bool * maximise);
double IGD_plus (const double *data, int nobj, int npoints, const double *ref, int ref_size, const bool * maximise);
double avg_Hausdorff_dist (const double *data, int nobj, int npoints, const double *ref, int ref_size, const bool * maximise, unsigned int p);
double epsilon_additive (const double *data, int nobj, int npoints, const double *ref, int ref_size, const bool * maximise);
double epsilon_mult (const double *data, int nobj, int npoints, const double *ref, int ref_size, const bool * maximise);
bool * is_nondominated (const double * data, int nobj, int npoint, const bool * maximise, bool keep_weakly);
int * pareto_rank (const double *points, int dim, int size);
void agree_normalise (double *data, int nobj, int npoint, const bool * maximise,
                      const double lower_range, const double upper_range, const double *lbound, const double *ubound);
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

/*
typedef ... hype_sample_dist;
hype_sample_dist * hype_dist_unif_new(unsigned long seed);
hype_sample_dist * hype_dist_exp_new(double mu, unsigned long seed);
hype_sample_dist * hype_dist_gaussian_new(const double *mu, unsigned long int seed);
void hype_dist_free(hype_sample_dist * d);
double whv_hype_estimate(const double *points, size_t n, const double *ideal, const double *ref, hype_sample_dist * dist, size_t nsamples);
*/
