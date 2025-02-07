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
    struct dlnode * cnext[2]; // current next
    struct dlnode * next; //keeps the points sorted according to coordinates 2,3 and 4
    // (in the case of 2 and 3, only the points swept by 4 are kept)
    struct dlnode *prev; //keeps the points sorted according to coordinates 2 and 3 (except the sentinel 3)
    bool dom;    // is this point dominated?
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

static void avl_clear_node(avl_node_t *newnode) {
	newnode->left = newnode->right = NULL;
	#ifdef AVL_COUNT
	newnode->count = 1;
	#endif
	#ifdef AVL_DEPTH
	newnode->depth = 1;
	#endif
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

    double * z = malloc(3 * 3 * sizeof(double));
    z[0] = -DBL_MAX;
    z[1] = ref[1];
    z[2] = -DBL_MAX;
    s1->z[0] = z[0];
    s1->z[1] = z[1];
    s1->z[2] = z[2];
    s1->x = z;
    s1->closest[0] = s2;
    s1->closest[1] = s1;

    s1->next = s2;
    s1->cnext[1] = NULL;
    s1->cnext[0] = NULL;

    s1->prev = s3;
    s1->dom = false;

    z += 3;
    z[0] = ref[0];
    z[1] = -DBL_MAX;
    z[2] = -DBL_MAX;
    s2->z[0] = z[0];
    s2->z[1] = z[1];
    s2->z[2] = z[2];
    s2->x = z;
    s2->closest[0] = s2;
    s2->closest[1] = s1;

    s2->next = s3;
    s2->cnext[1] = NULL;
    s2->cnext[0] = NULL;

    s2->prev = s1;
    s2->dom = false;


    // ???? It was INT_MAX
    z += 3;
    z[0] = -DBL_MAX;
    z[1] = -DBL_MAX;
    z[2] = ref[2];
    s3->z[0] = z[0];
    s3->z[1] = z[1];
    s3->z[2] = z[2];
    s3->x = z;
    s3->closest[0] = s2;
    s3->closest[1] = s1;

    s3->next = s1;
    s3->cnext[1] = NULL;
    s3->cnext[0] = NULL;

    s3->prev = s2;
    s3->dom = false;
}



/* ---------------------------------- Update data structure ---------------------------------------*/




static void removeFromz(dlnode_t * old)
{
    old->prev->next = old->next;
    old->next->prev = old->prev;
}




/* ---------------------------------- Sort ---------------------------------------*/

static int
compare_point3d(const void * p1, const void * p2)
{
    const double *x1 = *((const double **)p1);
    const double *x2 = *((const double **)p2);
    for (int i = 2; i >= 0; i--) {
        if (x1[i] < x2[i])
            return -1;
        if (x1[i] > x2[i])
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

static void preprocessing(dlnode_t * list, int n);

/*
 * Setup circular double-linked list in each dimension
 */
static dlnode_t *
setup_cdllist(const double * data, int n, const double *ref)
{
    ASSUME(n > 1);
    const dimension_t d = 3;

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
    n = i; // Update number of points.

    dlnode_t * list = (dlnode_t *) malloc((n + 3) * sizeof(dlnode_t));
    dlnode_t * list3 = list+3;
    initSentinels(list, ref);
    if (n == 0) {
        free(scratchd);
        return list;
    }

    qsort(scratchd, n, sizeof(const double*), compare_point3d);

    dlnode_t * q = list+1;
    assert(list->next == list + 1);
    assert(q->next == list + 2);
    for (int i = 0; i < n; i++) {
        /* FIXME: This creates yet another copy of the data. */
        dlnode_t * p = list3 + i;
        for (dimension_t j = 0; j < d; j++)
            p->z[j] = scratchd[i][j];
        p->x = scratchd[i];
//        p->x = p->z;
        // clearPoint:
        assert(list->next == list + 1);
        p->closest[0] = list + 1;
        p->closest[1] = list;
        /* because of printfs */ /* FIXME what does the comment mean????? */
        p->cnext[0] = list + 1;
        p->cnext[1] = list;
        p->dom = false;
        // Link the list in order.
        q->next = p;
        p->prev = q;
        q = p;
    }
    free(scratchd);
    q = list->prev;
    (list3 + n - 1)->next = q;
    q->prev = list3 + n - 1;

    for (int i=0; i < n; i++) {
        dlnode_t * p = list3 + i;
        check_point(p);
    }
    preprocessing(list, n);
    return list;
}



static void free_cdllist(dlnode_t * list)
{
    free((void*) list->x); // Free sentinels.
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


static avl_node_t * new_avl_node (dlnode_t * p, avl_node_t * node)
{
    node->dlnode = p;
    node->item = p->x;
    return node;
}

static void preprocessing(dlnode_t * list, int n)
{
    avl_tree_t * tree = avl_alloc_tree ((avl_compare_t) compare_tree_asc_y, NULL);
    avl_node_t * tnodes = malloc((n+3)* sizeof(avl_node_t));
    dlnode_t * p = list;
    avl_node_t * nodeaux = new_avl_node(p, tnodes);
    avl_insert_top(tree, nodeaux);
    p = p->next;

    avl_node_t * node = new_avl_node(p, tnodes + 1);
    avl_insert_before(tree, nodeaux, node);
    p = p->next;

    dlnode_t * stop = list->prev;
    assert(stop == list+2);
    while (p != stop) {
        if (avl_search_closest(tree, p->x, &nodeaux) == 1)
            nodeaux = nodeaux->next;

        const double * point = node_point(nodeaux);
        if (point[1] == p->x[1] && point[0] <= p->x[0]) {
            nodeaux = nodeaux->next;
        }

        const double * prev_x = node_point(nodeaux->prev);
        // FIXME: Do we need to check all coordinates? The points are already sorted!
        assert(prev_x[1] <= p->x[1]);
        assert(prev_x[2] <= p->x[2]);
        if (prev_x[0] <= p->x[0] && prev_x[1] <= p->x[1] && prev_x[2] <= p->x[2]) {
            p->dom = true;
        } else {
            // ???? What is this loop doing? Should this be > ?
            while (node_point(nodeaux)[0] >= p->x[0]) {
                nodeaux = nodeaux->next;
                avl_unlink_node(tree, nodeaux->prev);
            }
            node = new_avl_node(p, node + 1);
            avl_insert_before(tree, nodeaux, node);
            p->closest[0] = node->prev->dlnode;
            p->closest[1] = node->next->dlnode;
        }
        p = p->next;
    }
    free(tnodes);
    free(tree);
}




/* ----------------------Hypervolume Indicator Algorithms ---------------------------------------*/



static double compute_area3d_simple(const double * px, dlnode_t * q)
{
    const unsigned int di = 1, dj = 0;
    dlnode_t * u = q->cnext[1];
    double area = (q->x[dj] - px[dj]) * (u->x[di] - px[di]);
    assert(area > 0);
    while (px[dj] < u->x[dj]) {
        q = u;
        u = u->cnext[di];
        assert((q->x[dj] - px[dj]) * (u->x[di] - q->x[di]) > 0);
        area += (q->x[dj] - px[dj]) * (u->x[di] - q->x[di]);
    }
    return area;
}

static double hv3dplus(dlnode_t * list)
{
    // restartList:
    check_point(list);
    list->next->cnext[1] = list; //link sentinels sentinels ((-inf ref[1] -inf) and (ref[0] -inf -inf))
    list->cnext[0] = list->next;
    double area = 0;
    double volume = 0;
    dlnode_t * p = list->next->next;
    dlnode_t * stop = list->prev;
    assert(stop == list+2);
    while (p != stop) {
        if (p->dom) {
            removeFromz(p);
        } else {
            p->cnext[0] = p->closest[0];
            p->cnext[1] = p->closest[1];
//            fprintf(stderr, "area = %g\n", area);
            /* print_x(p); */
            /* print_x(p->cnext[0]); */
            /* print_x(p->cnext[0]->cnext[1]); */
            area += compute_area3d_simple(p->x, p->cnext[0]);

            p->cnext[0]->cnext[1] = p;
            p->cnext[1]->cnext[0] = p;
        }
        // FIXME: This assert should work but it fails because p and p->next are duplicated points:
        // assert((p->next->x[2] - p->x[2]) != 0);
        /*if (p->next->x[2] - p->x[2] == 0) {
            assert(p->next != p);
            print_x(p);
            print_x(p->next);
            assert((p->next->x[2] - p->x[2]) != 0);
            }*/
        assert(area > 0);
        volume += area * (p->next->x[2] - p->x[2]);
        p = p->next;
    }
    return volume;

}

double hv3d_plus(const double *data, int n, const double *ref)
{
    dlnode_t * list = setup_cdllist(data, n, ref);
    double hv = hv3dplus(list);
    free_cdllist(list);
    return hv;
}
