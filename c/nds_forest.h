/*************************************************************************

 nds_forest.h: Non-dominated Sorting Forest

 ---------------------------------------------------------------------
                       Copyright (C) 2026
          Jonathan Fieldsend <J.E.Fieldsend@exeter.ac.uk>
          Manuel Lopez-Ibanez  <manuel.lopez-ibanez@manchester.ac.uk>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ---------------------------------------------------------------------

*************************************************************************/
#ifndef NDS_FOREST_H
#define NDS_FOREST_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "nd_tree.h"
#include "treap_archive.h"
#include "timer.h"

enum nds_forest_insertion_strategy_t {
    LINEAR = 0,
    FRONT_BINARY,
    PREVIOUS_RANK,
    ZERO_THEN_BINARY,
    DOMINANCE_GUIDED_BINARY,
    UNKNOWN_STRATEGY,
};

static inline int
parse_nds_forest_strategy(const char * optarg)
{
#define STREQUAL(S1, S2) (strncmp((S1), (S2), strlen(S2)) == 0)
#define STRATEGY_MATCH(STRING, ENUM_VALUE)                                     \
    if (STREQUAL(optarg, STRING)) return ENUM_VALUE

    STRATEGY_MATCH("linear", LINEAR);
    STRATEGY_MATCH("front-binary", FRONT_BINARY);
    STRATEGY_MATCH("previous-rank", PREVIOUS_RANK);
    STRATEGY_MATCH("zero-then-binary", ZERO_THEN_BINARY);
    STRATEGY_MATCH("dominance-guided-binary", DOMINANCE_GUIDED_BINARY);
    return UNKNOWN_STRATEGY;
#undef STRATEGY_MATCH
#undef STREQUAL
}

enum nds_forest_tree_type_t {
    NDS_FOREST_TREE_TREAP,
    NDS_FOREST_TREE_NDTREE,
};

typedef struct NDSForest {
    enum nds_forest_tree_type_t tree_type;
    // FIXME: Use a deque
    void *fronts; // Array of concrete tree objects, not pointers.
    size_t n;
    size_t cap;
    size_t last_rank;
    dimension_t dim;
    enum nds_forest_insertion_strategy_t insertion_strategy;
    double query_time, update_time;
    BaseArchive base;
    NDTreeConfig config; // Used only by NDTreeArchive.
    double last_objectives[]; // Flexible array.
} NDSForest;

typedef struct NDSForestIterator {
    const NDSForest *forest;
    union {
        NDTreeIterator * ndtree_it;
        TreapIterator  * treap_it;
    };
    size_t front_index;
    uint64_t revision; // To detect modifications of the tree while iterating.
} NDSForestIterator;


#define NDS_FOREST_DISPATCH(SELF, TREAP_CODE, NDTREE_CODE)                     \
    switch ((SELF)->tree_type) {                                               \
      case NDS_FOREST_TREE_TREAP:  do { TREAP_CODE; } while(0); break;         \
      case NDS_FOREST_TREE_NDTREE: do { NDTREE_CODE; } while(0); break;        \
      default: unreachable();                                                  \
    }

static inline size_t
nds_forest_sizeof_tree_type(const NDSForest * restrict self)
{
    NDS_FOREST_DISPATCH(self,
                        return sizeof(TreapArchive),
                        return sizeof(NDTreeArchive));
}

_attr_warn_unused_result
static inline NDSForest *
nds_forest_new(dimension_t dim,
               enum nds_forest_insertion_strategy_t strategy,
               enum nds_forest_tree_type_t tree_type,
               const NDTreeConfig *config, bool x_is_null, bool do_timing)
{
    if (strategy >= UNKNOWN_STRATEGY) {
        errprintf("Unknown insertion strategy for NDS Forest");
        return NULL;
    }

    if (dim < 2) {
        errprintf("Minimum number of dimensions is 2");
        return NULL;
    }

    if (tree_type == NDS_FOREST_TREE_TREAP && dim != 2) {
        errprintf("Treap trees only support dimension 2");
        return NULL;
    }

    if (tree_type != NDS_FOREST_TREE_TREAP
        && tree_type != NDS_FOREST_TREE_NDTREE) {
        errprintf("Unknown tree type");
        return NULL;
    }

    /* FIXME: last_objectives could be just a pointer to the actual last point,
       because the last inserted point has to be in one tree and cannot be
       erased without resetting last_objectives.  */
    NDSForest *self = malloc(sizeof(*self) + dim * sizeof(*self->last_objectives));
    if (!self)
        return NULL;

    self->tree_type = tree_type;
    self->cap = 64;
    self->fronts = malloc(self->cap * nds_forest_sizeof_tree_type(self));
    if (self->fronts == NULL) {
        free(self);
        return NULL;
    }

    for (dimension_t i = 0; i < dim; i++)
        self->last_objectives[i] = INFINITY;

    self->n = 0;
    self->last_rank = SIZE_MAX;
    self->dim = dim;
    self->insertion_strategy = strategy;
    if (tree_type == NDS_FOREST_TREE_NDTREE)
        self->config = *config;
    self->base.revision = 0;
    self->base.keep_duplicates = true;
    self->base.x_is_null = x_is_null;
    self->query_time = (do_timing) ? 0 : -1;
    self->update_time = self->query_time;
    return self;
}

static inline Timer_t
nds_forest_timer_start(const NDSForest * self)
{
    if (likely(self->query_time < 0))
        return (Timer_t) { 0 }; // unused.
    return timer_start(VIRTUAL_TIME);
}

static inline void
nds_forest_timer_finish_query(NDSForest * self, Timer_t * timer)
{
    if (likely(self->query_time < 0))
        return;
    self->query_time += timer_reset(timer);
}

static inline void
nds_forest_timer_finish_update(NDSForest * self, Timer_t * timer)
{
    ASSUME((self->query_time < 0) == (self->update_time < 0));
    if (likely(self->update_time < 0))
        return;
    self->update_time += timer_elapsed(timer);
}

static inline void
nds_forest_get_timings(const NDSForest * self, double * query_time, double * update_time)
{
    *query_time = self->query_time;
    *update_time = self->update_time;
}

static inline NDSForest *
nds_forest_ndtree_new(dimension_t dim,
                      enum nds_forest_insertion_strategy_t strategy,
                      uint8_t max_children, uint8_t max_bucket_size,
                      bool x_is_null, bool do_timing)
{
    NDTreeConfig config = { .max_children = max_children, .max_bucket_size = max_bucket_size };
    return nds_forest_new(dim, strategy, NDS_FOREST_TREE_NDTREE, &config, x_is_null, do_timing);
}

static inline NDSForest *
nds_forest_treap_new(enum nds_forest_insertion_strategy_t strategy, bool x_is_null, bool do_timing)
{
    return nds_forest_new(2, strategy, NDS_FOREST_TREE_TREAP, NULL, x_is_null, do_timing);
}

static inline void *
nds_forest_front_at(NDSForest * restrict self, size_t rank)
{
    return (unsigned char *)self->fronts + rank * nds_forest_sizeof_tree_type(self);
}

static inline const void *
nds_forest_front_at_const(const NDSForest * restrict self, size_t rank)
{
    return (const unsigned char *)self->fronts + rank * nds_forest_sizeof_tree_type(self);
}

static inline void
nds_forest_front_free_nodes(NDSForest * restrict self, size_t rank)
{
    NDS_FOREST_DISPATCH(self,
                        treap_archive_free_nodes(nds_forest_front_at(self, rank)),
                        ndtree_free_nodes(nds_forest_front_at(self, rank)));
}

static inline void
nds_forest_free(NDSForest * restrict self)
{
    if (!self)
        return;

    for (size_t i = 0; i < self->n; i++)
        nds_forest_front_free_nodes(self, i);

    free(self->fronts);
    free(self);
}

static inline size_t
nds_forest_len(const NDSForest *self)
{
    return self->n;
}

static inline size_t
nds_forest_last_rank(const NDSForest *self)
{
    return self->last_rank;
}

static inline size_t
nds_forest_front_total_size(const NDSForest *self, size_t rank)
{
    assert(rank < self->n);
    if (rank >= self->n)
        return SIZE_MAX;

    NDS_FOREST_DISPATCH(
        self,
        return treap_archive_total_size(nds_forest_front_at_const(self, rank)),
        return ndtree_total_size(nds_forest_front_at_const(self, rank)));
}

static inline size_t
nds_forest_total_size(const NDSForest *self)
{
    size_t n_fronts = nds_forest_len(self);
    size_t total = 0;
    for (size_t i = 0; i < n_fronts; i++)
        total += nds_forest_front_total_size(self, i);
    return total;
}

static inline size_t
nds_forest_front_unique_size(const NDSForest *self, size_t rank)
{
    assert(rank < self->n);
    if (rank >= self->n)
        return SIZE_MAX;

    NDS_FOREST_DISPATCH(
        self,
        return treap_archive_unique_size(nds_forest_front_at_const(self, rank)),
        return ndtree_unique_size(nds_forest_front_at_const(self, rank)));
}

static inline size_t
nds_forest_unique_size(const NDSForest *self)
{
    size_t n_fronts = nds_forest_len(self);
    size_t total = 0;
    for (size_t i = 0; i < n_fronts; i++)
        total += nds_forest_front_unique_size(self, i);
    return total;
}

static inline bool
nds_forest_dominates(const NDSForest * restrict self, size_t rank, const double *z)
{
    if (rank >= self->n)
        return false;

    NDS_FOREST_DISPATCH(
        self,
        return treap_archive_dominates(nds_forest_front_at_const(self, rank), z),
        return ndtree_dominates(nds_forest_front_at_const(self, rank), z));
}

static inline bool
nds_forest_dominated_by(const NDSForest *self, size_t rank, const double *z)
{
    if (rank >= self->n)
        return false;

    NDS_FOREST_DISPATCH(
        self,
        return treap_archive_dominated_by(nds_forest_front_at_const(self, rank), z),
        return ndtree_dominated_by(nds_forest_front_at_const(self, rank), z));
}


static inline int
nds_forest_add_to_front(NDSForest * restrict self, size_t rank,
                        const double * restrict z, const void * restrict x,
                        bool check, void **displaced)
{
    NDS_FOREST_DISPATCH(
        self,
        return treap_archive_add(nds_forest_front_at(self, rank), z, x, check, (TreapArchive **)displaced),
        return ndtree_add(nds_forest_front_at(self, rank), z, x, check, (FlexBucket **)displaced));
}

static inline size_t
nds_forest_displaced_unique_size(const NDSForest * restrict self,
                                 const void * restrict displaced)
{
    NDS_FOREST_DISPATCH(
        self,
        return treap_archive_unique_size((const TreapArchive *)displaced),
        return flex_bucket_unique_len((const FlexBucket *)displaced));
}

static inline void
nds_forest_displaced_free(const NDSForest * restrict self, void * restrict displaced)
{
    NDS_FOREST_DISPATCH(
        self,
        treap_archive_free((TreapArchive *)displaced),
        flex_bucket_free((FlexBucket *)displaced));
}

static inline bool
nds_forest_front_init_with_single_solution(NDSForest * restrict self, size_t rank,
                                           const double * restrict z,
                                           const void * restrict x)
{
    assert((x == NULL) == self->base.x_is_null);
    NDS_FOREST_DISPATCH(
        self,
        return treap_archive_init_with_single_solution(nds_forest_front_at(self, rank),
                                                       z, x,
                                                       /*keep_duplicates=*/true),
        return ndtree_init_with_single_solution(nds_forest_front_at(self, rank),
                                                z, x, self->dim, (const NDTreeConfig *)&self->config,
                                                /*keep_duplicates=*/true));
}

static inline bool
nds_forest_front_init_with_displaced(NDSForest * restrict self, size_t rank, void * restrict displaced)
{
    NDS_FOREST_DISPATCH(
        self,
        return treap_archive_move(nds_forest_front_at(self, rank), (TreapArchive *)displaced),
        return ndtree_init_with_displaced(nds_forest_front_at(self, rank), (FlexBucket *)displaced,
                                          self->dim, (const NDTreeConfig *)&self->config,
                                          /*keep_duplicates=*/true,
                                          /*x_is_null=*/self->base.x_is_null));
}

/**
   Insert a new empty front at rank.

   This function may reallocate the memory of self->fronts and return a new pointer.
*/
_attr_warn_unused_result
static inline bool
nds_forest_insert_front(NDSForest * restrict self, size_t rank)
{
    const size_t sizeof_tree_type = nds_forest_sizeof_tree_type(self);

    if (self->n == self->cap) {
        size_t new_cap = self->cap * 2;
        void * fronts = realloc(self->fronts, new_cap * sizeof_tree_type);
        if (fronts == NULL)
            return false;
        self->fronts = fronts;
        self->cap = new_cap;
    }

    if (rank < self->n) {
        size_t size =  sizeof_tree_type * (self->n - rank);
        unsigned char *src = (unsigned char *) nds_forest_front_at(self, rank);
        unsigned char *dst = src + sizeof_tree_type;
        memmove(dst, src, size);
        // If we insert a new front in the middle, last_rank becomes invalid.
        if (self->last_rank >= rank)
            self->last_rank = SIZE_MAX;
    }

    self->n++;
    assert(rank < self->n);
    return true;
}

/**
   Insert a new front containing a single entry at rank.

   Updates last_rank.
*/
_attr_warn_unused_result
static inline int
nds_forest_insert_singleton_front(NDSForest * restrict self, size_t rank,
                                  const double * restrict z,
                                  const void * restrict x)
{
    if (!nds_forest_insert_front(self, rank))
        return ARCHIVE_INSERT_MEMORY_ERROR;

    self->last_rank = rank;
    memcpy(self->last_objectives, z, self->dim * sizeof(*z));

    bool ok = nds_forest_front_init_with_single_solution(self, rank, z, x);
    assert(nds_forest_front_unique_size(self, rank) > 0);
    return ok ? ARCHIVE_INSERT_ACCEPTED : ARCHIVE_INSERT_MEMORY_ERROR;
}

/**
   Insert a new front containing a displaced tree.

   Updates last_rank.
*/
_attr_warn_unused_result
static inline int
nds_forest_insert_displaced_front(NDSForest *self, size_t rank, void ** restrict displaced)
{
    if (!nds_forest_insert_front(self, rank))
        return ARCHIVE_INSERT_MEMORY_ERROR;

    bool ok = nds_forest_front_init_with_displaced(self, rank, *displaced);
    assert(nds_forest_front_unique_size(self, rank) > 0);
    *displaced = NULL;
    return ok ? ARCHIVE_INSERT_ACCEPTED : ARCHIVE_INSERT_MEMORY_ERROR;
}

static inline void
nds_forest_front_insert_displaced(NDSForest * restrict self, size_t rank,
                                  void ** restrict displaced,
                                  bool * restrict all_displaced)
{
    NDS_FOREST_DISPATCH(
        self,
        treap_archive_insert_displaced_p(nds_forest_front_at(self, rank), (TreapArchive **) displaced, all_displaced),
        ndtree_insert_displaced(nds_forest_front_at(self, rank), (FlexBucket **)displaced, all_displaced));
}

/**
   check: If true, check if FOREST_FUNC(_dominates)(self, rank, z) then z is rejected.

   Updates last_rank.
*/
_attr_warn_unused_result
static inline bool
nds_forest_cascade(NDSForest * restrict self, size_t rank,
                   const double * restrict z, const void * restrict x)
{
    assert(rank <= self->n);
    // Insert a new front at the end, no need to cascade.
    if (rank == self->n)
        return nds_forest_insert_singleton_front(self, rank, z, x);

    self->last_rank = rank;
    memcpy(self->last_objectives, z, self->dim * sizeof(*z));
    assert(!nds_forest_dominates(self, rank, z));

    void *displaced = NULL;
    _attr_maybe_unused size_t prev_size = nds_forest_front_unique_size(self, rank);
    assert(prev_size > 0);
    int res = nds_forest_add_to_front(self, rank, z, x, /*check=*/false, &displaced);
    assert(res == ARCHIVE_INSERT_ACCEPTED || res == ARCHIVE_INSERT_DUPLICATED);
    if (res == ARCHIVE_INSERT_DUPLICATED)
        return res;

    assert(nds_forest_front_unique_size(self, rank) > 0);
    if (nds_forest_front_unique_size(self, rank) == 1) {
        /* If we inserted a new vector, but there is only one vector, it means
           everything was displaced. */
        assert(displaced != NULL
               && nds_forest_displaced_unique_size(self, displaced) > 0
               && prev_size == nds_forest_displaced_unique_size(self, displaced));
        rank++;
        res = nds_forest_insert_displaced_front(self, rank, &displaced);
        assert(displaced == NULL);
        return res;
    }

    bool all_displaced = false;
    while (displaced && nds_forest_displaced_unique_size(self, displaced)) {
        rank++;
        if (all_displaced || rank == self->n) {
            // The whole front was displaced or we reached the last one.
            res = nds_forest_insert_displaced_front(self, rank, &displaced);
            assert(displaced == NULL);
            return res;
        }
        nds_forest_front_insert_displaced(self, rank, &displaced, &all_displaced);
        assert(nds_forest_front_unique_size(self, rank) > 0);
    }

    assert(displaced == NULL
           || nds_forest_displaced_unique_size(self, displaced) == 0);
    if (displaced)
        nds_forest_displaced_free(self, displaced);
    return ARCHIVE_INSERT_ACCEPTED;
}

/**
   z_list must be an array of double allocated with sufficient space. Vectors
   are copied to this array.

   x_list must be of array of void *. Only the pointers are copied.
*/
static inline void
nds_forest_front_get_contents(const NDSForest * restrict self, size_t rank, double * z_list, const void ** x_list)
{
    NDS_FOREST_DISPATCH(
        self,
        treap_archive_get_contents(nds_forest_front_at_const(self, rank), z_list, x_list),
        ndtree_get_contents(nds_forest_front_at_const(self, rank), z_list, x_list));
}

static inline size_t
nds_forest_find_exact_vector(const NDSForest * restrict self, const double * restrict z)
{
    // FIXME: We could use the dominance properties to do a binary search.
    for (size_t rank = 0; rank < self->n; rank++) {
        NDS_FOREST_DISPATCH(self,
                            if (treap_archive_find_exact_vector(nds_forest_front_at_const(self, rank), z)) return rank,
                            if (ndtree_find_exact_vector(nds_forest_front_at_const(self, rank), z)) return rank);
    }
    return (size_t)-1; // NOT FOUND
}


/* -------------------------------------------------------------------------
 * Insertion strategies
 * ------------------------------------------------------------------------- */

/**
   Returns rank == self->n || (rank < self->n && !nds_forest_dominates(self, rank, z))
*/
static inline size_t
nds_forest_find_insertion_linear(const NDSForest * restrict self, const double * restrict z)
{
    size_t rank = 0;
    while (nds_forest_dominates(self, rank, z))
        rank++;
    return rank;
}

static inline size_t
nds_forest_find_insertion_bsearch(const NDSForest * restrict self, const double * restrict z,
                                  size_t lowest, size_t highest)
{
    size_t low = lowest, high = highest;
    while (low < high) {
        size_t mid = low + (high - low) / 2;
        if (nds_forest_dominates(self, mid, z))
            low = mid + 1;
        else
            high = mid;
    }
    assert((low == 0 && self->n == 0)
           // z is dominated by highest - 1
           || (low == highest && nds_forest_dominates(self, low - 1, z))
           // z is dominated by low - 1, but not dominated by low
           || (lowest < low && low < highest && nds_forest_dominates(self, low - 1, z)
               && !nds_forest_dominates(self, low, z))
           // z is not dominated by anything in the range
           || (lowest == low && !nds_forest_dominates(self, low, z)));
    return low;
}

static inline size_t
nds_forest_find_insertion_front_binary(const NDSForest * restrict self, const double * restrict z)
{
    return nds_forest_find_insertion_bsearch(self, z, 0, self->n);
}

static inline size_t
nds_forest_find_insertion_previous_rank(const NDSForest * restrict self, const double * restrict z)
{
    if (self->n == 0)
        return 0;

    assert(self->last_rank < self->n);
    size_t rank = self->last_rank;
    if (nds_forest_dominates(self, rank, z)) {
        do {
            rank++;
        } while (rank < self->n && nds_forest_dominates(self, rank, z));
    } else {
        while (rank > 0 && !nds_forest_dominates(self, rank - 1, z))
            rank--;

        assert((rank == 0 && !nds_forest_dominates(self, 0, z))
               || (rank > 0 && nds_forest_dominates(self, rank - 1, z)));
    }
    return rank;
}

static inline size_t
nds_forest_find_insertion_zero_then_binary(const NDSForest * restrict self, const double * restrict z)
{
    return nds_forest_dominates(self, 0, z)
        ? nds_forest_find_insertion_bsearch(self, z, 1, self->n)
        : 0;
}

/**
   Bound binary search using dominance against the previously inserted point.
*/
_attr_warn_unused_result
static inline int
nds_forest_insert_strategy_dominance_guided_binary(NDSForest * restrict self, const double * restrict z, const void * restrict x)
{
    Timer_t timer = nds_forest_timer_start(self);

    size_t rank;
    if (unlikely(self->n == 0)) {
        rank = 0;
    } else if (weakly_dominates(z, self->last_objectives, self->dim)) {
        if (unlikely(all_equal_double(z, self->last_objectives, self->dim))) {
            assert(self->last_rank < self->n);
            // There is no cascade here, so insert directly.
            size_t last_rank = self->last_rank;
            void *displaced = NULL;
            nds_forest_timer_finish_query(self, &timer);
            _attr_maybe_unused int res =
                nds_forest_add_to_front(self, last_rank, z, x, /*check=*/false, &displaced);
            nds_forest_timer_finish_update(self, &timer);
            assert(nds_forest_front_unique_size(self, last_rank) > 0);
            assert(res == ARCHIVE_INSERT_DUPLICATED && displaced == NULL);
            return ARCHIVE_INSERT_DUPLICATED;
        }
        // new dominates previous.
        assert(self->last_rank < self->n);
        rank = nds_forest_dominates(self, 0, z)
            ? nds_forest_find_insertion_bsearch(self, z, 1, self->last_rank + 1)
            : 0;
        assert(rank < self->n && !nds_forest_dominates(self, rank, z));
        ASSUME(rank < self->n);
    } else if (weakly_dominates(self->last_objectives, z, self->dim)) {
        assert(self->last_rank < self->n);
        // previous weakly dominates new
        rank = self->last_rank + 1;
        if (nds_forest_dominates(self, rank, z))
            rank = nds_forest_find_insertion_bsearch(self, z, rank + 1, self->n);
    } else { // previous and new are mutually nondominated
        rank = nds_forest_find_insertion_zero_then_binary(self, z);
    }
    nds_forest_timer_finish_query(self, &timer);
    int res = nds_forest_cascade(self, rank, z, x);
    nds_forest_timer_finish_update(self, &timer);
    return res;
}

/* -------------------------------------------------------------------------
 * Top-level insertion entry point
 * ------------------------------------------------------------------------- */

static inline int
nds_forest_add(NDSForest * restrict self, const double * restrict z, const void * restrict x)
{
    ASSUME(self->insertion_strategy < UNKNOWN_STRATEGY);
    self->base.revision++;

    size_t rank;
    int res;
#if DEBUG >= 1
    // All strategies should return the same rank.
    rank = nds_forest_find_insertion_linear(self, z);
    assert(rank == nds_forest_find_insertion_front_binary(self, z));
    assert(rank == nds_forest_find_insertion_previous_rank(self, z));
    assert(rank == nds_forest_find_insertion_zero_then_binary(self, z));
    res = nds_forest_insert_strategy_dominance_guided_binary(self, z, x);
    assert(rank == self->last_rank);
    return res;
#endif

    Timer_t timer;
    if (self->insertion_strategy != DOMINANCE_GUIDED_BINARY)
        timer = nds_forest_timer_start(self);

    switch (self->insertion_strategy) {
    case LINEAR:
        rank = nds_forest_find_insertion_linear(self, z);
        break;
    case FRONT_BINARY:
        rank = nds_forest_find_insertion_front_binary(self, z);
        break;
    case PREVIOUS_RANK:
        rank = nds_forest_find_insertion_previous_rank(self, z);
        break;
    case ZERO_THEN_BINARY:
        rank = nds_forest_find_insertion_zero_then_binary(self, z);
        break;
    case DOMINANCE_GUIDED_BINARY:
        return nds_forest_insert_strategy_dominance_guided_binary(self, z, x);
    default:
        return ARCHIVE_INSERT_UKNOWN_ERROR;
    }
    nds_forest_timer_finish_query(self, &timer);
    res = nds_forest_cascade(self, rank, z, x);
    nds_forest_timer_finish_update(self, &timer);
    return res;
}

NDSForestIterator *
nds_forest_iter_new(const NDSForest *forest)
{
    if (forest == NULL)
        return NULL;

    NDSForestIterator * it = malloc(sizeof(*it));
    if (it == NULL)
        return NULL;

    // Init the iterator
    it->forest = forest;
    it->front_index = 0;
    it->revision = forest->base.revision;
    NDS_FOREST_DISPATCH(forest,
                        it->treap_it = treap_archive_iter_new(nds_forest_front_at_const(forest, 0)),
                        it->ndtree_it = ndtree_iter_new(nds_forest_front_at_const(forest, 0)));
    return it;
}

void
nds_forest_iter_free(NDSForestIterator * it)
{
    if (it == NULL)
        return;
    NDS_FOREST_DISPATCH(it->forest,
                        treap_archive_iter_free(it->treap_it),
                        ndtree_iter_free(it->ndtree_it));
    free(it);
}

/**
   1  next element returned
   0  end of iteration
   -1  error
*/
int
nds_forest_iter_next(NDSForestIterator * restrict it, size_t * restrict rank,
                     double * restrict z, const void ** restrict x)
{
    if (it == NULL || rank == NULL || z == NULL || it->revision != it->forest->base.revision)
        return -1;

    int res;
    NDS_FOREST_DISPATCH(it->forest,
                        res = treap_archive_iter_next(it->treap_it, z, x),
                        res = ndtree_iter_next(it->ndtree_it, z, x));
    if (res > 0) {
        *rank = it->front_index;
        return 1;
    }
    if (res == 0) {
        it->front_index++;
        if (it->front_index == it->forest->n)
            return 0;
        const void * front = nds_forest_front_at_const(it->forest, it->front_index);
        NDS_FOREST_DISPATCH(it->forest,
                            treap_archive_iter_init(it->treap_it, front),
                            ndtree_iter_init(it->ndtree_it, front));
        NDS_FOREST_DISPATCH(it->forest,
                            res = treap_archive_iter_next(it->treap_it, z, x),
                            res = ndtree_iter_next(it->ndtree_it, z, x));
        *rank = it->front_index;
        assert(res == 1);
        return res;
    }
    return -1;
}

#endif /* NDS_FOREST_H */
