/*************************************************************************

 hypervolume computation

 ---------------------------------------------------------------------

                       Copyright (c) 2010
                  Carlos M. Fonseca <cmfonsec@dei.uc.pt>
             Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>
                    Luis Paquete <paquete@dei.uc.pt>

 This program is free software (software libre); you can redistribute
 it and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2 of the
 License, or (at your option) any later version.

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

 Relevant literature:

 [1]  C. M. Fonseca, L. Paquete, and M. Lopez-Ibanez. An
      improved dimension-sweep algorithm for the hypervolume
      indicator. In IEEE Congress on Evolutionary Computation,
      pages 1157-1163, Vancouver, Canada, July 2006.

 [2]  Nicola Beume, Carlos M. Fonseca, Manuel López-Ibáñez, Luís
      Paquete, and J. Vahrenhold. On the complexity of computing the
      hypervolume indicator. IEEE Transactions on Evolutionary
      Computation, 13(5):1075-1082, 2009.

*************************************************************************/

#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <stdint.h>
#include "common.h"
#include "hv.h"

typedef uint_fast8_t dimension_t;

static int compare_tree_asc(const void *p1, const void *p2);

/*-----------------------------------------------------------------------------

  The following is a reduced version of the AVL-tree library used here
  according to the terms of the GPL. See the copyright notice below.

*/
#define AVL_DEPTH

/*****************************************************************************

    avl.h - Source code for the AVL-tree library.

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
typedef const double avl_item_t;
typedef struct avl_node_t {
	struct avl_node_t *next;
	struct avl_node_t *prev;
	struct avl_node_t *parent;
	struct avl_node_t *left;
	struct avl_node_t *right;
        avl_item_t *item;
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


/*****************************************************************************

    avl.c - Source code for the AVL-tree library.

*****************************************************************************/

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
avl_search_closest(const avl_tree_t *avltree, const avl_item_t *item, avl_node_t **avlnode) {
	avl_node_t *node;
	int c;

	if(!avlnode)
		avlnode = &node;

	node = avltree->top;

	if(!node)
		return *avlnode = NULL, 0;


	for(;;) {
		c = compare_tree_asc(item, node->item);

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

static avl_tree_t *
avl_init_tree(avl_tree_t *rc, avl_compare_t cmp, avl_freeitem_t freeitem) {
	if(rc) {
		rc->head = NULL;
		rc->tail = NULL;
		rc->top = NULL;
		rc->cmp = cmp;
		rc->freeitem = freeitem;
	}
	return rc;
}

static avl_tree_t *
avl_alloc_tree(avl_compare_t cmp, avl_freeitem_t freeitem) {
	return avl_init_tree(malloc(sizeof(avl_tree_t)), cmp, freeitem);
}

static void
avl_clear_tree(avl_tree_t *avltree) {
	avltree->top = avltree->head = avltree->tail = NULL;
}

static void
avl_clear_node(avl_node_t *newnode) {
	newnode->left = newnode->right = NULL;
	#ifdef AVL_COUNT
	newnode->count = 1;
	#endif
	#ifdef AVL_DEPTH
	newnode->depth = 1;
	#endif
}

static avl_node_t *
avl_insert_top(avl_tree_t *avltree, avl_node_t *newnode) {
	avl_clear_node(newnode);
	newnode->prev = newnode->next = newnode->parent = NULL;
	avltree->head = avltree->tail = avltree->top = newnode;
	return newnode;
}

static avl_node_t *
avl_insert_after(avl_tree_t *avltree, avl_node_t *node, avl_node_t *newnode);

static avl_node_t *
avl_insert_before(avl_tree_t *avltree, avl_node_t *node, avl_node_t *newnode) {
	if(!node)
		return avltree->tail
			? avl_insert_after(avltree, avltree->tail, newnode)
			: avl_insert_top(avltree, newnode);

	if(node->left)
		return avl_insert_after(avltree, node->prev, newnode);

        assert (node);
        assert (!node->left);

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

static avl_node_t *
avl_insert_after(avl_tree_t *avltree, avl_node_t *node, avl_node_t *newnode) {
	if(!node)
		return avltree->head
			? avl_insert_before(avltree, avltree->head, newnode)
			: avl_insert_top(avltree, newnode);

	if(node->right)
		return avl_insert_before(avltree, node->next, newnode);

        assert (node);
        assert (!node->right);

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
static void
avl_unlink_node(avl_tree_t *avltree, avl_node_t *avlnode) {
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
static void
avl_rebalance(avl_tree_t *avltree, avl_node_t *avlnode) {
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
typedef struct dlnode {
    const double *x;              /* The data vector              */
    struct dlnode **next;         /* Next-node vector             */
    struct dlnode **prev;         /* Previous-node vector         */
    struct avl_node_t * tnode;
    dimension_t ignore;           /* Restricts dim to be 255.  */
    double *area;                 /* Area */
    double *vol;                  /* Volume */
} dlnode_t;

#define STOP_DIMENSION 2 /* default: stop on dimension 3 */

static int compare_node(const void *p1, const void* p2)
{
    const double x1 = *((*(const dlnode_t **)p1)->x);
    const double x2 = *((*(const dlnode_t **)p2)->x);

    return (x1 < x2) ? -1 : (x1 > x2) ? 1 : 0;
}

static int compare_tree_asc(const void *p1, const void *p2)
{
    const double *x1 = (const double *)p1;
    const double *x2 = (const double *)p2;

    return (x1[1] > x2[1]) ? -1 : (x1[1] < x2[1]) ? 1
        : (x1[0] >= x2[0]) ? -1 : 1;
}

/*
static int compare_tree_asc_y( const void *p1, const void *p2)
{
    const double x1 = *((const double *)p1+1);
    const double x2 = *((const double *)p2+1);
    return (x1 < x2) ? -1 : ((x1 > x2) ? 1 : 0;
}
*/

static bool
strongly_dominates(const double * restrict x,
                   const double * restrict ref, dimension_t dim)
{
    ASSUME(dim >= 2);
    for (dimension_t i = 0; i < dim; i++)
        if (x[i] >= ref[i])
            return false;
    return true;
}

/*
 * Setup circular double-linked list in each dimension
 */

static dlnode_t *
setup_cdllist(const double * restrict data, dimension_t d, int * restrict size,
              const double * restrict ref)
{
    ASSUME(d > STOP_DIMENSION);
    dimension_t d_stop = d - STOP_DIMENSION;
    int n = *size;
    dlnode_t *head  = malloc ((n+1) * sizeof(dlnode_t));
    head->x = NULL; /* head contains no data */
    head->ignore = 0;  /* should never get used */
    head->next = malloc(d_stop * (n+1) * sizeof(dlnode_t*));
    head->prev = malloc(d_stop * (n+1) * sizeof(dlnode_t*));
    head->tnode = malloc((n+1) * sizeof(avl_node_t));
    head->area = malloc(d_stop * (n+1) * sizeof(double));
    head->vol = malloc(d_stop * (n+1) * sizeof(double));

    int i, j;
    for (i = 1, j = 0; j < n; j++) {
        /* Filters those points that do not strictly dominate the reference
           point.  This is needed to assure that the points left are only those
           that are needed to calculate the hypervolume. */
        if (unlikely(strongly_dominates(data + j * d, ref, d))) {
            head[i].x = data + (j+1) * d; /* this will be fixed a few lines below... */
            head[i].ignore = 0;
            head[i].next = head->next + i * d_stop;
            head[i].prev = head->prev + i * d_stop;
            head[i].tnode = head->tnode + i;
            head[i].area = head->area + i * d_stop;
            head[i].vol = head->vol + i * d_stop;
            i++;
        }
    }
    n = i - 1;
    if (unlikely(n == 0))
        goto finish;

    dlnode_t **scratch = malloc(n * sizeof(dlnode_t*));
    for (i = 0; i < n; i++)
        scratch[i] = head + i + 1;

    for (int k = d-1; k >= 0; k--) {
        for (i = 0; i < n; i++)
            scratch[i]->x--;
        int j = k - STOP_DIMENSION;
        if (j < 0)
            continue;
        qsort(scratch, n, sizeof(dlnode_t*), compare_node);
        head->next[j] = scratch[0];
        scratch[0]->prev[j] = head;
        for (i = 1; i < n; i++) {
            scratch[i-1]->next[j] = scratch[i];
            scratch[i]->prev[j] = scratch[i-1];
        }
        scratch[n-1]->next[j] = head;
        head->prev[j] = scratch[n-1];
    }

    free(scratch);

    for (i = 1; i <= n; i++) {
        (head[i].tnode)->item = head[i].x;
    }

    for (i = 0; i < d_stop; i++)
        head->area[i] = 0;

finish:
    *size = n;
    return head;
}

static void free_cdllist(dlnode_t * head)
{
    free(head->tnode); /* Frees _all_ nodes. */
    free(head->next);
    free(head->prev);
    free(head->area);
    free(head->vol);
    free(head);
}

static void delete (dlnode_t * nodep, dimension_t dim, double * restrict bound)
{
    ASSUME(dim > STOP_DIMENSION);
    for (int i = STOP_DIMENSION; i < dim; i++) {
        int d = i - STOP_DIMENSION;
        nodep->prev[d]->next[d] = nodep->next[d];
        nodep->next[d]->prev[d] = nodep->prev[d];
        if (bound[i] > nodep->x[i])
            bound[i] = nodep->x[i];
  }
}

static void reinsert (dlnode_t *nodep, dimension_t dim, double * restrict bound)
{
    ASSUME(dim > STOP_DIMENSION);
    for (int i = STOP_DIMENSION; i < dim; i++) {
        int d = i - STOP_DIMENSION;
        nodep->prev[d]->next[d] = nodep;
        nodep->next[d]->prev[d] = nodep;
        if (bound[i] > nodep->x[i])
            bound[i] = nodep->x[i];
    }
}

static inline const double *node_point(const avl_node_t *node)
{
    return (const double *) node->item;
}

static double
fpli_hv3d(avl_tree_t *tree, dlnode_t *list, int c)
{
    dlnode_t *pp = list->next[0];
    double hypera = pp->x[0] * pp->x[1];
    double height = (c == 1)
        ? -pp->x[2]
        : pp->next[0]->x[2] - pp->x[2];

    double hyperv = hypera * height;

    if (pp->next[0]->x == NULL)
        return hyperv;

    avl_insert_top(tree, pp->tnode);

    pp = pp->next[0];
    do {
        height = (pp == list->prev[0])
            ? -pp->x[2]
            : pp->next[0]->x[2] - pp->x[2];
        if (pp->ignore >= 2)
            hyperv += hypera * height;
        else {
            avl_node_t *tnode;
            double nxt_ip0;
            if (avl_search_closest(tree, pp->x, &tnode) <= 0) {
                nxt_ip0 = node_point(tnode)[0];
                tnode = tnode->prev;
            } else {
                nxt_ip0 = (tnode->next != NULL)
                    ? node_point(tnode->next)[0]
                    : 0;
            }

            if (nxt_ip0 > pp->x[0]) {

                avl_insert_after(tree, tnode, pp->tnode);
                if (tnode != NULL) {
                    const double *prv_ip = node_point(tnode);

                    if (prv_ip[0] > pp->x[0]) {
                        tnode = pp->tnode->prev;
                        /* cur_ip = point dominated by pp with
                           highest [0]-coordinate */
                        const double * cur_ip = node_point(tnode);
                        while (tnode->prev) {
                            prv_ip = node_point(tnode->prev);
                            hypera -= (prv_ip[1] - cur_ip[1]) * (nxt_ip0 - cur_ip[0]);
                            if (prv_ip[0] < pp->x[0])
                                break; /* prv is not dominated by pp */
                            cur_ip = prv_ip;
                            avl_unlink_node(tree, tnode);
                            tnode = tnode->prev;
                        }

                        avl_unlink_node(tree, tnode);

                        if (!tnode->prev) {
                            hypera -=  -cur_ip[1] * (nxt_ip0 - cur_ip[0]);
                            hypera +=  -pp->x[1] * (nxt_ip0 - pp->x[0]);
                        } else {
                            hypera += (prv_ip[1] - pp->x[1]) * (nxt_ip0 - pp->x[0]);
                        }
                    } else {
                        hypera += (prv_ip[1] - pp->x[1]) * (nxt_ip0 - pp->x[0]);
                    }
                } else
                    hypera += -pp->x[1] * (nxt_ip0 - pp->x[0]);
            }
            else
                pp->ignore = 2;

            if (height > 0)
                hyperv += hypera * height;
        }
        pp = pp->next[0];
    } while (pp->x != NULL);

    avl_clear_tree(tree);
    return hyperv;
}

static double hv_recursive(avl_tree_t *tree, dlnode_t *list,
                           dimension_t dim, int c, double * restrict bound);

static void skip_or_recurse(dlnode_t *p, avl_tree_t *tree, dlnode_t *list,
                            dimension_t dim, int c, double * restrict bound)
{
    ASSUME(dim > STOP_DIMENSION);
    dimension_t d_stop = dim - STOP_DIMENSION;
    if (p->ignore >= dim) {
        p->area[d_stop] = p->prev[d_stop]->area[d_stop];
    } else {
        p->area[d_stop] = hv_recursive(tree, list, dim-1, c, bound);
        if (p->area[d_stop] <= p->prev[d_stop]->area[d_stop])
            p->ignore = dim;
    }
}

static double
hv_recursive(avl_tree_t *tree, dlnode_t *list,
             dimension_t dim, int c, double * restrict bound)
{
    /* ------------------------------------------------------
       General case for dimensions higher than 3D
       ------------------------------------------------------ */
    if (dim > STOP_DIMENSION) {
        dimension_t d_stop = dim - STOP_DIMENSION;
        dlnode_t *p1 = list->prev[d_stop];
        for (dlnode_t *pp = p1; pp->x; pp = pp->prev[d_stop]) {
            if (pp->ignore < dim)
                pp->ignore = 0;
        }
        dlnode_t *p0 = list;
        while (c > 1
               /* We delete all points x[dim] > bound[dim]. In case of
                  repeated coordinates, we also delete all points
                  x[dim] == bound[dim] except one. */
               && (p1->x[dim] > bound[dim]
                   || p1->prev[d_stop]->x[dim] >= bound[dim])
            ) {
            delete(p1, dim, bound);
            p0 = p1;
            p1 = p1->prev[d_stop];
            c--;
        }

        double hyperv = 0;
        if (c > 1) {
            hyperv = p1->prev[d_stop]->vol[d_stop] + p1->prev[d_stop]->area[d_stop]
                * (p1->x[dim] - p1->prev[d_stop]->x[dim]);
        } else {
            ASSUME(c == 1);
            double area = -p1->x[0];
            for (dimension_t i = 1; i <= STOP_DIMENSION; i++)
                area = area * -p1->x[i];
            p1->area[0] = area;
            for (dimension_t i = 1; i <= d_stop; i++)
                p1->area[i] = p1->area[i-1] * -p1->x[STOP_DIMENSION + i];
        }
        p1->vol[d_stop] = hyperv;
        skip_or_recurse(p1, tree, list, dim, c, bound);

        while (p0->x != NULL) {
            hyperv += p1->area[d_stop] * (p0->x[dim] - p1->x[dim]);
            bound[dim] = p0->x[dim];
            reinsert(p0, dim, bound);
            p1 = p0;
            p0 = p0->next[d_stop];
            p1->vol[d_stop] = hyperv;
            c++;
            skip_or_recurse(p1, tree, list, dim, c, bound);
        }
        hyperv += p1->area[d_stop] * -p1->x[dim];
        return hyperv;
    }

    /* ---------------------------
       special case of dimension 3
       --------------------------- */
    else if (dim == 2) {
        return fpli_hv3d(tree, list, c);
    }
    else
        fatal_error("%s:%d: unreachable condition! \n"
                    "This is a bug, please report it to "
                    "manuel.lopez-ibanez@manchester.ac.uk\n", __FILE__, __LINE__);
}

static int
compare_x_asc_y_asc (const void * restrict p1, const void * restrict p2)
{
    const double x1 = **(const double **)p1;
    const double x2 = **(const double **)p2;
    const double y1 = *(*(const double **)p1 + 1);
    const double y2 = *(*(const double **)p2 + 1);
    return (x1 < x2) ? -1 : ((x1 > x2) ? 1 :
                             ((y1 < y2) ? -1 : ((y1 > y2) ? 1 : 0)));
}

double hv2d(const double * restrict data, int n, const double * restrict ref)
{
    const double **p = malloc (n * sizeof(double *));
    if (unlikely(!p)) return -1;

    for (int k = 0; k < n; k++)
        p[k] = data + 2 * k;

    qsort(p, n, sizeof(*p), &compare_x_asc_y_asc);

    double hyperv = 0;
    double prev_j = ref[1];
    int j = 0;
    do {
        /* Filter everything that may be above the ref point. */
        while (j < n && p[j][1] >= prev_j)
            j++;
        if (unlikely(j == n || p[j][0] >= ref[0]))
            break; /* No other point dominates ref. */
        // We found one point that dominates ref.
        hyperv += (ref[0] - p[j][0]) * (prev_j - p[j][1]);
        prev_j = p[j][1];
        j++;
    } while (j < n && p[j][0] < ref[0]);

    free(p);
    return hyperv;
}

static void
shift_reference(double * restrict data, dimension_t d, size_t n,
                const double * restrict ref)
{
    for (size_t i = 0; i < n; i++)
        for (dimension_t k = 0; k < d; k++)
            data[i * d + k] -= ref[k];
}

double fpli_hv_shift(double * restrict data, int d, int n,
                     const double * restrict ref)
{
    if (unlikely(n == 0)) return 0.0;
    if (d == 2) return hv2d(data, n, ref);
    ASSUME(d < 256);
    ASSUME(d > 2);
    dimension_t dim = (dimension_t) d;

    // Shift the data so that the reference point is at zero [0,..., 0]
    shift_reference(data, dim, n, ref);
    avl_tree_t *tree = avl_alloc_tree ((avl_compare_t) compare_tree_asc,
                                       (avl_freeitem_t) NULL);
    double * zero_ref = calloc(dim, sizeof(double));
    dlnode_t *list = setup_cdllist(data, dim, &n, zero_ref);
    double hyperv;
    if (n == 0) {
        /* Returning here would leak memory.  */
	hyperv = 0.0;
    } else if (n == 1) {
        dlnode_t * p = list->next[0];
        hyperv = 1;
        for (int i = 0; i < dim; i++)
            hyperv *= -p->x[i];
    } else if (dim == 3) {
        hyperv = fpli_hv3d(tree, list, n);
    } else {
        double *bound = malloc (dim * sizeof(double));
        for (int i = 0; i < dim; i++) bound[i] = -DBL_MAX;
	hyperv = hv_recursive(tree, list, dim - 1, n, bound);
        free (bound);
    }
    /* Clean up.  */
    free (zero_ref);
    free_cdllist (list);
    free (tree);  /* The nodes are freed by free_cdllist ().  */
    return hyperv;
}

static double
fpli_hv3d_ref(avl_tree_t *tree, dlnode_t *list, int c,
              const double * restrict ref)
{
    dlnode_t *pp = list->next[0];
    double hypera = (ref[0] - pp->x[0]) * (ref[1] - pp->x[1]);
    double height = (c == 1)
        ? ref[2] - pp->x[2]
        : pp->next[0]->x[2] - pp->x[2];

    double hyperv = hypera * height;

    if (pp->next[0]->x == NULL)
        return hyperv;

    avl_insert_top(tree, pp->tnode);

    pp = pp->next[0];
    do {
        height = (pp == list->prev[0])
            ? ref[2] - pp->x[2]
            : pp->next[0]->x[2] - pp->x[2];
        ASSUME(height >= 0);
        if (pp->ignore < 2) {
            avl_node_t *tnode;
            double nxt_ip0;
            if (avl_search_closest(tree, pp->x, &tnode) <= 0) {
                nxt_ip0 = node_point(tnode)[0];
                tnode = tnode->prev;
            } else {
                nxt_ip0 = (tnode->next != NULL)
                    ? node_point(tnode->next)[0]
                    : ref[0];
            }

            if (nxt_ip0 > pp->x[0]) {
                const double *prv_ip;
                avl_insert_after(tree, tnode, pp->tnode);
                if (tnode != NULL) {
                    prv_ip = node_point(tnode);

                    if (prv_ip[0] > pp->x[0]) {
                        tnode = pp->tnode->prev;
                        /* cur_ip = point dominated by pp with
                           highest [0]-coordinate */
                        const double * cur_ip = node_point(tnode);
                        while (tnode->prev) {
                            prv_ip = node_point(tnode->prev);
                            hypera -= (prv_ip[1] - cur_ip[1]) * (nxt_ip0 - cur_ip[0]);
                            if (prv_ip[0] < pp->x[0])
                                break; /* prv is not dominated by pp */
                            cur_ip = prv_ip;
                            avl_unlink_node(tree, tnode);
                            tnode = tnode->prev;
                        }

                        avl_unlink_node(tree, tnode);

                        if (!tnode->prev) {
                            hypera -= (ref[1] - cur_ip[1]) * (nxt_ip0 - cur_ip[0]);
                            prv_ip = ref;
                        }
                    }
                } else
                    prv_ip = ref;

                hypera += (prv_ip[1] - pp->x[1])*(nxt_ip0 - pp->x[0]);
            }
            else
                pp->ignore = 2;
        }
        hyperv += hypera * height;
        pp = pp->next[0];
    } while (pp->x != NULL);

    avl_clear_tree(tree);
    return hyperv;
}

static double
hv_recursive_ref(avl_tree_t * restrict tree, dlnode_t * restrict list,
                 dimension_t dim, int c,
                 const double * restrict ref, double * restrict bound);

static void
skip_or_recurse_ref(dlnode_t *p, avl_tree_t *tree, dlnode_t *list,
                    dimension_t dim, int c,
                    const double * restrict ref, double * restrict bound)
{
    ASSUME(dim > STOP_DIMENSION);
    dimension_t d_stop = dim - STOP_DIMENSION;
    if (p->ignore >= dim) {
        p->area[d_stop] = p->prev[d_stop]->area[d_stop];
    } else {
        p->area[d_stop] = hv_recursive_ref(tree, list, dim-1, c, ref, bound);
        if (p->area[d_stop] <= p->prev[d_stop]->area[d_stop])
            p->ignore = dim;
    }
}

static double
hv_recursive_ref(avl_tree_t * restrict tree, dlnode_t * restrict list,
                 dimension_t dim, int c,
                 const double * restrict ref, double * restrict bound)
{
    /* ------------------------------------------------------
       General case for dimensions higher than 3D
       ------------------------------------------------------ */
    if ( dim > STOP_DIMENSION ) {
        dimension_t d_stop = dim - STOP_DIMENSION;
        dlnode_t *p1 = list->prev[d_stop];
        for (dlnode_t *pp = p1; pp->x; pp = pp->prev[d_stop]) {
            if (pp->ignore < dim)
                pp->ignore = 0;
        }
        dlnode_t *p0 = list;
        while (c > 1
               /* We delete all points x[dim] > bound[dim]. In case of
                  repeated coordinates, we also delete all points
                  x[dim] == bound[dim] except one. */
               && (p1->x[dim] > bound[dim]
                   || p1->prev[d_stop]->x[dim] >= bound[dim])
            ) {
            delete(p1, dim, bound);
            p0 = p1;
            p1 = p1->prev[d_stop];
            c--;
        }

        double hyperv = 0;
        if (c > 1) {
            hyperv = p1->prev[d_stop]->vol[d_stop] + p1->prev[d_stop]->area[d_stop]
                * (p1->x[dim] - p1->prev[d_stop]->x[dim]);
        } else {
            ASSUME(c == 1);
            double area = (ref[0] - p1->x[0]);
            for (int i = 1; i <= STOP_DIMENSION; i++)
                area = area * (ref[i] - p1->x[i]);
            p1->area[0] = area;
            for (int i = 1; i <= d_stop; i++)
                p1->area[i] = p1->area[i-1] * (ref[STOP_DIMENSION + i] - p1->x[STOP_DIMENSION + i]);
        }
        p1->vol[d_stop] = hyperv;
        skip_or_recurse_ref(p1, tree, list, dim, c, ref, bound);

        while (p0->x != NULL) {
            hyperv += p1->area[d_stop] * (p0->x[dim] - p1->x[dim]);
            bound[dim] = p0->x[dim];
            reinsert(p0, dim, bound);
            p1 = p0;
            p0 = p0->next[d_stop];
            p1->vol[d_stop] = hyperv;
            c++;
            skip_or_recurse_ref(p1, tree, list, dim, c, ref, bound);
        }
        hyperv += p1->area[d_stop] * (ref[dim] - p1->x[dim]);
        return hyperv;
    }

    /* ---------------------------
       special case of dimension 3
       --------------------------- */
    else if (dim == 2) {
        return fpli_hv3d_ref(tree, list, c, ref);
    }
    else
        fatal_error("%s:%d: unreachable condition! \n"
                    "This is a bug, please report it to "
                    "manuel.lopez-ibanez@manchester.ac.uk\n", __FILE__, __LINE__);
}

/*
   Returns 0 if no point strictly dominates ref.
   Returns -1 if out of memory.
*/
double fpli_hv(const double * restrict data, int d, int n,
               const double * restrict ref)
{
    if (unlikely(n == 0)) return 0.0;
    if (d == 2) return hv2d(data, n, ref);
    ASSUME(d < 256);
    ASSUME(d > 2);
    dimension_t dim = (dimension_t) d;
    dlnode_t * list = setup_cdllist(data, dim, &n, ref);
    double hyperv;
    if (unlikely(n == 0)) {
        /* Returning here would leak memory.  */
	hyperv = 0.0;
    } else if (unlikely(n == 1)) {
        dlnode_t * p = list->next[0];
        hyperv = 1;
        for (dimension_t i = 0; i < dim; i++)
            hyperv *= ref[i] - p->x[i];
    } else {
        avl_tree_t *tree = avl_alloc_tree((avl_compare_t) compare_tree_asc,
                                          (avl_freeitem_t) NULL);
        double * bound = malloc (dim * sizeof(double));
        for (dimension_t i = 0; i < dim; i++) bound[i] = -DBL_MAX;
	hyperv = hv_recursive_ref(tree, list, dim - 1, n, ref, bound);
        free (bound);
        free (tree);  /* The nodes are freed by free_cdllist ().  */
    }
    /* Clean up.  */
    free_cdllist (list);
    return hyperv;
}
