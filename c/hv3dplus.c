/*************************************************************************

 hv-plus.c

 ---------------------------------------------------------------------

                        Copyright (c) 2013, 2016, 2017
                Andreia P. Guerreiro <apg@dei.uc.pt>


 This program is free software (software libre); you can redistribute
 it and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of the
 License.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, you can obtain a copy of the GNU
 General Public License at:
                 http://www.gnu.org/copyleft/gpl.html
 or by writing to:
           Free Software Foundation, Inc., 59 Temple Place,
                 Suite 330, Boston, MA 02111-1307 USA

 ----------------------------------------------------------------------

 Reference:

 [1] A. P. Guerreiro, C. M. Fonseca, “Computing and Updating Hypervolume Contributions in Up to Four Dimensions”, CISUC Technical Report TR-2017-001, University of Coimbra, 2017

*************************************************************************/


#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include "common.h"

typedef struct dlnode {
  double z[3];                    /* The data vector              */
  const double *x;                    /* The data vector              */
  struct dlnode * closest[2]; // closest[0] == cx, closest[1] == cy
  struct dlnode * cnext[2]; //current next

  struct dlnode * next[3]; //keeps the points sorted according to coordinates 2,3 and 4
                           // (in the case of 2 and 3, only the points swept by 4 are kept)
  struct dlnode *prev[3]; //keeps the points sorted according to coordinates 2 and 3 (except the sentinel 3)

  int ndomr;    //number of dominators
} dlnode_t;

/*-----------------------------------------------------------------------------

  The following is a reduced version of the AVL-tree library used here
  according to the terms of the GPL. See the copyright notice below.

*/
#define AVL_DEPTH

/*****************************************************************************

    avl.c - Source code for the AVL-tree library.

    Copyright (C) 1998  Michael H. Buselli <cosine@cosine.org>
    Copyright (C) 2000-2002  Wessel Dankers <wsl@nl.linux.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

    Augmented AVL-tree. Original by Michael H. Buselli <cosine@cosine.org>.

    Modified by Wessel Dankers <wsl@nl.linux.org> to add a bunch of bloat to
    the sourcecode, change the interface and squash a few bugs.
    Mail him if you find new bugs.

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


/* User supplied function to compare two items like strcmp() does.
 * For example: cmp(a,b) will return:
 *   -1  if a < b
 *    0  if a = b
 *    1  if a > b
 */
typedef int (*avl_compare_t)(const void *, const void *);

/* User supplied function to delete an item when a node is free()d.
 * If NULL, the item is not free()d.
 */
typedef void (*avl_freeitem_t)(void *);

typedef struct avl_node_t {
	struct avl_node_t *next;
	struct avl_node_t *prev;
	struct avl_node_t *parent;
	struct avl_node_t *left;
	struct avl_node_t *right;
    dlnode_t * dlnode;
    const double * item;
#ifdef AVL_DEPTH
	unsigned char depth;
#endif
} avl_node_t;

typedef struct avl_tree_t {
	avl_node_t *head;
	avl_node_t *tail;
	avl_node_t *top;
	avl_compare_t cmp;
	avl_freeitem_t freeitem;
} avl_tree_t;


static void avl_rebalance(avl_tree_t *, avl_node_t *);

#ifdef AVL_DEPTH
#define NODE_DEPTH(n)  ((n) ? (n)->depth : 0)
#define L_DEPTH(n)     (NODE_DEPTH((n)->left))
#define R_DEPTH(n)     (NODE_DEPTH((n)->right))
#define CALC_DEPTH(n)  (unsigned char)((L_DEPTH(n)>R_DEPTH(n)?L_DEPTH(n):R_DEPTH(n)) + 1)
#endif

static int avl_check_balance(avl_node_t *avlnode) {
#ifdef AVL_DEPTH
	int d = R_DEPTH(avlnode) - L_DEPTH(avlnode);
	return d < -1 ? -1 : d > 1 ? 1 : 0;
#else
#error No balancing possible.
#endif
}

static int
avl_search_closest(const avl_tree_t *avltree, const void *item, avl_node_t **avlnode) {
	avl_node_t *node;
	avl_compare_t cmp;
	int c;

	if(!avlnode)
		avlnode = &node;

	node = avltree->top;

	if(!node)
		return *avlnode = NULL, 0;

	cmp = avltree->cmp;

	for(;;) {
		c = cmp(item, node->item);

		if(c < 0) {
			if(node->left)
				node = node->left;
			else
				return *avlnode = node, -1;
		} else if(c > 0) {
			if(node->right)
				node = node->right;
			else
				return *avlnode = node, 1;
		} else {
			return *avlnode = node, 0;
		}
	}
}

static avl_tree_t *avl_init_tree(avl_tree_t *rc, avl_compare_t cmp, avl_freeitem_t freeitem) {
	if(rc) {
		rc->head = NULL;
		rc->tail = NULL;
		rc->top = NULL;
		rc->cmp = cmp;
		rc->freeitem = freeitem;
	}
	return rc;
}

static avl_tree_t *avl_alloc_tree(avl_compare_t cmp, avl_freeitem_t freeitem) {
	return avl_init_tree(malloc(sizeof(avl_tree_t)), cmp, freeitem);
}

static void avl_clear_tree(avl_tree_t *avltree) {
	avltree->top = avltree->head = avltree->tail = NULL;
}

static void avl_free_nodes(avl_tree_t *avltree) {
	avl_node_t *node, *next;
	avl_freeitem_t freeitem;

	freeitem = avltree->freeitem;

	for(node = avltree->head; node; node = next) {
		next = node->next;
		if(freeitem)
			freeitem(node->item);
		free(node);
	}

	avl_clear_tree(avltree);
}

/*
 * avl_free_tree:
 * Free all memory used by this tree.  If freeitem is not NULL, then
 * it is assumed to be a destructor for the items referenced in the avl_
 * tree, and they are deleted as well.
 */
static void avl_free_tree(avl_tree_t *avltree) {
	avl_free_nodes(avltree);
	free(avltree);
}

static void avl_clear_node(avl_node_t *newnode) {
	newnode->left = newnode->right = NULL;
	#ifdef AVL_COUNT
	newnode->count = 1;
	#endif
	#ifdef AVL_DEPTH
	newnode->depth = 1;
	#endif
}

static avl_node_t *avl_init_node(avl_node_t *newnode, void *item) {
	if(newnode) {
	  avl_clear_node(newnode);
	  newnode->item = item;
	}
	return newnode;
}

static avl_node_t *avl_insert_top(avl_tree_t *avltree, avl_node_t *newnode) {
	avl_clear_node(newnode);
	newnode->prev = newnode->next = newnode->parent = NULL;
	avltree->head = avltree->tail = avltree->top = newnode;
	return newnode;
}

static avl_node_t *avl_insert_after(avl_tree_t *avltree, avl_node_t *node, avl_node_t *newnode);

static avl_node_t *avl_insert_before(avl_tree_t *avltree, avl_node_t *node, avl_node_t *newnode) {
	if(!node)
		return avltree->tail
			? avl_insert_after(avltree, avltree->tail, newnode)
			: avl_insert_top(avltree, newnode);

	if(node->left)
		return avl_insert_after(avltree, node->prev, newnode);

	avl_clear_node(newnode);

	newnode->next = node;
	newnode->parent = node;

	newnode->prev = node->prev;
	if(node->prev)
		node->prev->next = newnode;
	else
		avltree->head = newnode;
	node->prev = newnode;

	node->left = newnode;
	avl_rebalance(avltree, node);
	return newnode;
}

static avl_node_t *avl_insert_after(avl_tree_t *avltree, avl_node_t *node, avl_node_t *newnode) {
	if(!node)
		return avltree->head
			? avl_insert_before(avltree, avltree->head, newnode)
			: avl_insert_top(avltree, newnode);

	if(node->right)
		return avl_insert_before(avltree, node->next, newnode);

	avl_clear_node(newnode);

	newnode->prev = node;
	newnode->parent = node;

	newnode->next = node->next;
	if(node->next)
		node->next->prev = newnode;
	else
		avltree->tail = newnode;
	node->next = newnode;

	node->right = newnode;
	avl_rebalance(avltree, node);
	return newnode;
}

/*
 * avl_unlink_node:
 * Removes the given node.  Does not delete the item at that node.
 * The item of the node may be freed before calling avl_unlink_node.
 * (In other words, it is not referenced by this function.)
 */
static void avl_unlink_node(avl_tree_t *avltree, avl_node_t *avlnode) {
	avl_node_t *parent;
	avl_node_t **superparent;
	avl_node_t *subst, *left, *right;
	avl_node_t *balnode;

	if(avlnode->prev)
		avlnode->prev->next = avlnode->next;
	else
		avltree->head = avlnode->next;

	if(avlnode->next)
		avlnode->next->prev = avlnode->prev;
	else
		avltree->tail = avlnode->prev;

	parent = avlnode->parent;

	superparent = parent
		? avlnode == parent->left ? &parent->left : &parent->right
		: &avltree->top;

	left = avlnode->left;
	right = avlnode->right;
	if(!left) {
		*superparent = right;
		if(right)
			right->parent = parent;
		balnode = parent;
	} else if(!right) {
		*superparent = left;
		left->parent = parent;
		balnode = parent;
	} else {
		subst = avlnode->prev;
		if(subst == left) {
			balnode = subst;
		} else {
			balnode = subst->parent;
			balnode->right = subst->left;
			if(balnode->right)
				balnode->right->parent = balnode;
			subst->left = left;
			left->parent = subst;
		}
		subst->right = right;
		subst->parent = parent;
		right->parent = subst;
		*superparent = subst;
	}

	avl_rebalance(avltree, balnode);
}

static void *avl_delete_node(avl_tree_t *avltree, avl_node_t *avlnode) {
	void *item = NULL;
	if(avlnode) {
		item = avlnode->item;
		avl_unlink_node(avltree, avlnode);
		if(avltree->freeitem)
			avltree->freeitem(item);
		free(avlnode);
	}
	return item;
}

/*
 * avl_rebalance:
 * Rebalances the tree if one side becomes too heavy.  This function
 * assumes that both subtrees are AVL-trees with consistant data.  The
 * function has the additional side effect of recalculating the count of
 * the tree at this node.  It should be noted that at the return of this
 * function, if a rebalance takes place, the top of this subtree is no
 * longer going to be the same node.
 */
static void avl_rebalance(avl_tree_t *avltree, avl_node_t *avlnode) {
	avl_node_t *child;
	avl_node_t *gchild;
	avl_node_t *parent;
	avl_node_t **superparent;

	parent = avlnode;

	while(avlnode) {
		parent = avlnode->parent;

		superparent = parent
			? avlnode == parent->left ? &parent->left : &parent->right
			: &avltree->top;

		switch(avl_check_balance(avlnode)) {
		case -1:
			child = avlnode->left;
			#ifdef AVL_DEPTH
			if(L_DEPTH(child) >= R_DEPTH(child)) {
			#else
			#ifdef AVL_COUNT
			if(L_COUNT(child) >= R_COUNT(child)) {
			#else
			#error No balancing possible.
			#endif
			#endif
				avlnode->left = child->right;
				if(avlnode->left)
					avlnode->left->parent = avlnode;
				child->right = avlnode;
				avlnode->parent = child;
				*superparent = child;
				child->parent = parent;
				#ifdef AVL_COUNT
				avlnode->count = CALC_COUNT(avlnode);
				child->count = CALC_COUNT(child);
				#endif
				#ifdef AVL_DEPTH
				avlnode->depth = CALC_DEPTH(avlnode);
				child->depth = CALC_DEPTH(child);
				#endif
			} else {
				gchild = child->right;
				avlnode->left = gchild->right;
				if(avlnode->left)
					avlnode->left->parent = avlnode;
				child->right = gchild->left;
				if(child->right)
					child->right->parent = child;
				gchild->right = avlnode;
				if(gchild->right)
					gchild->right->parent = gchild;
				gchild->left = child;
				if(gchild->left)
					gchild->left->parent = gchild;
				*superparent = gchild;
				gchild->parent = parent;
				#ifdef AVL_COUNT
				avlnode->count = CALC_COUNT(avlnode);
				child->count = CALC_COUNT(child);
				gchild->count = CALC_COUNT(gchild);
				#endif
				#ifdef AVL_DEPTH
				avlnode->depth = CALC_DEPTH(avlnode);
				child->depth = CALC_DEPTH(child);
				gchild->depth = CALC_DEPTH(gchild);
				#endif
			}
		break;
		case 1:
			child = avlnode->right;
			#ifdef AVL_DEPTH
			if(R_DEPTH(child) >= L_DEPTH(child)) {
			#else
			#ifdef AVL_COUNT
			if(R_COUNT(child) >= L_COUNT(child)) {
			#else
			#error No balancing possible.
			#endif
			#endif
				avlnode->right = child->left;
				if(avlnode->right)
					avlnode->right->parent = avlnode;
				child->left = avlnode;
				avlnode->parent = child;
				*superparent = child;
				child->parent = parent;
				#ifdef AVL_COUNT
				avlnode->count = CALC_COUNT(avlnode);
				child->count = CALC_COUNT(child);
				#endif
				#ifdef AVL_DEPTH
				avlnode->depth = CALC_DEPTH(avlnode);
				child->depth = CALC_DEPTH(child);
				#endif
			} else {
				gchild = child->left;
				avlnode->right = gchild->left;
				if(avlnode->right)
					avlnode->right->parent = avlnode;
				child->left = gchild->right;
				if(child->left)
					child->left->parent = child;
				gchild->left = avlnode;
				if(gchild->left)
					gchild->left->parent = gchild;
				gchild->right = child;
				if(gchild->right)
					gchild->right->parent = gchild;
				*superparent = gchild;
				gchild->parent = parent;
				#ifdef AVL_COUNT
				avlnode->count = CALC_COUNT(avlnode);
				child->count = CALC_COUNT(child);
				gchild->count = CALC_COUNT(gchild);
				#endif
				#ifdef AVL_DEPTH
				avlnode->depth = CALC_DEPTH(avlnode);
				child->depth = CALC_DEPTH(child);
				gchild->depth = CALC_DEPTH(gchild);
				#endif
			}
		break;
		default:
			#ifdef AVL_COUNT
			avlnode->count = CALC_COUNT(avlnode);
			#endif
			#ifdef AVL_DEPTH
			avlnode->depth = CALC_DEPTH(avlnode);
			#endif
		}
		avlnode = parent;
	}
}

/*------------------------------------------------------------------------------
 end of functions from AVL-tree library.
*******************************************************************************/




/* ---------------------------------- Data Structures Functions ---------------------------------------*/

static void initSentinels(dlnode_t * list, const double * ref)
{
    dlnode_t * s1 = list;
    dlnode_t * s2 = list + 1;
    dlnode_t * s3 = list + 2;

    s1->z[0] = -DBL_MAX;
    s1->z[1] = ref[1];
    s1->z[2] = -DBL_MAX;
    s1->x = s1->z;
    s1->closest[0] = s2;
    s1->closest[1] = s1;

    s1->next[2] = s2;
    s1->cnext[1] = NULL;
    s1->cnext[0] = NULL;

    s1->prev[2] = s3;
    s1->ndomr = 0;


    s2->z[0] = ref[0];
    s2->z[1] = -DBL_MAX;
    s2->z[2] = -DBL_MAX;
    s2->x = s2->z;
    s2->closest[0] = s2;
    s2->closest[1] = s1;

    s2->next[2] = s3;
    s2->cnext[1] = NULL;
    s2->cnext[0] = NULL;

    s2->prev[2] = s1;
    s2->ndomr = 0;


    // ???? It was INT_MAX
    s3->z[0] = -DBL_MAX;
    s3->z[1] = -DBL_MAX;
    s3->z[2] = ref[2];
    s3->x = s3->z;
    s3->closest[0] = s2;
    s3->closest[1] = s1;

    s3->next[2] = s1;
    s3->cnext[1] = NULL;
    s3->cnext[0] = NULL;

    s3->prev[2] = s2;
    s3->ndomr = 0;
}



/* ---------------------------------- Update data structure ---------------------------------------*/




static void removeFromz(dlnode_t * old)
{
    old->prev[2]->next[2] = old->next[2];
    old->next[2]->prev[2] = old->prev[2];
}




/* ---------------------------------- Sort ---------------------------------------*/

static int
compare_point3d(const void * p1, const void * p2)
{
    for (int i = 2; i >= 0; i--) {
        double x1 = (*((const double **)p1))[i];
        double x2 = (*((const double **)p2))[i];

        if (x1 < x2)
            return -1;
        if (x1 > x2)
            return 1;
    }
    return 0;
}

typedef unsigned int dimension_t;

static bool
strongly_dominates(const double * x, const double * ref, dimension_t dim)
{
    ASSUME(dim >= 2);
    for (dimension_t i = 0; i < dim; i++)
        if (x[i] >= ref[i])
            return false;
    return true;
}


static void check_point(dlnode_t *p) {
    assert(p->x[0] == p->z[0]);
    assert(p->x[1] == p->z[1]);
    assert(p->x[2] == p->z[2]);
}
static void print_x(dlnode_t * p)
{
    assert(p != NULL);
    const double * z = p->z;
    fprintf(stderr, "z: %g %g %g\n", z[0], z[1], z[2]);
    const double * x = p->x;
    fprintf(stderr, "x: %g %g %g\n", x[0], x[1], x[2]);
    check_point(p);
}


/*
 * Setup circular double-linked list in each dimension
 */
static dlnode_t *
setup_cdllist(const double * data, int n, const double *ref)
{
    ASSUME(n > 1);
    dimension_t d = 3;

    dlnode_t * list = (dlnode_t *) malloc((n + 3) * sizeof(dlnode_t));
    dlnode_t * list3 = list+3;
    initSentinels(list, ref);

    const double **scratchd = malloc(n * sizeof(const double*));
    int i, j;
    for (i = 0, j = 0; j < n; j++) {
        /* Filters those points that do not strictly dominate the reference
           point.  This is needed to assure that the points left are only those
           that are needed to calculate the hypervolume. */
        if (strongly_dominates(data + j * d, ref, d)) {
            scratchd[i] = data + j * d;
            i++;
        }
    }
    n = i;
    if (n == 0) {
        free(scratchd);
        return list;
    }

    qsort(scratchd, n, sizeof(const double*), compare_point3d);

    for (int i = 0; i < n; i++) {
        /* FIXME: This creates yet another copy of the data. */
        dlnode_t * p = list3 + i;
        for (dimension_t j = 0; j < d; j++)
            p->z[j] = scratchd[i][j];
        p->x = scratchd[i];
//        p->x = p->z;
        // clearPoint:
        p->closest[1] = list;
        assert(list->next[2] == list + 1);
        p->closest[0] = list->next[2];
        /* because of printfs */ /* FIXME what does the comment mean????? */
        p->cnext[1] = list;
        p->cnext[0] = list->next[2];
        assert(list->next[2] == list + 1);
        p->ndomr = 0;
    }

    free(scratchd);

    dimension_t d_1 = d - 1;
    dlnode_t * s = list->next[d_1];
    assert(s == list + 1);
    assert(s->next[d_1] == list + 2);
    s->next[d_1] = list3;
    list3->prev[d_1] = s;
    for (i = 0; i < n - 1; i++) {
        (list3+i)->next[d_1] = list3+i+1;
        (list3+i+1)->prev[d_1] = list3+i;
    }
    s = list->prev[d_1];
    s->prev[d_1] = list3 + n - 1;
    (list3 + n - 1)->next[d_1] = s;

    for (int i=0; i < n; i++) {
        dlnode_t * p = list3 + i;
        check_point(p);
    }
    return list;
}



static void free_cdllist(dlnode_t * list)
{
    free(list);
}




/* ---------------------------------- Preprocessing ---------------------------------------*/


static int compare_tree_asc_y( const void *p1, const void *p2)
{
    const double x1= *((const double *)p1+1);
    const double x2= *((const double *)p2+1);

    if (x1 < x2)
        return -1;
    else if (x1 > x2)
        return 1;
    else return 0;
}





static inline const double *node_point(const avl_node_t *node)
{
    return (const double*) node->item;
}


static avl_node_t * new_avl_node (dlnode_t * p)
{
    avl_node_t * node = malloc(sizeof(avl_node_t));
    node->dlnode = p;
    node->item = p->x;
    return node;
}
static void preprocessing(dlnode_t * list)
{
    avl_tree_t * tree = avl_alloc_tree ((avl_compare_t) compare_tree_asc_y, NULL);
    dlnode_t * p = list;
    avl_node_t * node = new_avl_node(p);
    avl_insert_top(tree, node);
    p = p->next[2];

    avl_node_t * nodeaux = new_avl_node(p);
    avl_insert_before(tree, node, nodeaux);
    p = p->next[2];

    dlnode_t * stop = list->prev[2];
    assert(stop == list+2);
    while (p != stop) {
        node = new_avl_node(p);

        if (avl_search_closest(tree, p->x, &nodeaux) == 1)
            nodeaux = nodeaux->next;

        const double * point = node_point(nodeaux);
        if (point[1] == p->x[1] && point[0] <= p->x[0]) {
            nodeaux = nodeaux->next;
        }

        const double * prev_x = node_point(nodeaux->prev);

        if (prev_x[0] <= p->x[0] && prev_x[1] <= p->x[1] && prev_x[2] <= p->x[2]) {
            p->ndomr = 1;
            free(node);
        } else {
            while (node_point(nodeaux)[0] >= p->x[0]) {

                nodeaux = nodeaux->next;
                avl_delete_node(tree, nodeaux->prev);
            }

            avl_insert_before(tree, nodeaux, node);
            p->closest[0] = node->prev->dlnode;
            p->closest[1] = node->next->dlnode;
        }
        p = p->next[2];
    }
    avl_free_tree(tree);
}




/* ----------------------Hypervolume Indicator Algorithms ---------------------------------------*/



static void restartListy(dlnode_t * list)
{
    check_point(list);
    list->next[2]->cnext[1] = list; //link sentinels sentinels ((-inf ref[1] -inf) and (ref[0] -inf -inf))
    list->cnext[0] = list->next[2];
}

static double compute_area3d_simple(const double * p, dlnode_t * q)
{
    const unsigned int di = 1, dj = 0;
    dlnode_t * u = q->cnext[1];
    double area = (q->x[dj] - p[dj]) * (u->x[di] - p[di]);
    while (p[dj] < u->x[dj]) {
        q = u;
        u = u->cnext[di];
        area += (q->x[dj] - p[dj]) * (u->x[di] - q->x[di]);
    }
    return area;
}

static double hv3dplus(dlnode_t * list)
{
    restartListy(list);
    double area = 0;
    double volume = 0;
    dlnode_t * p = list->next[2]->next[2];
    dlnode_t * stop = list->prev[2];
    assert(stop == list+2);
    while (p != stop) {
        if (p->ndomr < 1) {
            p->cnext[0] = p->closest[0];
            p->cnext[1] = p->closest[1];
//            fprintf(stderr, "area = %g\n", area);
            /* print_x(p); */
            /* print_x(p->cnext[0]); */
            /* print_x(p->cnext[0]->cnext[1]); */
            area += compute_area3d_simple(p->x, p->cnext[0]);

            p->cnext[0]->cnext[1] = p;
            p->cnext[1]->cnext[0] = p;
        } else {
            removeFromz(p);
        }
        volume += area * (p->next[2]->x[2] - p->x[2]);
        p = p->next[2];
    }
    return volume;

}

double hv3d_plus(const double * restrict data, int n, const double * restrict ref)
{
    dlnode_t * list = setup_cdllist(data, n, ref);
    preprocessing(list);
    double hv = hv3dplus(list);
    free_cdllist(list);
    return hv;
}
