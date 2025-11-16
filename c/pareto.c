#include "nondominated.h"

#if DEBUG >= 2
#include "io.h"
#endif

/**
   Nondominated sorting in 3D in O(k * n log n), where k is the number of fronts.

   Uses the same algorithm as find_nondominated_set_3d_helper().
*/

static int *
pareto_rank_3d(const double * restrict points, size_t size)
{
    ASSUME(size >= 2);
    const size_t orig_size = size;
    const bool keep_weakly = true;

    int * rank = calloc(size, sizeof(*rank));
    int front = 0;

    const double **p = generate_sorted_pp_3d(points, size);

    avl_tree_t tree;
    avl_init_tree(&tree, cmp_pdouble_asc_x_nonzero);
    avl_node_t * tnodes = malloc((size+1) * sizeof(*tnodes));
    const double sentinel[] = { INFINITY, -INFINITY };
    tnodes->item = sentinel;

    while (true) {
        ASSUME(size >= 2);
        const double * restrict pk = p[0];
        avl_node_t * node = tnodes + 1;
        node->item = pk;
        avl_insert_top(&tree, node);
        // Insert sentinel.
        avl_insert_after(&tree, node, tnodes);

        // In this context, orig_size means "no dominated solution found".
        size_t pos_last_dom = orig_size, n_nondom = size, j = 1;
        do {
            const double * restrict pj = p[j];
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
                    nodeaux = nodeaux->next;
                } else if (nodeaux->prev) {// nodeaux goes after pj, so move to the next one.
                    const double * restrict prev = nodeaux->prev->item;
                    assert(prev[0] != sentinel[0]);
                    assert(prev[0] <= pj[0]);
                    dominated = prev[1] <= pj[1];
                } else {
                    dominated = false;
                }

                if (!dominated) { // pj is NOT dominated by a point in the tree.
                    const double * restrict point = nodeaux->item;
                    assert(pj[0] <= point[0]);
                    // Delete everything in the tree that is dominated by pj.
                    while (pj[1] <= point[1]) {
                        assert(pj[0] <= point[0]);
                        nodeaux = nodeaux->next;
                        point = nodeaux->item;
                        /* FIXME: A possible speed up is to delete without
                           rebalancing the tree because avl_insert_before() will
                           rebalance. */
                        avl_unlink_node(&tree, nodeaux->prev);
                    }
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
                    dominated = !k_eq_j
                        // or pk was dominated, then this one is also dominated.
                        || pos_last_dom == (size_t) ((pk - points) / 3);
                }
            }
            if (dominated) { // pj is dominated by a point in the tree or by prev.
                /* Map the order in p[], which is sorted, to the original order in
                   points. */
                pos_last_dom = (pj - points) / 3;
                assert(rank[pos_last_dom] == front);
                rank[pos_last_dom] = front + 1;
                n_nondom--;
            } else {
                pk = pj;
            }
            j++;
        } while (j < size);

        for (size_t k = 0; k < orig_size; k++)
            DEBUG2_PRINT("rank[%zu] = %d\n", k, rank[k]);

        // If everything is nondominated or there is only one dominated point,
        // we can stop.
        size -= n_nondom;
        if (size <= 1) {
            free(tnodes);
            free(p);
            return rank;
        }

        assert(rank[(p[0] - points) / 3] == front);
        size_t k = 0, n = 0;
        assert(k < size);
        do {
            do {
                n++;
            } while (rank[(p[n] - points) / 3] == front);
            p[k] = p[n];
            k++;
        } while (k < size);

        for (size_t k = 0; k < size; k++)
            DEBUG2_PRINT("p[%zu] = %.16g %.16g %.16g\n",
                         k, p[k][0], p[k][1], p[k][2]);

        front++;
        avl_clear_tree(&tree);
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
        size_t f, i, k;
        int *front_size = calloc(n_front, sizeof(int));
        int ** front = calloc(n_front, sizeof(int *));
        for (k = 0; k < size; k++) {
            f = rank[(p[k]  - points) / dim];
            if (front_size[f] == 0) {
                front[f] = malloc(size * sizeof(int));
            }
            front[f][front_size[f]] = k;
            front_size[f]++;
        }
        int * order = malloc(size * sizeof(*order));
        f = 0, k = 0, i = 0;
        do {
            order[i] = front[f][k];
            fprintf (stderr, "\n_front[%zu][%zu] = %d = { %g , %g, %d, %d }",
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
pareto_rank_naive(const double * restrict points, size_t size, dimension_t dim)
{
    ASSUME(dim >= 2);
    ASSUME(size >= 2);
    const size_t orig_size = size;
    const bool keep_weakly = true;
    int * rank = calloc(size, sizeof(*rank));
    int front = 1;

    bool * dominated = calloc(size, sizeof(*dominated));
    const double ** p = malloc(size * sizeof(*p));
    for (size_t k = 0; k < size; k++)
        p[k] = points + dim * k;

    while (true) {
        ASSUME(size >= 2);
        size_t new_size = size;
        size_t min_k = 0, last_dom_pos = orig_size;
        for (size_t j = 1; j < size; j++) {
            const double * restrict pj = p[j];
            size_t k = min_k;
            assert(!dominated[j]);
            while (dominated[k])
                k++;
            min_k = k;
            ASSUME(k < j);
            ASSUME(!dominated[k]);
            for (; k < j; k++) {
                assert(!dominated[j]);
                if (dominated[k]) continue;

                const double * restrict pk = p[k];
                // Use unsigned instead of bool to allow auto-vectorization.
                unsigned k_leq_j = true, j_leq_k = true;
                for (dimension_t d = 0; d < dim; d++) {
                    double cmp = pj[d] - pk[d];
                    k_leq_j &= (cmp >= 0.0);
                    j_leq_k &= (cmp <= 0.0);
                }
                // k is removed if it is dominated by j.
                unsigned dom_k = (!k_leq_j) & j_leq_k;
                // j is removed if it is weakly dominated by k (unless keep_weakly).
                // Only the first duplicated point is kept.
                unsigned dom_j = !keep_weakly ? k_leq_j : (k_leq_j & (!j_leq_k));

                /* As soon as j_leq_k and k_leq_j become false, neither k or j will
                   be removed, so skip the rest.  */
                if (!(dom_k | dom_j)) continue;

                assert(dom_k ^ dom_j); // At least one but not both can be removed.
                new_size--; // Something dominated.

                last_dom_pos = (dom_j) ? j : k;
                assert(!dominated[last_dom_pos]);
                dominated[last_dom_pos] = true;
                if (dom_j) break;
            }
        }
        // If everything is nondominated or there is only one dominated point,
        // we can stop.
        size -= new_size;
        if (size <= 1) {
            if (size == 1) {
                assert(last_dom_pos < orig_size);
                // Update the only dominated point.
                rank[(p[last_dom_pos] - points) / dim] = front;
            }
            free(dominated);
            free(p);
            return rank;
        }
        size_t k = 0;
        while (dominated[k]) k++; // Find first nondominated
        for (size_t n = k; k < size; k++) {
            do { // Find next dominated
                n++;
            } while (!dominated[n]);
            p[k] = p[n];
        }
        // Update ranks. Everything still in list p goes to the next front.
        for (k = 0; k < size; k++)
            rank[(p[k] - points) / dim] = front;
        for (k = 0; k < size; k++)
            dominated[k] = false;
        front++;
    }
}


static inline void
check_pareto_rank(const int * restrict rank_true, const double * restrict points,
                  size_t size, dimension_t d)
{
    int * rank = pareto_rank_naive(points, size, d);
    for (size_t k = 0; k < size; k++) {
        if (rank[k] != rank_true[k])
            fatal_error(__FILE__ ":%u: rank[%zu]=%d != rank_true[%zu]=%d !",
                        __LINE__, k, rank[k], k, rank_true[k]);
    }
    free(rank);
}

/**
   Returns a 0-based rank value that indicates the level of dominance of each
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

    if (likely(dim > 3))
        return pareto_rank_naive(points, size, dim);

    int * rank;
    if (dim == 3) {
        rank = pareto_rank_3d(points, size);
    } else {
        assert(dim == 2);
        rank = pareto_rank_2d(points, size);
    }
    DEBUG1(check_pareto_rank(rank, points, size, dim));
    return rank;
}
