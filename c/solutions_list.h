#ifndef SOLUTIONS_LIST_H_
#define SOLUTIONS_LIST_H_

#include <stdint.h>
#include <string.h> // memcpy
#include <assert.h>

typedef struct VoidList {
    uint32_t n;
    uint32_t cap;
    const void ** list;
} VoidList;

/**
   A duplicate-aware leaf bucket stores one unique objective vector plus all
   payloads that share it, so duplicate-heavy leaves are checked once per
   distinct objective vector rather than once per stored solution.
*/
typedef struct SolutionsList {
    const double * z;  // point coordinates (objective vector).
    // FIXME: Compile with -fms-extensions to avoid duplication and use:
    // struct VoidList;
    VoidList x;
} SolutionsList;

static inline bool
void_list_init(VoidList * restrict item, const void * restrict x)
{
    item->cap = 1;
    item->n = 1;
    if (x == NULL) {
        item->list = NULL;
    } else {
        item->list = (const void **) malloc(item->cap * sizeof(*item->list));
        if (item->list == NULL)
            return false;
        item->list[0] = x;
    }
    return true;
}

static inline bool
solutions_list_init(SolutionsList * restrict item, const double * restrict z, const void * restrict x, dimension_t dim)
{
    assert(dim >= 2);
    // FIXME: We should have a pool of z vectors or make z a flexible array.
    double * new_z = malloc(dim * sizeof(*new_z));
    if (new_z == NULL)
        return false;
    item->z = memcpy(new_z, z, dim * sizeof(*z));
    return void_list_init(&item->x, x);
}

static inline void
void_list_free(VoidList * item)
{
    item->n = 0;
    // item->n may be > 0 if we stored repeated z values but no x values.
    if (!item->list)
        return;
    free(item->list);
    item->list = NULL;
}

static inline void
solutions_list_free(SolutionsList * item)
{
    free((void *)item->z);
    item->z = NULL;
    void_list_free(&item->x);
}

/**
   Add a duplicated point.
*/
static inline bool
void_list_append(VoidList * restrict item, const void * restrict x)
{
    if (x == NULL) {
        item->n++;
        return true;
    }

    if (item->n == item->cap) {
        assert(item->cap < UINT32_MAX / 2);
        uint32_t new_cap = item->cap * 2;
        const void ** new_list = (const void **) realloc(item->list, new_cap * sizeof(*new_list));
        if (new_list == NULL)
            return false;

        item->list = new_list;
        item->cap = new_cap;
    }
    item->list[item->n++] = x;
    return true;
}

/**
   Add a duplicated point.
*/
static inline bool
solutions_list_append(SolutionsList * restrict item, const void * restrict x)
{
    assert(item->z != NULL);
    return void_list_append(&item->x, x);
}


static inline void
void_list_get_x_values(const VoidList * item, const void *** x_list)
{
    for (unsigned i = 0; i < item->n; i++, (*x_list)++) {
        assert(item->list[i] != NULL); // Storing NULL values is a bug.
        **x_list = item->list[i];
    }
}

static inline void
solutions_list_get_x_values(const SolutionsList * item, const void *** x_list)
{
    void_list_get_x_values(&item->x, x_list);
}

static inline void
solutions_list_get_contents(const SolutionsList * item,
                            double ** z_list, const void *** x_list,
                            dimension_t dim)
{
    for (unsigned i = 0; i < item->x.n; i++, *z_list += dim) {
        // Duplicate z for each x
        memcpy(*z_list, item->z, dim * sizeof **z_list);
    }
    if (x_list != NULL)
        void_list_get_x_values(&item->x, x_list);
}


/**
   Shallow copy.
*/
static inline void
solutions_list_copy(SolutionsList * restrict dst, const SolutionsList * restrict src)
{
    memcpy(dst, src, sizeof *src);
}



#endif // SOLUTIONS_LIST_H_
