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
#include "common.h"
#include "hv.h"

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

typedef struct avl_node_t {
	struct avl_node_t *next;
	struct avl_node_t *prev;
	struct avl_node_t *parent;
	struct avl_node_t *left;
	struct avl_node_t *right;
        const void *item;
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
#endif
}

static int
avl_search_closest(const avl_tree_t *avltree, const void *item, avl_node_t **avlnode) {
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
    int ignore;
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
 * Setup circular double-linked list in each dimension
 */

static dlnode_t *
setup_cdllist(const double *data, int d, int n)
{
    dlnode_t *head  = malloc ((n+1) * sizeof(dlnode_t));

    head->x = data;
    head->ignore = 0;  /* should never get used */
    head->next = malloc( d * (n+1) * sizeof(dlnode_t*));
    head->prev = malloc( d * (n+1) * sizeof(dlnode_t*));
    head->tnode = malloc((n+1) * sizeof(avl_node_t));

    head->area = malloc(d * (n+1) * sizeof(double));
    head->vol = malloc(d * (n+1) * sizeof(double));

    int i, j;
    for (i = 1; i <= n; i++) {
        head[i].x = head[i-1].x + d;/* this will be fixed a few lines below... */
        head[i].ignore = 0;
        head[i].next = head[i-1].next + d;
        head[i].prev = head[i-1].prev + d;
        head[i].tnode = head[i-1].tnode + 1;
        head[i].area = head[i-1].area + d;
        head[i].vol = head[i-1].vol + d;
    }
    head->x = NULL; /* head contains no data */

    dlnode_t **scratch = malloc(n * sizeof(dlnode_t*));
    for (i = 0; i < n; i++)
        scratch[i] = head + i + 1;

    for (j = d-1; j >= 0; j--) {
        for (i = 0; i < n; i++)
            scratch[i]->x--;
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

    for (i = 0; i < d; i++)
        head->area[i] = 0;

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

static void delete (dlnode_t *nodep, int dim, double * bound)
{
    int i;

    for (i = STOP_DIMENSION; i < dim; i++) {
        nodep->prev[i]->next[i] = nodep->next[i];
        nodep->next[i]->prev[i] = nodep->prev[i];
        if (bound[i] > nodep->x[i])
            bound[i] = nodep->x[i];
  }
}

static void reinsert (dlnode_t *nodep, int dim, double * bound)
{
    for (int i = STOP_DIMENSION; i < dim; i++) {
        nodep->prev[i]->next[i] = nodep;
        nodep->next[i]->prev[i] = nodep;
        if (bound[i] > nodep->x[i])
            bound[i] = nodep->x[i];
    }
}

static double
fpli_hv3d(avl_tree_t *tree, dlnode_t *list, int c)
{
    dlnode_t *pp = list->next[2];
    double hypera = pp->x[0] * pp->x[1];
    double height = (c == 1)
        ? -pp->x[2]
        : pp->next[2]->x[2] - pp->x[2];

    double hyperv = hypera * height;

    if (pp->next[2]->x == NULL)
        return hyperv;

    avl_insert_top(tree, pp->tnode);

    pp = pp->next[2];
    do {
        height = (pp == list->prev[2])
            ? -pp->x[2]
            : pp->next[2]->x[2] - pp->x[2];
        if (pp->ignore >= 2)
            hyperv += hypera * height;
        else {
            avl_node_t *tnode;
            double nxt_ip0;
            if (avl_search_closest(tree, pp->x, &tnode) <= 0) {
                nxt_ip0 = ((double *)(tnode->item))[0];
                tnode = tnode->prev;
            } else {
                nxt_ip0 = (tnode->next != NULL)
                    ? ((double *)(tnode->next->item))[0]
                    : 0;
            }

            if (nxt_ip0 > pp->x[0]) {

                avl_insert_after(tree, tnode, pp->tnode);
                if (tnode != NULL) {
                    const double *prv_ip = (double *)(tnode->item);

                    if (prv_ip[0] > pp->x[0]) {
                        tnode = pp->tnode->prev;
                        /* cur_ip = point dominated by pp with
                           highest [0]-coordinate */
                        const double * cur_ip = (double *)(tnode->item);
                        while (tnode->prev) {
                            prv_ip = (double *)(tnode->prev->item);
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
        pp = pp->next[2];
    } while (pp->x != NULL);

    avl_clear_tree(tree);
    return hyperv;
}

static double hv_recursive(avl_tree_t *tree, dlnode_t *list,
                           int dim, int c, double * bound);

static void skip_or_recurse(dlnode_t *p, avl_tree_t *tree, dlnode_t *list,
                            int dim, int c, double * bound)
{
    if (p->ignore >= dim) {
        p->area[dim] = p->prev[dim]->area[dim];
    } else {
        p->area[dim] = hv_recursive(tree, list, dim-1, c, bound);
        if (p->area[dim] <= p->prev[dim]->area[dim])
            p->ignore = dim;
    }
}

static double
hv_recursive(avl_tree_t *tree, dlnode_t *list,
             int dim, int c, double * bound)
{
    /* ------------------------------------------------------
       General case for dimensions higher than 3D
       ------------------------------------------------------ */
    if (dim > STOP_DIMENSION) {
        dlnode_t *p0 = list;
        dlnode_t *p1 = list->prev[dim];
        double hyperv = 0;
        dlnode_t *pp;
        for (pp = p1; pp->x; pp = pp->prev[dim]) {
            if (pp->ignore < dim)
                pp->ignore = 0;
        }
        while (c > 1
               /* We delete all points x[dim] > bound[dim]. In case of
                  repeated coordinates, we also delete all points
                  x[dim] == bound[dim] except one. */
               && (p1->x[dim] > bound[dim]
                   || p1->prev[dim]->x[dim] >= bound[dim])
            ) {
            p0 = p1;
            delete(p0, dim, bound);
            p1 = p0->prev[dim];
            c--;
        }

        if (c > 1) {
            hyperv = p1->prev[dim]->vol[dim] + p1->prev[dim]->area[dim]
                * (p1->x[dim] - p1->prev[dim]->x[dim]);
            p1->vol[dim] = hyperv;
        } else {
            p1->area[0] = 1;
            for (int i = 1; i <= dim; i++)
                p1->area[i] = p1->area[i-1] *  -p1->x[i-1];
            p1->vol[dim] = 0;
        }
        skip_or_recurse(p1, tree, list, dim, c, bound);

        while (p0->x != NULL) {
            hyperv += p1->area[dim] * (p0->x[dim] - p1->x[dim]);
            bound[dim] = p0->x[dim];
            reinsert(p0, dim, bound);
            p1 = p0;
            p0 = p0->next[dim];
            p1->vol[dim] = hyperv;
            c++;
            skip_or_recurse(p1, tree, list, dim, c, bound);
        }
        hyperv += p1->area[dim] * -p1->x[dim];
        return hyperv;
    }

    /* ---------------------------
       special case of dimension 3
       --------------------------- */
    else if (dim == 2) {
        return fpli_hv3d(tree, list, c);
    }
    /* FIXME: This is only interesting for measuring the effect of stopping the
     recursion, we should have a dedicated hv2d function with the O(n * log *
     n) case */
    else if (dim == 1) {
        const dlnode_t *p1 = list->next[1];
        double hypera = p1->x[0];
        double hyperv = 0;
        const dlnode_t *p0;

        while ((p0 = p1->next[1])->x) {
            hyperv +=  -hypera * (p0->x[1] - p1->x[1]);
            if (p0->x[0] < hypera)
                hypera = p0->x[0];
            p1 = p0;
        }
        hyperv += hypera * p1->x[1];
        return hyperv;
    }
    /* This is only interesting for measuring the effect of stopping the
       recursion at this dimension. */
    else if (dim == 0) {
        return (-list->next[0]->x[0]);
    }
    else
        fatal_error("%s:%d: unreachable condition! \n"
                    "This is a bug, please report it to "
                    "manuel.lopez-ibanez@manchester.ac.uk\n", __FILE__, __LINE__);
}

/*
  Removes the point from the circular double-linked list, but it
  doesn't remove the data.
*/
static void
filter_delete_node(dlnode_t *node, int d)
{
    /* The memory allocated for the deleted node is lost (leaked)
       until the end of the program, but this should not be a problem. */
    for (int i = 0; i < d; i++) {
        node->next[i]->prev[i] = node->prev[i];
        node->prev[i]->next[i] = node->next[i];
    }
}

/*
  Filters those points that do not strictly dominate the reference
  point.  This is needed to assure that the points left are only those
  that are needed to calculate the hypervolume.
*/
static int
filter(dlnode_t *list, int d, int n)
{
    int i, j;
    /* fprintf (stderr, "%d points initially\n", n); */
    for (i = 0; i < d; i++) {
        dlnode_t *aux = list->prev[i];
        const int np = n;
        for (j = 0; j < np; j++) {
            if (aux->x[i] < 0)
                break;
            filter_delete_node (aux, d);
            aux = aux->prev[i];
            n--;
        }
    }
    /* fprintf (stderr, "%d points remain\n", n); */
    return n;
}

static int compare_x_asc_y_asc (const void *p1, const void *p2)
{
    const double x1 = **(const double **)p1;
    const double x2 = **(const double **)p2;
    const double y1 = *(*(const double **)p1 + 1);
    const double y2 = *(*(const double **)p2 + 1);
    return (x1 < x2) ? -1 : ((x1 > x2) ? 1 :
                             ((y1 < y2) ? -1 : ((y1 > y2) ? 1 : 0)));
}

double hv2d(const double *data, int n, const double *ref)
{
    const double **p = malloc (n * sizeof(double *));

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
        if (j == n || p[j][0] >= ref[0])
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
shift_reference(double *data, size_t d, size_t n, const double *ref)
{
    for (size_t i = 0; i < n; i++)
        for (size_t k = 0; k < d; k++)
            data[i * d + k] -= ref[k];
}

double fpli_hv_shift(double *data, int d, int n, const double *ref)
{
    if (n == 0) return 0.0;
    if (d == 2) return hv2d(data, n, ref);

    // Shift the data so that the reference point is at zero [0,..., 0]
    shift_reference(data, d, n, ref);
    double hyperv;
    avl_tree_t *tree = avl_alloc_tree ((avl_compare_t) compare_tree_asc,
                                       (avl_freeitem_t) NULL);
    dlnode_t *list = setup_cdllist(data, d, n);
    // Filtering using the list is faster.
    n = filter(list, d, n);
    if (n == 0) {
        /* Returning here would leak memory.  */
	hyperv = 0.0;
    } else if (n == 1) {
        dlnode_t * p = list->next[0];
        hyperv = 1;
        for (int i = 0; i < d; i++)
            hyperv *= -p->x[i];
    } else if (d == 3) {
        hyperv = fpli_hv3d(tree, list, n);
    } else {
        double *bound = malloc (d * sizeof(double));
        for (int i = 0; i < d; i++) bound[i] = -DBL_MAX;
	hyperv = hv_recursive(tree, list, d-1, n, bound);
        free (bound);
    }
    /* Clean up.  */
    free_cdllist (list);
    free (tree);  /* The nodes are freed by free_cdllist ().  */
    return hyperv;
}

static double
fpli_hv3d_ref(avl_tree_t *tree, dlnode_t *list, int c, const double *ref)
{
    dlnode_t *pp = list->next[2];
    double hypera = (ref[0] - pp->x[0]) * (ref[1] - pp->x[1]);
    double height = (c == 1)
        ? ref[2] - pp->x[2]
        : pp->next[2]->x[2] - pp->x[2];

    double hyperv = hypera * height;

    if (pp->next[2]->x == NULL)
        return hyperv;

    avl_insert_top(tree, pp->tnode);

    pp = pp->next[2];
    do {
        height = (pp == list->prev[2])
            ? ref[2] - pp->x[2]
            : pp->next[2]->x[2] - pp->x[2];
        if (pp->ignore >= 2)
            hyperv += hypera * height;
        else {
            avl_node_t *tnode;
            const double *nxt_ip;
            if (avl_search_closest(tree, pp->x, &tnode) <= 0) {
                nxt_ip = (double *)(tnode->item);
                tnode = tnode->prev;
            } else {
                nxt_ip = (tnode->next != NULL)
                    ? (double *)(tnode->next->item)
                    : ref;
            }

            if (nxt_ip[0] > pp->x[0]) {
                const double *prv_ip;
                avl_insert_after(tree, tnode, pp->tnode);
                if (tnode != NULL) {
                    prv_ip = (double *)(tnode->item);

                    if (prv_ip[0] > pp->x[0]) {
                        tnode = pp->tnode->prev;
                        /* cur_ip = point dominated by pp with
                           highest [0]-coordinate */
                        const double * cur_ip = (double *)(tnode->item);
                        while (tnode->prev) {
                            prv_ip = (double *)(tnode->prev->item);
                            hypera -= (prv_ip[1] - cur_ip[1]) * (nxt_ip[0] - cur_ip[0]);
                            if (prv_ip[0] < pp->x[0])
                                break; /* prv is not dominated by pp */
                            cur_ip = prv_ip;
                            avl_unlink_node(tree, tnode);
                            tnode = tnode->prev;
                        }

                        avl_unlink_node(tree, tnode);

                        if (!tnode->prev) {
                            hypera -= (ref[1] - cur_ip[1]) * (nxt_ip[0] - cur_ip[0]);
                            prv_ip = ref;
                        }
                    }
                } else
                    prv_ip = ref;

                hypera += (prv_ip[1] - pp->x[1])*(nxt_ip[0] - pp->x[0]);
            }
            else
                pp->ignore = 2;

            if (height > 0)
                hyperv += hypera * height;
        }
        pp = pp->next[2];
    } while (pp->x != NULL);

    avl_clear_tree(tree);
    return hyperv;
}

static double
hv_recursive_ref(avl_tree_t *tree, dlnode_t *list,
                 int dim, int c, const double * ref, double * bound);

static void skip_or_recurse_ref(dlnode_t *p, avl_tree_t *tree, dlnode_t *list,
                                int dim, int c, const double * ref, double * bound)
{
    if (p->ignore >= dim) {
        p->area[dim] = p->prev[dim]->area[dim];
    } else {
        p->area[dim] = hv_recursive_ref(tree, list, dim-1, c, ref, bound);
        if (p->area[dim] <= p->prev[dim]->area[dim])
            p->ignore = dim;
    }
}

static double
hv_recursive_ref(avl_tree_t *tree, dlnode_t *list,
                 int dim, int c, const double * ref, double * bound)
{
    /* ------------------------------------------------------
       General case for dimensions higher than 3D
       ------------------------------------------------------ */
    if ( dim > STOP_DIMENSION ) {
        dlnode_t *p0 = list;
        dlnode_t *p1 = list->prev[dim];
        double hyperv = 0;
        dlnode_t *pp;
        for (pp = p1; pp->x; pp = pp->prev[dim]) {
            if (pp->ignore < dim)
                pp->ignore = 0;
        }
        while (c > 1
               /* We delete all points x[dim] > bound[dim]. In case of
                  repeated coordinates, we also delete all points
                  x[dim] == bound[dim] except one. */
               && (p1->x[dim] > bound[dim]
                   || p1->prev[dim]->x[dim] >= bound[dim])
            ) {
            p0 = p1;
            delete(p0, dim, bound);
            p1 = p0->prev[dim];
            c--;
        }

        if (c > 1) {
            hyperv = p1->prev[dim]->vol[dim] + p1->prev[dim]->area[dim]
                * (p1->x[dim] - p1->prev[dim]->x[dim]);
            p1->vol[dim] = hyperv;
        } else {
            p1->area[0] = 1;
            for (int i = 1; i <= dim; i++)
                p1->area[i] = p1->area[i-1] * (ref[i-1] - p1->x[i-1]);
            p1->vol[dim] = 0;
        }
        skip_or_recurse_ref(p1, tree, list, dim, c, ref, bound);

        while (p0->x != NULL) {
            hyperv += p1->area[dim] * (p0->x[dim] - p1->x[dim]);
            bound[dim] = p0->x[dim];
            reinsert(p0, dim, bound);
            p1 = p0;
            p0 = p0->next[dim];
            p1->vol[dim] = hyperv;
            c++;
            skip_or_recurse_ref(p1, tree, list, dim, c, ref, bound);
        }
        hyperv += p1->area[dim] * (ref[dim] - p1->x[dim]);
        return hyperv;
    }

    /* ---------------------------
       special case of dimension 3
       --------------------------- */
    else if (dim == 2) {
        return fpli_hv3d_ref(tree, list, c, ref);
    }
    /* This is only interesting for measuring the effect of stopping the
       recursion, but this code is currently unused because we have a dedicated
       hv2d function that is already O(n * log n). */
    else if (dim == 1) {
        const dlnode_t *p1 = list->next[1];
        double hypera = p1->x[0];
        double hyperv = 0;
        const dlnode_t *p0;

        while ((p0 = p1->next[1])->x) {
            hyperv += (ref[0] - hypera) * (p0->x[1] - p1->x[1]);
            if (p0->x[0] < hypera)
                hypera = p0->x[0];
            p1 = p0;
        }
        hyperv += (ref[0] - hypera) * (ref[1] - p1->x[1]);
        return hyperv;
    }

    /* special case of dimension 1 */
    else if (dim == 0) {
        return (ref[0] - list->next[0]->x[0]);
    }
    else
        fatal_error("%s:%d: unreachable condition! \n"
                    "This is a bug, please report it to "
                    "manuel.lopez-ibanez@manchester.ac.uk\n", __FILE__, __LINE__);
}

/*
  Filters those points that do not strictly dominate the reference
  point.  This is needed to assure that the points left are only those
  that are needed to calculate the hypervolume.
*/
static int
filter_ref(dlnode_t *list, int d, int n, const double *ref)
{
    int i, j;
    /* fprintf (stderr, "%d points initially\n", n); */
    for (i = 0; i < d; i++) {
        dlnode_t *aux = list->prev[i];
        const int np = n;
        for (j = 0; j < np; j++) {
            if (aux->x[i] < ref[i])
                break;
            filter_delete_node (aux, d);
            aux = aux->prev[i];
            n--;
        }
    }
    /* fprintf (stderr, "%d points remain\n", n); */
    return n;
}

double fpli_hv(const double *data, int d, int n, const double *ref)
{
    if (n == 0) return 0.0;
    if (d == 2) return hv2d(data, n, ref);

    avl_tree_t *tree = avl_alloc_tree((avl_compare_t) compare_tree_asc,
                                       (avl_freeitem_t) NULL);
    dlnode_t *list = setup_cdllist(data, d, n);

    double hyperv;
    n = filter_ref(list, d, n, ref);
    if (n == 0) {
        /* Returning here would leak memory.  */
	hyperv = 0.0;
    } else if (n == 1) {
        dlnode_t * p = list->next[0];
        hyperv = 1;
        for (int i = 0; i < d; i++)
            hyperv *= ref[i] - p->x[i];
    } else {
        double *bound = malloc (d * sizeof(double));
        for (int i = 0; i < d; i++) bound[i] = -DBL_MAX;
	hyperv = hv_recursive_ref(tree, list, d - 1, n, ref, bound);
        free (bound);
    }
    /* Clean up.  */
    free_cdllist (list);
    free (tree);  /* The nodes are freed by free_cdllist ().  */

    return hyperv;
}
