static const int HV_INEX_MAX_ROWS;
static const int KUNG_SMALL_THRESHOLD;
static const int MOOCORE_DIMENSION_MAX;
static const int MOOCORE_HV_DIMENSION_MAX;
static const int MOOCORE_HVAPPROX_DIMENSION_MAX;

// Must be consistent with the definition in moocore.
typedef uint_fast8_t dimension_t;
typedef uint8_t boolvec;

// From stdlib.h
void free(void *);

// io.h
int read_datasets(const char * filename, double ** restrict data_p, int * restrict ncols_p, int * restrict datasize_p);
// hv.h
double fpli_hv(const double * restrict data, size_t n, dimension_t d, const double * restrict ref);
void hv_contributions(double * restrict hvc, double * restrict points, size_t n, dimension_t d, const double * restrict ref, bool ignore_dominated);
// igd.h
double IGD(const double * restrict data, size_t n, dimension_t d, const double * restrict ref, size_t ref_size, const boolvec * restrict maximise);
double IGD_plus(const double * restrict data, size_t n, dimension_t d, const double * restrict ref, size_t ref_size, const boolvec * restrict maximise);
double avg_Hausdorff_dist(const double * restrict data, size_t n, dimension_t d, const double * restrict ref, size_t ref_size, const boolvec * restrict maximise, unsigned int p);
// epsilon.h
double epsilon_additive(const double * restrict data, size_t n, dimension_t d, const double * restrict ref, size_t ref_size, const boolvec * restrict maximise);
double epsilon_mult(const double * restrict data, size_t n, dimension_t d, const double * restrict ref, size_t ref_size, const boolvec * restrict maximise);
// r2_exact.h
double r2_exact(const double * restrict data, size_t n, dimension_t d, const double * restrict ref);

// nondominated.h
size_t find_weakly_dominated_point(const double * restrict points, size_t n, dimension_t d,
                                   const boolvec * restrict maximise);
void is_nondominated(boolvec * restrict nondom,
                     const double * restrict data, size_t n, dimension_t d,
                     bool keep_weakly, const boolvec * restrict maximise);
void pareto_rank(int * rank, const double * restrict points, size_t size, dimension_t dim);
void agree_normalise(double * restrict data, size_t size, dimension_t dim,
                     const boolvec * restrict maximise,
                     const double lower_range, const double upper_range,
                     const double * restrict lbound, const double * restrict ubound);

double * eaf_compute_matrix (int *eaf_npoints, double * data, int nobj, const int *cumsizes,
                             int nruns, const double * percentile, int nlevels);
double * eafdiff_compute_rectangles(int *eaf_npoints, double * data, int nobj,
                                    const int *cumsizes, int nruns, int intervals);
double *
eafdiff_compute_matrix(int *eaf_npoints, double * data, int nobj,
                       const int *cumsizes, int nruns, int intervals);

// whv_hype.h
double whv_hype_unif(const double *points, int npoints,
                     const double *ideal, const double *ref,
                     int nsamples, uint32_t seed);
double whv_hype_expo(const double *points, int npoints,
                     const double *ideal, const double *ref,
                     int nsamples, uint32_t seed, double mu);
double whv_hype_gaus(const double *points, int npoints,
                     const double *ideal, const double *ref,
                     int nsamples, uint32_t seed, const double *mu);
// whv.h
double rect_weighted_hv2d(double *data, int n, double * rectangles, int rectangles_nrow, const double * reference);

// hvapprox.h
double hv_approx_hua_wang(const double * restrict data,
                          size_t npoints, dimension_t nobjs,
                          const double * restrict ref,
                          const boolvec * restrict maximise,
                          uint_fast32_t nsamples);

double hv_approx_normal(const double * restrict data,
                        size_t npoints, dimension_t nobjs,
                        const double * restrict ref,
                        const boolvec * restrict maximise,
                        uint_fast32_t nsamples, uint32_t random_seed);

double hv_approx_rphi_fang_wang_plus(const double * restrict data,
                                     size_t npoints, dimension_t nobjs,
                                     const double * restrict ref,
                                     const boolvec * restrict maximise,
                                     uint_fast32_t nsamples);

// archive.h
typedef enum archive_insert_result_t {
    ARCHIVE_INSERT_X_MUST_BE_NOT_NULL =...,
    ARCHIVE_INSERT_X_MUST_BE_NULL,
    ARCHIVE_INSERT_MEMORY_ERROR,
    ARCHIVE_INSERT_REJECTED,
    ARCHIVE_INSERT_ACCEPTED,
    ARCHIVE_INSERT_DUPLICATED,
};
typedef struct SolutionsList SolutionsList;

// treap_archive.h
typedef struct TreapArchive TreapArchive;
typedef struct TreapNode TreapNode;

TreapArchive * treap_archive_new(void);
void treap_archive_free(TreapArchive * t);
TreapNode * treap_archive_find_exact_vector(const TreapArchive * t, const double * z);
bool treap_archive_dominated_by(const TreapArchive * self, const double * z);
bool treap_archive_dominates(const TreapArchive * self, const double * z);
int treap_archive_add(TreapArchive *self, const double * z, const void * x, bool check, TreapArchive **displaced);
void treap_archive_insert_displaced(TreapArchive *t, TreapArchive *incoming, bool *all_displaced);
void treap_archive_get_unique_vectors(const TreapArchive *t, const double ** z_list);
void treap_archive_get_x_values(const TreapArchive *t, const void ** x_list);
bool treap_archive_has_x_values(const TreapArchive *t);
void treap_archive_get_contents(const TreapArchive *t,  double * z_list, const void ** x_list);
size_t treap_archive_total_size(const TreapArchive *t);
size_t treap_archive_unique_size(const TreapArchive *t);

typedef struct TreapIterator TreapIterator;
TreapIterator *treap_archive_iter_new(const TreapArchive *);
int treap_archive_iter_next(TreapIterator *it, const double **z, const void **x);
void treap_archive_iter_free(TreapIterator *it);

// nd_tree.h
typedef struct FlexBucket FlexBucket;
void flex_bucket_free(FlexBucket * bucket);
size_t flex_bucket_unique_len(const FlexBucket * bucket);
size_t flex_bucket_total_len(const FlexBucket * bucket);
void flex_bucket_get_unique_vectors(const FlexBucket * bucket, const double ** z_list);
void flex_bucket_get_x_values(const FlexBucket * bucket, const void ** x_list);

typedef struct NDTreeConfig {
    uint8_t max_children;
    uint8_t max_bucket_size;
    bool allow_duplicates;
} NDTreeConfig;

typedef struct NDTreeArchive NDTreeArchive;
typedef struct NDTreeNode NDTreeNode;
NDTreeArchive * ndtree_new(dimension_t dim, uint8_t max_children, uint8_t max_bucket_size, bool allow_duplicates);
void ndtree_free(NDTreeArchive *self);
bool ndtree_has_x_values(const NDTreeArchive *t);
size_t ndtree_total_size(const NDTreeArchive *self);
size_t ndtree_unique_size(const NDTreeArchive *self);
bool ndtree_dominates(const NDTreeArchive *self, const double *z);
bool ndtree_dominated_by(const NDTreeArchive * self, const double * z);
int ndtree_add(NDTreeArchive *self, const double *z, const void *x, bool check, FlexBucket **displaced);
int ndtree_insert_displaced(NDTreeArchive *tree, FlexBucket **displaced_p, bool *all_displaced);
SolutionsList * ndtree_find_exact_vector(const NDTreeArchive *self, const double *z);
void ndtree_get_contents(const NDTreeArchive *self, double * z_list, const void ** x_list);
void ndtree_get_unique_vectors(const NDTreeArchive *self, const double ** z_list);
void ndtree_get_x_values(const NDTreeArchive *arch, const void ** x_list);

typedef struct NDTreeIterator NDTreeIterator;
NDTreeIterator *ndtree_iter_new(const NDTreeArchive *);
int ndtree_iter_next(NDTreeIterator *it, const double **z, const void **x);
void ndtree_iter_free(NDTreeIterator *it);


/*
typedef ... hype_sample_dist;
hype_sample_dist * hype_dist_unif_new(unsigned long seed);
hype_sample_dist * hype_dist_exp_new(double mu, unsigned long seed);
hype_sample_dist * hype_dist_gaussian_new(const double *mu, unsigned long int seed);
void hype_dist_free(hype_sample_dist * d);
double whv_hype_estimate(const double *points, size_t n, const double *ideal, const double *ref, hype_sample_dist * dist, size_t nsamples);
*/
