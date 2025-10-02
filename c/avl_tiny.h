#ifndef _AVL_TINY_H_
#define _AVL_TINY_H_
/*-----------------------------------------------------------------------------

  The following is a reduced version of the AVL-tree library used here
  according to the terms of the LGPL-2.1-or-later.  See the copyright notice
  below.

*/

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
// "const void" here is actually "const avl_item_t"
typedef int (*avl_compare_t)(const void *, const void *);

typedef struct avl_tree_t {
    avl_node_t * head;
    avl_node_t * tail;
    avl_node_t * top;
    avl_compare_t cmp;
} avl_tree_t;


/*****************************************************************************

    avl.c - Source code for the AVL-tree library.

*****************************************************************************/

static void avl_rebalance(avl_tree_t *, avl_node_t *);

#define NODE_DEPTH(n) ((n) ? (n)->depth : 0)
#define L_DEPTH(n) (NODE_DEPTH((n)->left))
#define R_DEPTH(n) (NODE_DEPTH((n)->right))
#define CALC_DEPTH(n) \
    (unsigned char)((L_DEPTH(n) > R_DEPTH(n) ? L_DEPTH(n) : R_DEPTH(n)) + 1)

static int
avl_check_balance(const avl_node_t * avlnode)
{
    int d = R_DEPTH(avlnode) - L_DEPTH(avlnode);
    return d < -1 ? -1 : d > 1 ? 1 : 0;
}

/* Searches for a node with the key closest (or equal) to the given item.
   If avlnode is not NULL, *avlnode will be set to the node found or NULL
   if the tree is empty. Return values:
    -1  if the returned node is greater
     0  if the returned node is equal or if the tree is empty
     1  if the returned node is smaller
  O(lg n) */
static int
avl_search_closest(const avl_tree_t * avltree, const avl_item_t * item,
                   avl_node_t ** avlnode)
{
    avl_node_t * node = avltree->top;
    assert(node);
    avl_compare_t cmp = avltree->cmp;
    while (true) {
        int c = cmp(item, node->item);
        if (c < 0) {
            if (node->left)
                node = node->left;
            else
                return *avlnode = node, -1;
        } else if (c > 0) {
            if (node->right)
                node = node->right;
            else
                return *avlnode = node, 1;
        } else {
            return *avlnode = node, 0;
        }
    }
}

/* Initializes a new tree for elements that will be ordered using
 * the supplied strcmp()-like function.
 * Returns the value of avltree (even if it's NULL).
 * O(1) */
static avl_tree_t *
avl_init_tree(avl_tree_t * rc, avl_compare_t cmp)
{
    rc->head = NULL;
    rc->tail = NULL;
    rc->top = NULL;
    rc->cmp = cmp;
    return rc;
}

/* Reinitializes the tree structure for reuse. Nothing is free()d.
 * Compare and freeitem functions are left alone.
 * O(1) */
static void
avl_clear_node(avl_node_t * newnode)
{
    newnode->left = newnode->right = NULL;
    newnode->depth = 1;
}

/* Insert a node in an empty tree. If avlnode is NULL, the tree will be
 * cleared and ready for re-use.
 * If the tree is not empty, the old nodes are left dangling.
 * O(1) */
static avl_node_t *
avl_insert_top(avl_tree_t * avltree, avl_node_t * newnode)
{
    avl_clear_node(newnode);
    newnode->prev = newnode->next = newnode->parent = NULL;
    avltree->head = avltree->tail = avltree->top = newnode;
    return newnode;
}

static avl_node_t * avl_insert_after(avl_tree_t * avltree, avl_node_t * node,
                                     avl_node_t * newnode);

/* Insert a node before another node. Returns the new node.
 * If old is NULL, the item is appended to the tree.
 * O(lg n) */
static avl_node_t *
avl_insert_before(avl_tree_t * avltree, avl_node_t * node, avl_node_t * newnode)
{
    if (!node)
        return avltree->tail ? avl_insert_after(avltree, avltree->tail, newnode)
                             : avl_insert_top(avltree, newnode);

    if (node->left) return avl_insert_after(avltree, node->prev, newnode);

    assert(node);
    assert(!node->left);

    avl_clear_node(newnode);

    newnode->next = node;
    newnode->parent = node;

    newnode->prev = node->prev;
    if (node->prev)
        node->prev->next = newnode;
    else
        avltree->head = newnode;
    node->prev = newnode;

    node->left = newnode;
    avl_rebalance(avltree, node);
    return newnode;
}

/* Insert a node after another node. Returns the new node.
 * If old is NULL, the item is prepended to the tree.
 * O(lg n) */
static avl_node_t *
avl_insert_after(avl_tree_t * avltree, avl_node_t * node, avl_node_t * newnode)
{
    if (!node)
        return avltree->head
                   ? avl_insert_before(avltree, avltree->head, newnode)
                   : avl_insert_top(avltree, newnode);

    if (node->right) return avl_insert_before(avltree, node->next, newnode);

    assert(node);
    assert(!node->right);

    avl_clear_node(newnode);

    newnode->prev = node;
    newnode->parent = node;

    newnode->next = node->next;
    if (node->next)
        node->next->prev = newnode;
    else
        avltree->tail = newnode;
    node->next = newnode;

    node->right = newnode;
    avl_rebalance(avltree, node);
    return newnode;
}

/* Deletes the node from the tree.  Does not delete the item at that node.
 * The item of the node may be freed before calling avl_unlink_node.
 * (In other words, it is not referenced by this function.)
 * O(lg n) */
static void
avl_unlink_node(avl_tree_t * avltree, avl_node_t * avlnode)
{
    avl_node_t * parent;
    avl_node_t ** superparent;
    avl_node_t *subst, *left, *right;
    avl_node_t * balnode;

    if (avlnode->prev)
        avlnode->prev->next = avlnode->next;
    else
        avltree->head = avlnode->next;

    if (avlnode->next)
        avlnode->next->prev = avlnode->prev;
    else
        avltree->tail = avlnode->prev;

    parent = avlnode->parent;

    superparent = parent
                      ? avlnode == parent->left ? &parent->left : &parent->right
                      : &avltree->top;

    left = avlnode->left;
    right = avlnode->right;
    if (!left) {
        *superparent = right;
        if (right) right->parent = parent;
        balnode = parent;
    } else if (!right) {
        *superparent = left;
        left->parent = parent;
        balnode = parent;
    } else {
        subst = avlnode->prev;
        if (subst == left) {
            balnode = subst;
        } else {
            balnode = subst->parent;
            balnode->right = subst->left;
            if (balnode->right) balnode->right->parent = balnode;
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
avl_rebalance(avl_tree_t * avltree, avl_node_t * avlnode)
{
    avl_node_t * child;
    avl_node_t * gchild;
    avl_node_t * parent;
    avl_node_t ** superparent;

    parent = avlnode;

    while (avlnode) {
        parent = avlnode->parent;

        superparent =
            parent ? avlnode == parent->left ? &parent->left : &parent->right
                   : &avltree->top;

        switch (avl_check_balance(avlnode)) {
        case -1:
            child = avlnode->left;
            if (L_DEPTH(child) >= R_DEPTH(child)) {
                avlnode->left = child->right;
                if (avlnode->left) avlnode->left->parent = avlnode;
                child->right = avlnode;
                avlnode->parent = child;
                *superparent = child;
                child->parent = parent;
                avlnode->depth = CALC_DEPTH(avlnode);
                child->depth = CALC_DEPTH(child);
            } else {
                gchild = child->right;
                avlnode->left = gchild->right;
                if (avlnode->left) avlnode->left->parent = avlnode;
                child->right = gchild->left;
                if (child->right) child->right->parent = child;
                gchild->right = avlnode;
                if (gchild->right) gchild->right->parent = gchild;
                gchild->left = child;
                if (gchild->left) gchild->left->parent = gchild;
                *superparent = gchild;
                gchild->parent = parent;
                avlnode->depth = CALC_DEPTH(avlnode);
                child->depth = CALC_DEPTH(child);
                gchild->depth = CALC_DEPTH(gchild);
            }
            break;
        case 1:
            child = avlnode->right;
            if (R_DEPTH(child) >= L_DEPTH(child)) {
                avlnode->right = child->left;
                if (avlnode->right) avlnode->right->parent = avlnode;
                child->left = avlnode;
                avlnode->parent = child;
                *superparent = child;
                child->parent = parent;
                avlnode->depth = CALC_DEPTH(avlnode);
                child->depth = CALC_DEPTH(child);
            } else {
                gchild = child->left;
                avlnode->right = gchild->left;
                if (avlnode->right) avlnode->right->parent = avlnode;
                child->left = gchild->right;
                if (child->left) child->left->parent = child;
                gchild->left = avlnode;
                if (gchild->left) gchild->left->parent = gchild;
                gchild->right = child;
                if (gchild->right) gchild->right->parent = gchild;
                *superparent = gchild;
                gchild->parent = parent;
                avlnode->depth = CALC_DEPTH(avlnode);
                child->depth = CALC_DEPTH(child);
                gchild->depth = CALC_DEPTH(gchild);
            }
            break;
        default:
            avlnode->depth = CALC_DEPTH(avlnode);
        }
        avlnode = parent;
    }
}

/*-----------------------------------------------------------------------------
 end of functions from AVL-tree library.
******************************************************************************/
#endif
