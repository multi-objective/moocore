#ifndef FLEX_BUCKET_H_
#define FLEX_BUCKET_H_

#include "solutions_list.h"

typedef struct FlexBucket {
    size_t n_unique;
    size_t n_total;
    size_t cap;
    SolutionsList solutions[]; // List of solutions (each list tracks duplicates).
} FlexBucket;

static FlexBucket *
flex_bucket_new(size_t cap)
{
    cap = cap > 0 ? cap : 4;
    FlexBucket * bucket = malloc(sizeof*bucket + cap * sizeof(SolutionsList));
    if (bucket) {
        bucket->cap = cap;
        bucket->n_unique = 0;
        bucket->n_total = 0;
    }
    return bucket;
}

static FlexBucket *
flex_bucket_reserve(FlexBucket * bucket, size_t add_cap)
{
    if (bucket == NULL)
        return flex_bucket_new(MAX(add_cap, (size_t)4)); // Use same default cap within flex_bucket_new()

    size_t new_cap = bucket->n_unique + add_cap;
    if (new_cap <= bucket->cap)
        return bucket;
    new_cap = MAX(new_cap, bucket->cap * 2U);
    FlexBucket * new_bucket = realloc(bucket, sizeof *bucket + new_cap * sizeof(SolutionsList));
    if (!new_bucket)
        return NULL;
    new_bucket->cap = new_cap;
    return new_bucket;
}

/**
   Copy n_src elements of src to the bucket, possibly re-allocating its memory.

   On return, for each element of src, src.n=0, src[i].x.list = NULL.
*/
static size_t
flex_bucket_append_move(FlexBucket ** bucket_p, SolutionsList *src, uint32_t n_src)
{
    FlexBucket * bucket = flex_bucket_reserve(*bucket_p, n_src);
    if (!bucket)
        return 0;
    memcpy(&bucket->solutions[bucket->n_unique], src, n_src * sizeof *src);
    size_t total = 0;
    for (uint32_t i = 0; i < n_src; i++) {
        total += src[i].x.n;
        // Reset src to detect double copy/free.
        src[i].x.n = 0;
        src[i].z = NULL;
        src[i].x.list = NULL;
    }
    bucket->n_total += total;
    bucket->n_unique += n_src;
    *bucket_p = bucket;
    return total;
}

static inline size_t
flex_bucket_unique_len(const FlexBucket * bucket)
{
    return bucket->n_unique;
}

static inline size_t
flex_bucket_total_len(const FlexBucket * bucket)
{
    return bucket->n_total;
}

static inline void
flex_bucket_get_unique_vectors(const FlexBucket * bucket, const double ** z_list)
{
    for (size_t i = 0; i < bucket->n_unique; i++, z_list++)
        z_list[0] = bucket->solutions[i].z;
}

static inline void
flex_bucket_get_x_values(const FlexBucket * bucket, const void ** x_list)
{
    for (size_t i = 0; i < bucket->n_unique; i++)
        for (size_t j = 0; j < bucket->solutions[i].x.n; j++, x_list++)
            x_list[0] = bucket->solutions[i].x.list[j];
}

static void
flex_bucket_free(FlexBucket * bucket)
{
    for (size_t i = 0; i < bucket->n_unique; i++)
        solutions_list_free(&bucket->solutions[i]);
    free(bucket);
}

#endif // FLEX_BUCKET_H_
