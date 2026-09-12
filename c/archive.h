#ifndef _ARCHIVE_H_
#define _ARCHIVE_H_

#include "treap_archive.h"
#include "nd_tree.h"

enum unbounded_archive_type_t {
    UNBOUNDED_ARCHIVE_TREAP,
    UNBOUNDED_ARCHIVE_NDTREE,
};

typedef struct UnboundedArchive {
    enum unbounded_archive_type_t archive_type;
    union {
        TreapArchive treap;
        NDTreeArchive ndtree;
    };
} UnboundedArchive;

#define UNBOUNDED_ARCHIVE_DISPATCH(SELF, TREAP_CODE, NDTREE_CODE)              \
    switch ((SELF)->archive_type) {                                            \
      case UNBOUNDED_ARCHIVE_TREAP:  do { TREAP_CODE; } while(0); break;       \
      case UNBOUNDED_ARCHIVE_NDTREE: do { NDTREE_CODE; } while(0); break;      \
      default: unreachable();                                                  \
    }


static inline UnboundedArchive *
archive_new(enum unbounded_archive_type_t type)
{
    UnboundedArchive * arch = malloc(sizeof(*arch));
    arch->archive_type = type;
    return arch;

}
static inline UnboundedArchive *
archive_ndtree_new(dimension_t dim, NDTreeConfig config)
{
    UnboundedArchive * archive = archive_new(UNBOUNDED_ARCHIVE_NDTREE);
    ndtree_init(&archive->ndtree, dim, config.max_children, config.max_bucket_size, config.allow_duplicates);
    return archive;
}

static inline UnboundedArchive *
archive_treap_new(void)
{
    UnboundedArchive * archive = archive_new(UNBOUNDED_ARCHIVE_TREAP);
    treap_archive_init(&archive->treap);
    return archive;
}

/**

*/
static inline int
archive_add(UnboundedArchive * restrict arch, const double * restrict z, const void * restrict x)
{
    UNBOUNDED_ARCHIVE_DISPATCH(
        arch,
        return treap_archive_add_discard(&arch->treap, z, x),
        return ndtree_add_discard(&arch->ndtree, z, x));
}

static inline size_t
archive_total_size(const UnboundedArchive *arch)
{
    UNBOUNDED_ARCHIVE_DISPATCH(
        arch,
        return treap_archive_total_size(&arch->treap),
        return ndtree_total_size(&arch->ndtree));
}

static inline size_t
archive_unique_size(const UnboundedArchive *arch)
{
    UNBOUNDED_ARCHIVE_DISPATCH(
        arch,
        return treap_archive_unique_size(&arch->treap),
        return ndtree_unique_size(&arch->ndtree));
}

static inline void
archive_get_contents(const UnboundedArchive *arch, double * z_list, const void ** x_list)
{
    UNBOUNDED_ARCHIVE_DISPATCH(
        arch,
        treap_archive_get_contents(&arch->treap, z_list, x_list),
        ndtree_get_contents(&arch->ndtree, z_list, x_list));
}

static inline void
archive_free(UnboundedArchive *arch)
{
    UNBOUNDED_ARCHIVE_DISPATCH(
        arch,
        treap_archive_free_nodes(&arch->treap),
        ndtree_free_nodes(&arch->ndtree));
    free(arch);
}

static inline void
archive_print_stats(FILE * stream, const UnboundedArchive *arch)
{
    UNBOUNDED_ARCHIVE_DISPATCH(
        arch,
        (void)0,
        ndtree_print_stats(stream, &arch->ndtree));
}

#endif // _ARCHIVE_H_
