#define TREAP_ITEM_IS_EMPTY
#include "treap2d.h"

typedef struct {
    Treap treap;
    TreapNode * last_node;
    TreapNode node_pool[]; // Flexible array
} Treap2D;

Treap2D * treap2d_new(size_t n);
void treap2d_free(Treap2D * tree);
void treap2d_init_with_single_node(Treap2D *tree, double x, double y);
TreapNode * treap2d_find_le(const Treap2D *tree, double value);
TreapNode ** treap2d_find_le_link(Treap2D *tree, double value);
double treap2d_node_get_x(const TreapNode *node);
double treap2d_node_get_y(const TreapNode *node);
void treap2d_validate_tree(Treap2D * tree);
TreapNode * treap2d_erase_at(TreapNode **link);
void treap2d_insert(Treap2D *tree, double x, double y);
