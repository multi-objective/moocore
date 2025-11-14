#include "nondominated.h"

#if DEBUG >= 2
#include "io.h"
#endif

/**
   Nondominated sorting in 23 in O(k * n log n), where k is the number of fronts.
*/

static int *
pareto_rank_3d(const double * restrict points, size_t size)
{
    const bool keep_weakly = true;
    int * rank = calloc(size, sizeof(*rank));
    int front = 0;

    ASSUME(size >= 2);
    const double **p = generate_sorted_pp_3d(points, size);

    avl_tree_t tree;
    avl_init_tree(&tree, cmp_pdouble_asc_x_nonzero);
    avl_node_t * tnodes = malloc((size+1) * sizeof(*tnodes));
    while (true) {
        avl_node_t * node = tnodes;
        node->item = p[0];
        avl_insert_top(&tree, node);

        const double sentinel[] = { INFINITY, -INFINITY };
        (++node)->item = sentinel;
        avl_insert_after(&tree, node - 1, node);

        // In this context, size means "no dominated solution found".
        size_t pos_last_dom = size, n_nondom = size, j = 1;
        const double * restrict pk = p[0];
        do {
            const double * restrict pj = p[j];
            // printf("pj: "); print_point(pj); printf("\n");
            bool dominated;
            if (pk[0] > pj[0] || pk[1] > pj[1]) {
                avl_node_t * nodeaux;
                int res = avl_search_closest(&tree, pj, &nodeaux);
                assert(res != 0);
                if (res > 0) { // nodeaux goes before pj
                    const double * restrict prev = nodeaux->item;
                    assert(prev[0] != sentinel[0]);
                    assert(prev[0] <= pj[0]);
                    dominated = prev[1] <= pj[1];
                    // printf("res > 0: prev: "); print_point(prev); printf("\n");
                    nodeaux = nodeaux->next;
                } else if (nodeaux->prev) {// nodeaux goes after pj, so move to the next one.
                    const double * restrict prev = nodeaux->prev->item;
                    assert(prev[0] != sentinel[0]);
                    assert(prev[0] <= pj[0]);
                    dominated = prev[1] <= pj[1];
                    // printf("res < 0: prev: "); print_point(prev); printf("\n");
                } else {
                    dominated = false;
                }

                if (!dominated) { // pj is NOT dominated by a point in the tree.
                    const double * restrict point = nodeaux->item;
                    assert(pj[0] <= point[0]);
                    // Delete everything in the tree that is dominated by pj.
                    while (pj[1] <= point[1]) {
                        // printf("delete point: "); print_point(point); printf("\n");
                        assert(pj[0] <= point[0]);
                        nodeaux = nodeaux->next;
                        point = nodeaux->item;
                        /* FIXME: A possible speed up is to delete without
                           rebalancing the tree because avl_insert_before() will
                           rebalance. */
                        avl_unlink_node(&tree, nodeaux->prev);
                    }
                    // printf("insert before point: "); print_point(point); printf("\n");
                    (++node)->item = pj;
                    avl_insert_before(&tree, nodeaux, node);
                }
            } else {
                // Handle duplicates and points that are dominated by the immediate
                // previous one.
                const bool k_eq_j = (pk[0] == pj[0]) & (pk[1] == pj[1]) & (pk[2] == pj[2]);
                dominated = !keep_weakly;
                if (dominated) { // We don't keep duplicates;
                    if (unlikely(k_eq_j) && pj < pk) // Only the first duplicated point is kept.
                        SWAP(pk, pj);
                } else { // or it is not a duplicate, so it is non-weakly dominated;
                    dominated = likely(!k_eq_j)
                        // or pk was dominated, then this one is also dominated.
                        || pos_last_dom == (size_t) ((pk - points) / 3);
                }
                // printf("dom by prev: "); print_point(prev); printf("\n");
            }
            if (dominated) { // pj is dominated by a point in the tree or by prev.
                /* Map the order in p[], which is sorted, to the original order in
                   points. */
                pos_last_dom = (pj - points) / 3;
                assert(rank[pos_last_dom] == front);
                rank[pos_last_dom]++;
                n_nondom--;
            } else {
                pk = pj;
            }
            j++;
        } while (j < size);

        if (n_nondom == size) {
            free(tnodes);
            free(p);
            return rank;
        }

        size_t k = 0;
        assert(rank[(p[k] - points) / 3] == front);
        size_t n = k;
        while (k < (size - n_nondom)) {
            do {
                n++;
            } while (rank[(p[n] - points) / 3] == front);
            p[k] = p[n];
            k++;
        }
        front++;
        avl_clear_tree(&tree);
        size -= n_nondom;
    }
}

/*
   Nondominated sorting in 2D in O(n log n) from:

   M. T. Jensen. Reducing the run-time complexity of multiobjective
   EAs: The NSGA-II and other algorithms. IEEE Transactions on
   Evolutionary Computation, 7(5):503–515, 2003.
*/

static int *
pareto_rank_2d(const double * restrict points, size_t size)
{
    const int dim = 2;
    const double ** p = malloc(size * sizeof(*p));
    for (size_t k = 0; k < size; k++)
        p[k] = points + dim * k;

#if DEBUG >= 2
#define PARETO_RANK_2D_DEBUG
    double *help_0 = malloc(size * sizeof(double));
    double *help_1 = malloc(size * sizeof(double));
    int *   help_i = malloc(size * sizeof(int));

    for (size_t k = 0; k < size; k++) {
        help_0[k] = p[k][0];
        help_1[k] = p[k][1];
        help_i[k] = k;
    }
    fprintf(stderr, "%s():\n-------------------\n>>INPUT:", __FUNCTION__);
    fprintf(stderr, "\nIndex: "); vector_int_fprintf(stderr, help_i, size);
    fprintf(stderr, "\n[0]  : "); vector_fprintf (stderr, help_0, size);
    fprintf(stderr, "\n[1]  : "); vector_fprintf (stderr, help_1, size);
#endif

    // Sort in ascending lexicographic order from the first dimension.
    qsort(p, size, sizeof(*p), cmp_double_asc_rev_2d);

#ifdef PARETO_RANK_2D_DEBUG
    for (size_t k = 0; k < size; k++) {
       help_0[k] = p[k][0];
       help_1[k] = p[k][1];
       help_i[k] = (p[k]  - points) / dim;
    }
    fprintf(stderr, "%s():\n-------------------\n>>SORTED:", __FUNCTION__);
    fprintf(stderr, "\nIndex: "); vector_int_fprintf(stderr, help_i, size);
    fprintf(stderr, "\n[0]  : "); vector_fprintf (stderr, help_0, size);
    fprintf(stderr, "\n[1]  : "); vector_fprintf (stderr, help_1, size);
#endif

    int * rank = malloc(size * sizeof(*rank));
    const double ** front_last = malloc(size * sizeof(*front_last));
    int n_front = 0;
    front_last[0] = p[0];
    rank[(p[0] - points) / dim] = 0; // The first point is in the first front.
    for (size_t k = 1; k < size; k++) {
        const double * pk = p[k];
        const double * plast = front_last[n_front];
        if (pk[0] < plast[0]) {
            int low = 0;
            int high = n_front + 1;
            do {
                int mid = low + (high - low) / 2;
                assert (mid <= n_front);
                const double * pmid = front_last[mid];
                if (pk[0] < pmid[0])
                    high = mid;
                else if (pk[0] > pmid[0] || (pk[0] == pmid[0] && pk[1] > pmid[1]))
                    low = mid + 1;
                else { // Duplicated points are assigned to the same front.
                    low = mid;
                    break;
                }
            } while (low < high);
            assert (low <= n_front);
            assert (pk[0] < front_last[low][0]
                    || (pk[0] == front_last[low][0]
                        && pk[1] == front_last[low][1]));
            front_last[low] = pk;
            rank[(pk - points)/dim] = low;
        } else if (pk[0] == plast[0] && pk[1] == plast[1]) {
            front_last[n_front] = pk;
            rank[(pk - points)/dim] = n_front;
        } else {
            n_front++;
            front_last[n_front] = pk;
            rank[(pk - points)/dim] = n_front;
        }
    }
    free(front_last);
#ifdef PARETO_RANK_2D_DEBUG
    {
        n_front++; // count max + 1
        int f, i;
        int *front_size = calloc(n_front, sizeof(int));
        int ** front = calloc(n_front, sizeof(int *));
        for (size_t k = 0; k < size; k++) {
            f = rank[(p[k]  - points) / dim];
            if (front_size[f] == 0) {
                front[f] = malloc(size * sizeof(int));
            }
            front[f][front_size[f]] = k;
            front_size[f]++;
        }
        int *order = malloc(size * sizeof(int));
        f = 0, k = 0, i = 0;
        do {
            order[i] = front[f][k];
            fprintf (stderr, "\n_front[%d][%d] = %d = { %g , %g, %d, %d }",
                     f, k, order[i],
                     p[order[i]][0], p[order[i]][1],
                     (p[order[i]] - points) / dim, rank[(p[order[i]] - points) / dim]);
            i++, k++;
            if (k == front_size[f]) { f++; k = 0; }
        } while (f != n_front);

        for (f = 0; f < n_front; f++)
            free(front[f]);
        free(front);
        free(front_size);

        for (k = 0; i < size; k++) {
            help_0[k] = p[order[k]][0];
            help_1[k] = p[order[k]][1];
            help_i[k] = (p[order[k]] - points) / dim;
        }
        fprintf(stderr, "%s():\n-------------------\n>>OUTPUT:", __FUNCTION__);
        fprintf(stderr, "\nIndex: "); vector_int_fprintf(stderr, help_i, size);
        fprintf(stderr, "\n[0]  : "); vector_fprintf (stderr, help_0, size);
        fprintf(stderr, "\n[1]  : "); vector_fprintf (stderr, help_1, size);

        free(order);

    }
    free(help_0);
    free(help_1);
    free(help_i);
    exit(1);
#endif

    free(p);
    return rank;
}

/* FIXME: This takes O(n^3). Look at

   M. T. Jensen. Reducing the run-time complexity of multiobjective
   EAs: The NSGA-II and other algorithms. IEEE Transactions on
   Evolutionary Computation, 7(5):503–515, 2003.
*/
static int *
pareto_rank_naive (const double * points, size_t size, dimension_t dim)
{
    int * rank = calloc(size, sizeof(*rank));
    int level = 1;
    bool something_new;
    do {
        something_new = false;
        for (size_t j = 0; j < size; j++) {
            assert(rank[j] <= level);
            /* Is already dominated or belongs to a previous front? */
            if (rank[j] != level - 1) continue;

            for (size_t k = 0; k < size; k++) {
                if (k == j) continue;
                if (rank[k] != level - 1) continue;
                const double * pj = points + j * dim;
                const double * pk = points + k * dim;
                bool j_leq_k = weakly_dominates(pj, pk, dim);
                bool k_leq_j = weakly_dominates(pk, pj, dim);
                if (j_leq_k && !k_leq_j) {
                    something_new = true;
                    rank[k]++;
                } else if (!j_leq_k && k_leq_j) {
                    something_new = true;
                    rank[j]++;
                    break;
                }
            }
        }
        level++;
    } while (something_new);
    return rank;
}

static inline void
check_pareto_rank(const int * restrict rank_true, const double * restrict points,
                  size_t size, dimension_t d)
{
    int * rank = pareto_rank_naive(points, size, d);
    for (size_t k = 0; k < size; k++) {
        if (rank[k] != rank_true[k])
            fatal_error("rank[%zu] = %d != rank_true[%zu] = %d !", k, rank[k], k, rank_true[k]);
    }
    free(rank);
}

/* Returns a 0-based rank value that indicates the level of dominance of each
   point (lower means less dominated).
*/
int *
pareto_rank(const double * restrict points, size_t size, int d)
{
    ASSUME(d >= 2 && d <= 32);
    dimension_t dim = (dimension_t) d;
    if (unlikely(size <= 0))
        return NULL;
    if (unlikely(size == 1))
        return (int *) calloc(1, sizeof(int));

    if (unlikely(dim == 3)) {
        int * rank = pareto_rank_3d(points, size);
        DEBUG1(check_pareto_rank(rank, points, size, dim));
        return rank;
    }
    if (unlikely(dim == 2)) {
        int * rank = pareto_rank_2d(points, size);
        DEBUG1(check_pareto_rank(rank, points, size, dim));
        return rank;
    }
    return pareto_rank_naive(points, size, dim);
}
