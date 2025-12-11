#ifndef _HV_PRIV_H
#define _HV_PRIV_H

#if !defined(HV_DIMENSION) || (HV_DIMENSION != 3 && HV_DIMENSION != 4)
#error "HV_DIMENSION must be 3 or 4"
#endif

#include <float.h> // DBL_MAX
#include <string.h> // memcpy
#include "sort.h"

// ----------------------- Data Structure -------------------------------------

/*
*/

typedef struct dlnode {
    const double * restrict x;  // point coordinates (objective vector).
#ifdef HV_RECURSIVE
    // In the recursive algorithm, the number of dimensions is not known in
    // advance, so next and prev cannot be fixed-size arrays.
    struct dlnode ** r_next;       // next-node vector for dimensions 5 and above.
    struct dlnode ** r_prev;       // previous-node vector for dimensions 5 and above.
    double * restrict area;      // partial area for dimensions 4 and above.
    double * restrict vol;       // partial volume for dimensions 4 and above.
#endif
    /* With HV_DIMENSION==3, we have 'struct dlnode * next[1]' instead of
       'struct dlnode * next'.  GCC -O3 is able to remove the extra
       indirection, so it is not worth having a special case.  */
    struct dlnode * next[HV_DIMENSION - 2]; /* keeps the points sorted according to coordinates 2,3 and 4
                                               (in the case of 2 and 3, only the points swept by 4 are kept) */
    struct dlnode * prev[HV_DIMENSION - 2]; //keeps the points sorted according to coordinates 2 and 3 (except the sentinel 3)

    struct dlnode * cnext[2]; //current next
#if HV_DIMENSION == 4 || defined(HVC_ONLY)
    struct dlnode * closest[2]; // closest[0] == cx, closest[1] == cy
    // FIXME: unused
    //unsigned int ndomr;    // number of dominators.
#endif
#ifdef HVC_ONLY
    double area, volume;
    double last_slice_z; // FIXME: Is this really needed?
    struct dlnode * head[2]; // lowest (x, y)
#endif
    // In hvc this is used as a boolean (duplicated or dominated)
    dimension_t ignore;          // [0, 255]
} dlnode_t;

// ------------ Update data structure -----------------------------------------

// Link sentinels (-inf ref[1] -inf) and (ref[0] -inf -inf).
static inline void
restart_list_y(dlnode_t * restrict list)
{
    assert(list+1 == list->next[0]);
    list->cnext[0] = list+1;
    (list+1)->cnext[1] = list;
}

#if HV_DIMENSION == 4 || defined(HVC_ONLY)
static inline void
set_cnext_to_closest(dlnode_t * restrict p)
{
    p->cnext[0] = p->closest[0];
    p->cnext[1] = p->closest[1];
    assert(p->cnext[0]);
    assert(p->cnext[1]);
}
#endif

static inline void
remove_from_z(dlnode_t * restrict old)
{
    old->prev[0]->next[0] = old->next[0];
    old->next[0]->prev[0] = old->prev[0];
}

/* -------------------- Preprocessing ---------------------------------------*/


_attr_maybe_unused static inline void
print_x(const dlnode_t * p)
{
    assert(p != NULL);
    const double * x = p->x;
    fprintf(stderr, "x: %g %g %g\n", x[0], x[1], x[2]);
}

#if HV_DIMENSION == 3
#include "hv3d_priv.h"
#endif // HV_DIMENSION == 3

// ------------------------ Circular double-linked list ----------------------

static inline void
reset_sentinels_3d(dlnode_t * restrict list)
{
    dlnode_t * restrict s1 = list;
    dlnode_t * restrict s2 = list + 1;
    dlnode_t * restrict s3 = list + 2;

    s1->next[0] = s2;
    s1->prev[0] = s3;

    s2->next[0] = s3;
    s2->prev[0] = s1;

    s3->next[0] = s1;
    s3->prev[0] = s2;

#if HV_DIMENSION == 4 || defined(HVC_ONLY)
    s1->closest[0] = s2;
    s1->closest[1] = s1;

    s2->closest[0] = s2;
    s2->closest[1] = s1;

    s3->closest[0] = s2;
    s3->closest[1] = s1;
#endif
}

static inline void
reset_sentinels(dlnode_t * restrict list)
{
    reset_sentinels_3d(list);

#if HV_DIMENSION == 4
    dlnode_t * restrict s1 = list;
    dlnode_t * restrict s2 = list + 1;
    dlnode_t * restrict s3 = list + 2;

    s1->next[1] = s2;
    s1->prev[1] = s3;

    s2->next[1] = s3;
    s2->prev[1] = s1;

    s3->next[1] = s1;
    s3->prev[1] = s2;
#endif
}

static inline void
init_sentinel(dlnode_t * restrict s, const double * restrict x)
{
    s->x = x;
    // Initialize it when debugging so it will crash if uninitialized.
    DEBUG1(s->cnext[0] = s->cnext[1] = NULL);
#if HV_DIMENSION == 4
    //s->ndomr = 0;
#endif
#ifdef HVC_ONLY
    s->ignore = false;
    s->volume = s->area = 0;
    s->head[0] = s->head[1] = s;
#endif
}

static void
init_sentinels(dlnode_t * restrict list, const double * restrict ref)
{
    // Allocate the 3 sentinels of dimension dim.
    const double z[] = {
#if HV_DIMENSION == 3
        -DBL_MAX, ref[1], -DBL_MAX, // Sentinel 1
        ref[0], -DBL_MAX, -DBL_MAX, // Sentinel 2
        -DBL_MAX, -DBL_MAX, ref[2]  // Sentinel 3
#elif HV_DIMENSION == 4
        -DBL_MAX, ref[1], -DBL_MAX, -DBL_MAX, // Sentinel 1
        ref[0], -DBL_MAX, -DBL_MAX, -DBL_MAX, // Sentinel 2
        -DBL_MAX, -DBL_MAX, ref[2], ref[3]    // Sentinel 3
#else
#  error "Unknown HV_DIMENSION"
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
    reset_sentinels(list);
    init_sentinel(list, x); // Sentinel 1
    init_sentinel(list+1, x + HV_DIMENSION); // Sentinel 2
    init_sentinel(list+2, x + 2 * HV_DIMENSION); // Sentinel 3
}

static inline dlnode_t *
new_cdllist(size_t n, const double * restrict ref)
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
        /* Filter those points that do not strictly dominate the reference
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
#ifdef HVC_ONLY
              // Lexicographic ordering ensures that we do not have dominated points in the AVL-tree.
              cmp_double_asc_rev_3d
#else
              (HV_DIMENSION == 3) ? cmp_double_asc_only_3d : cmp_double_asc_only_4d
#endif
            );

    dlnode_t * list = new_cdllist(n, ref);
    if (unlikely(n == 0)) {
        free(scratch);
        return list;
    }

    const dimension_t d = HV_DIMENSION - 3; // index within the list.
    assert(list->next[d] == list+1);
    dlnode_t * q = list+1;
    dlnode_t * list3 = list+3;
    assert(q->next[d] == list + 2);
    for (i = 0, j = 0; j < n; j++) {
        dlnode_t * p = list3 + i;
        p->x = scratch[j];
        DEBUG1(p->cnext[0] = p->cnext[1] = NULL);
#if HV_DIMENSION == 4 || defined(HVC_ONLY)
        //p->ndomr = 0;
        // Initialize it when debugging so it will crash if uninitialized.
        DEBUG1(p->closest[0] = p->closest[1] = NULL);
#endif
#ifdef HVC_ONLY
        p->ignore = false;
        // Initialize it when debugging so it will crash if uninitialized.
        DEBUG1(p->head[0] = p->head[1] = NULL);
#endif
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
    hv3d_preprocessing(list, n);
#endif
    return list;
}

static inline void
free_cdllist(dlnode_t * restrict list)
{
    free((void*) list->x); // Free sentinels.
    free(list);
}

/*
  Returns the area dominated by 'p', by sweeping points in ascending order of
  coordinate 'i' (which is either 0 or 1, i.e., x or y), starting from
  point 'q' and stopping when a point nondominated by 'p' and with coordinate
  'i' higher than that of 'p' on the (x,y)-plane is reached.
    p : The point whose contributions in 2D is to be computed.
    i : Dimension used for sweeping points (in ascending order).
    q : Outer delimiter of p (with lower 'i'-coordinate value than p) from
        which to start the sweep.
    u : The delimiter of p with lowest 'i'-coordinate which is not q. If p has
        inner delimiters, then u is the inner delimiter of p with lowest
        'i'-coordinate, otherwise, u is the outer delimiter with higher
        'i'-coordinate than p. Note: u is given because of the cases for
        which p has inner delimiter(s). When p does not have any inner delimiters
        then u=q->cnext[i].
*/
static inline double
compute_area_simple(const double * px, const dlnode_t * q, const dlnode_t * u, uint_fast8_t i)
{
    ASSUME(i == 0 || i == 1);
    const uint_fast8_t j = 1 - i;
    double area = (q->x[j] - px[j]) * (u->x[i] - px[i]);
#if HV_DIMENSION == 3 && !defined(HVC_ONLY)
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

/* Same as compute_area_simple() but for cases when p does not have any inner
   delimiters and u=q->cnext[i].  */
static inline double
compute_area_no_inners(const double * px, const dlnode_t * q, uint_fast8_t i)
{
    ASSUME(i == 0 || i == 1);
    return compute_area_simple(px, q, q->cnext[i], i);
}
#endif // _HV_PRIV_H
