/*************************************************************************

 treap-archive.h: 2D Archive based on Randomised Binary Tree (Treap)

 ---------------------------------------------------------------------
                       Copyright (C) 2026
          Jonathan Fieldsend <J.E.Fieldsend@exeter.ac.uk>
          Manuel Lopez-Ibanez  <manuel.lopez-ibanez@manchester.ac.uk>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ---------------------------------------------------------------------

*************************************************************************/
#ifndef TREAP_ARCHIVE_H
#define TREAP_ARCHIVE_H

#include "archiving.h"

typedef struct TreapArchive TreapArchive;
typedef struct TreapIterator TreapIterator;

// FIXME: We have to include this here because archiver.h needs it.
#include "solutions_list.h"

/**
   The treap stores the 2D vectors. Optional x-values are stored in this VoidList.
*/
typedef VoidList TreapItem;

static inline void
treap_item_free(TreapItem item)
{
    void_list_free(&item);
}

#include "treap2d.h"

typedef struct TreapArchive {
    Treap tree;
    size_t unique_size;  // Number of nodes in the tree, i.e., unique z vectors.
    size_t total_size;   // Number of x values (including duplicates of z vectors).
    signed char x_is_null;
    uint64_t revision; // To detect modifications of the tree while iterating.
} TreapArchive;


TreapArchive * treap_archive_new(void);
void treap_archive_init(TreapArchive *arch);
int treap_archive_add(TreapArchive * restrict self, const double * restrict z,
                      const void * restrict x, bool check, TreapArchive ** restrict displaced);
int treap_archive_add_discard(TreapArchive * restrict self, const double * restrict z, const void * restrict x);
int treap_archive_insert_displaced(TreapArchive *arch, TreapArchive *incoming, bool *all_displaced);
size_t treap_archive_total_size(const TreapArchive *arch);
size_t treap_archive_unique_size(const TreapArchive *arch);
void treap_archive_get_contents(const TreapArchive *arch, double * z_list, const void ** x_list);
void treap_archive_get_unique_vectors(const TreapArchive *arch, const double ** z_list);
void treap_archive_get_x_values(const TreapArchive *arch, const void ** x_list);
bool treap_archive_has_x_values(const TreapArchive *arch);
void treap_archive_free_nodes(TreapArchive * arch);
void treap_archive_free(TreapArchive * arch);
bool treap_archive_dominated_by(const TreapArchive * arch, const double * z);
bool treap_archive_dominates(const TreapArchive * arch, const double * z);
TreapNode * treap_archive_find_exact_vector(const TreapArchive * arch, const double * z);

typedef struct TreapIterator TreapIterator;
void treap_archive_iter_free(TreapIterator *it);
TreapIterator * treap_archive_iter_new(const TreapArchive *arch);
int treap_archive_iter_next(TreapIterator * restrict it, const double ** restrict z, const void ** restrict x);

#endif // TREAP_ARCHIVE_H
