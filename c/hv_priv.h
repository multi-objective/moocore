#if !defined(HV_DIMENSION) || (HV_DIMENSION != 3 && HV_DIMENSION != 4)
#error "HV_DIMENSION must be 3 or 4"
#endif

#include <float.h> // DBL_MAX
#include <string.h> // memcpy
#include "sort.h"

// ----------------------- Data Structure -------------------------------------

/*
  With HV_DIMENSION==3, we have 'struct dlnode * next[1]' instead of 'struct dlnode * next'.
  The compiler should be able to remove the extra indirection.
*/

typedef struct dlnode {
    const double *x;            // point coordinates (objective vector).
    struct dlnode * next[HV_DIMENSION - 2]; /* keeps the points sorted according to coordinates 2,3 and 4
                                (in the case of 2 and 3, only the points swept by 4 are kept) */
    struct dlnode * prev[HV_DIMENSION - 2]; //keeps the points sorted according to coordinates 2 and 3 (except the sentinel 3)
    struct dlnode * cnext[2]; //current next
#if HV_DIMENSION == 4
    struct dlnode * closest[2]; // closest[0] == cx, closest[1] == cy
    unsigned int ndomr;    // number of dominators.
#endif
} dlnode_t;

static inline void
reset_sentinels(dlnode_t * list)
{
    dlnode_t * restrict s1 = list;
    dlnode_t * restrict s2 = list + 1;
    dlnode_t * restrict s3 = list + 2;

    s1->next[0] = s2;
    s1->prev[0] = s3;
#if HV_DIMENSION == 4
    s1->closest[0] = s2;
    s1->closest[1] = s1;
    s1->next[1] = s2;
    s1->prev[1] = s3;
#endif

    s2->next[0] = s3;
    s2->prev[0] = s1;
#if HV_DIMENSION == 4
    s2->closest[0] = s2;
    s2->closest[1] = s1;
    s2->next[1] = s3;
    s2->prev[1] = s1;
#endif

    s3->next[0] = s1;
    s3->prev[0] = s2;
#if HV_DIMENSION == 4
    s3->closest[0] = s2;
    s3->closest[1] = s1;
    s3->next[1] = s1;
    s3->prev[1] = s2;
#endif
}

static void
init_sentinels(dlnode_t * list, const double * ref)
{
    // Allocate the 3 sentinels of dimension dim.
    const double z[] = {
#if HV_DIMENSION == 3
        -DBL_MAX, ref[1], -DBL_MAX, // Sentinel 1
        ref[0], -DBL_MAX, -DBL_MAX, // Sentinel 2
        -DBL_MAX, -DBL_MAX, ref[2]  // Sentinel 2
#else
        -DBL_MAX, ref[1], -DBL_MAX, -DBL_MAX, // Sentinel 1
        ref[0], -DBL_MAX, -DBL_MAX, -DBL_MAX, // Sentinel 2
        -DBL_MAX, -DBL_MAX, ref[2], ref[3]    // Sentinel 2
#endif
    };

    double * x = malloc(sizeof(z));
    memcpy(x, z, sizeof(z));
    /* The list that keeps the points sorted according to the 3rd-coordinate
       does not really need the 3 sentinels, just one to represent (-inf, -inf,
       ref[2]).  But we need the other two to maintain a list of nondominated
       projections in the (x,y)-plane of points that is kept sorted according
       to the 1st and 2nd coordinates, and for that list we need two sentinels
       to represent (-inf, ref[1]) and (ref[0], -inf). */
    dlnode_t * restrict s1 = list;
    dlnode_t * restrict s2 = list + 1;
    dlnode_t * restrict s3 = list + 2;

    // Sentinel 1
    s1->x = x;
    // Initialize it when debugging so it will crash if uninitialized.
    DEBUG1(s1->cnext[0] = s1->cnext[1] = NULL);
    s1->next[0] = s2;
    s1->prev[0] = s3;
#if HV_DIMENSION == 4
    s1->closest[0] = s2;
    s1->closest[1] = s1;
    s1->next[1] = s2;
    s1->prev[1] = s3;
    s1->ndomr = 0;
#endif

    x += HV_DIMENSION;
    // Sentinel 2
    s2->x = x;
    DEBUG1(s2->cnext[0] = s2->cnext[1] = NULL);
    s2->next[0] = s3;
    s2->prev[0] = s1;
#if HV_DIMENSION == 4
    s2->closest[0] = s2;
    s2->closest[1] = s1;
    s2->next[1] = s3;
    s2->prev[1] = s1;
    s2->ndomr = 0;
#endif

    x += HV_DIMENSION;
    // Sentinel 3
    s3->x = x;
    DEBUG1(s3->cnext[0] = s3->cnext[1] = NULL);
    s3->next[0] = s1;
    s3->prev[0] = s2;
#if HV_DIMENSION == 4
    s3->closest[0] = s2;
    s3->closest[1] = s1;
    s3->next[1] = s1;
    s3->prev[1] = s2;
    s3->ndomr = 0;
#endif
}

#if HV_DIMENSION == 3 // Defined in hv3dplus.c
static inline void preprocessing(dlnode_t * list, size_t n);
#endif

static inline dlnode_t *
new_cdllist(size_t n, const double * ref)
{
    dlnode_t * list = (dlnode_t *) malloc((n + 3) * sizeof(*list));
    init_sentinels(list, ref);
    return list;
}

/*
 * Setup circular double-linked list in each dimension
 */
static inline dlnode_t *
setup_cdllist(const double * restrict data, size_t n, const double * restrict ref)
{
    ASSUME(n >= 1);
    const dimension_t dim = HV_DIMENSION;
    const double **scratch = malloc(n * sizeof(*scratch));
    size_t i, j;
    for (i = 0, j = 0; j < n; j++) {
        /* Filters those points that do not strictly dominate the reference
           point.  This is needed to ensure that the points left are only those
           that are needed to calculate the hypervolume. */
        if (likely(strongly_dominates(data + j * dim, ref, dim))) {
            scratch[i] = data + j * dim;
            i++;
        }
    }
    n = i; // Update number of points.
    if (likely(n > 1))
        qsort(scratch, n, sizeof(*scratch),
              (HV_DIMENSION == 3) ? cmp_double_asc_only_3d : cmp_double_asc_only_4d);

    dlnode_t * list = new_cdllist(n, ref);
    if (unlikely(n == 0)) {
        free(scratch);
        return list;
    }

    const dimension_t d = HV_DIMENSION - 3; // index within the list.
    dlnode_t * q = list+1;
    dlnode_t * list3 = list+3;
    assert(list->next[d] == list + 1);
    assert(q->next[d] == list + 2);
    for (i = 0, j = 0; j < n; j++) {
#if HV_DIMENSION == 4
        if (weakly_dominates(q->x, scratch[j], dim)) {
            /* print_point("q", q->x); */
            /* print_point("i", scratch[j]); */
            continue;
        }
#endif
        dlnode_t * p = list3 + i;
        p->x = scratch[j];
#if HV_DIMENSION == 4
        p->ndomr = 0;
        // Initialize it when debugging so it will crash if uninitialized.
        DEBUG1(p->closest[0] = p->closest[1] = NULL);
#endif
        DEBUG1(p->cnext[0] = p->cnext[1] = NULL);
         // Link the list in order.
        q->next[d] = p;
        p->prev[d] = q;
        q = p;
        i++;
    }
    n = i;
    free(scratch);
    assert((list3 + n - 1) == q);
    assert(list+2 == list->prev[d]);
    // q = last point, q->next = s3, s3->prev = last point
    q->next[d] = list+2;
    (list+2)->prev[d] = q;
#if HV_DIMENSION == 3
    preprocessing(list, n);
#endif
    return list;
}

static inline void
free_cdllist(dlnode_t * list)
{
    free((void*) list->x); // Free sentinels.
    free(list);
}

// ------------ Update data structure -----------------------------------------

// Link sentinels (-inf ref[1] -inf) and (ref[0] -inf -inf).
static inline void
restart_list_y(dlnode_t * list)
{
    assert(list+1 == list->next[0]);
    list->cnext[0] = list+1;
    (list+1)->cnext[1] = list;
}

static inline void
remove_from_z(dlnode_t * old)
{
    old->prev[0]->next[0] = old->next[0];
    old->next[0]->prev[0] = old->prev[0];
}


static inline double
compute_area_simple(const double * px, const dlnode_t * q, int i)
{
    ASSUME(i == 0 || i == 1);
    const int j = 1 - i;
    const dlnode_t * u = q->cnext[i];
    double area = (q->x[j] - px[j]) * (u->x[i] - px[i]);
#if HV_DIMENSION == 3
    assert(area > 0);
#endif
    while (px[j] < u->x[j]) {
        q = u;
        u = u->cnext[i];
        // With repeated coordinates, it can be zero.
        assert(u->x[i] - q->x[i] >= 0);
        area += (q->x[j] - px[j]) * (u->x[i] - q->x[i]);
    }
    return area;
}
