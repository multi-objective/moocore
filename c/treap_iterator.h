#ifndef TREAP_ITERATOR_H
#define TREAP_ITERATOR_H

#define TREAP_ITERATOR_STACK_MAX 64

typedef struct TreapIterator {
    const TreapNode * stack[TREAP_ITERATOR_STACK_MAX];
    size_t stack_size;
    const TreapNode * current_node;
    size_t solution_index;
    // To detect modifications of the tree while iterating.
    uint64_t revision;
    const TreapArchive *arch;
} TreapIterator;


static bool
treap_archive_iter_push(TreapIterator *it, const TreapNode *node)
{
    assert(it->stack_size < TREAP_ITERATOR_STACK_MAX);
    if (it->stack_size == TREAP_ITERATOR_STACK_MAX)
        return false;

    it->stack[it->stack_size++] = node;
    return true;
}

static const TreapNode *
treap_archive_iter_pop(TreapIterator *it)
{
    return it->stack_size ? it->stack[--it->stack_size] : NULL;
}


void
treap_archive_iter_free(TreapIterator *it)
{
    if (it != NULL)
        free(it);
}

static void
treap_archive_iter_init(TreapIterator *it, const TreapArchive *arch)
{
    it->stack_size = 0;
    it->current_node = NULL;
    it->solution_index = 0;
    it->revision = arch->revision;
    it->arch = arch;
    /* Push the entire left spine of the tree. The smallest key will then be on
       top of the stack.  */
    const TreapNode *node = arch->tree.root;
    while (node != NULL) {
        if (!treap_archive_iter_push(it, node)) {
            // FIXME: Signal an error!
            return;
        }
        node = node->left;
    }
}

TreapIterator *
treap_archive_iter_new(const TreapArchive *arch)
{
    if (arch == NULL)
        return NULL;

    TreapIterator *it = malloc(sizeof(*it));
    if (it == NULL)
        return NULL;

    treap_archive_iter_init(it, arch);
    return it;
}

/**
   1  next element returned
   0  end of iteration
   -1  error
*/
int
treap_archive_iter_next(TreapIterator * restrict it, const double ** restrict z, const void ** restrict x)
{
    if (it == NULL || z == NULL || it->revision != it->arch->revision)
        return -1;

    // Continue emitting solutions from the node most recently popped.
    if (it->current_node != NULL) {
        const VoidList *item = &it->current_node->item;
        if (it->solution_index < item->n) {
            *z = &it->current_node->x;
            if (x)
                *x = item->list[it->solution_index];
            it->solution_index++;
            return 1;
        }
    }

    // Find the next node in in-order traversal.
    const VoidList *item;
    do {
        if (it->stack_size == 0) {
            *z = NULL;
            if (x)
                *x = NULL;
            return 0;
        }

        it->current_node = treap_archive_iter_pop(it);
        const TreapNode *node =  it->current_node->right;
        while (node != NULL) {
            if (!treap_archive_iter_push(it, node))
                return -1;
            node = node->left;
        }
        item = &it->current_node->item;
    } while (item->n == 0);

    *z = &it->current_node->x;
    if (x)
        *x = item->list[it->solution_index];
    it->solution_index++;
    return 1;
}

#endif // TREAP_ITERATOR_H
