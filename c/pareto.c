#include "nondominated.h"

#if DEBUG >= 2
#include "io.h"
#endif

/**
   Nondominated sorting in 3D in O(k * n log n), where k is the number of fronts.

   Uses the same algorithm as find_nondominated_set_3d_helper().
*/

static void
pareto_rank_3d(int * restrict rank, const double * restrict points, size_t size)
{
    ASSUME(size >= 2);
    memset(rank, 0, sizeof(*rank) * size);

    const size_t orig_size = size;
    const bool keep_weakly = true;
    const double ** p = generate_row_pointers_asc_rev_3d(points, size);

    avl_tree_t tree;
    avl_init_tree(&tree, qsort_cmp_pdouble_asc_x_nonzero);
    avl_node_t * tnodes = malloc((size+1) * sizeof(*tnodes));
    const double sentinel[] = { INFINITY, -INFINITY };
    tnodes->item = sentinel;
    int front = 0;

    while (true) {
        ASSUME(size >= 2);
        const double * restrict pk = p[0];
        avl_node_t * node = tnodes + 1;
        node->item = pk;
        avl_insert_top(&tree, node);
        // Insert sentinel.
        avl_insert_after(&tree, node, tnodes);

        // In this context, size means "no dominated solution found".
        size_t n_nondom = size, j = 1;
        const double * last_dom = NULL;
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
                        || last_dom == pk;
                }
            }
            if (dominated) { // pj is dominated by a point in the tree or by prev.
                /* Map the order in p[], which is sorted, to the original order in
                   points. */
                size_t pos_last_dom = row_index_from_ptr(points, pj, 3);
                assert(rank[pos_last_dom] == front);
                rank[pos_last_dom] = front + 1;
                last_dom = pj;
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
            return;
        }

        assert(rank[row_index_from_ptr(points, p[0], 3)] == front);
        size_t k = 0, n = 0;
        assert(k < size);
        do {
            do {
                n++;
            } while (rank[row_index_from_ptr(points, p[n], 3)] == front);
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
static void
pareto_rank_2d(int * restrict rank, const double * restrict points, size_t size)
{
    const dimension_t dim = 2;
    const double ** p = generate_row_pointers(points, size, dim);

#if DEBUG >= 2
#define PARETO_RANK_2D_DEBUG
    double * help_0 = malloc(size * sizeof(*help_0));
    double * help_1 = malloc(size * sizeof(*help_1));
    int * help_i = malloc(size * sizeof(*help_i));

    for (size_t k = 0; k < size; k++) {
        help_0[k] = p[k][0];
        help_1[k] = p[k][1];
        help_i[k] = (int) k;
    }
    fprintf(stderr, "%s():\n-------------------\n>>INPUT:", __FUNCTION__);
    fprintf(stderr, "\nIndex: "); vector_int_fprintf(stderr, help_i, size);
    fprintf(stderr, "\n[0]  : "); vector_fprintf (stderr, help_0, size);
    fprintf(stderr, "\n[1]  : "); vector_fprintf (stderr, help_1, size);
#endif

    // Sort in ascending lexicographic order from the second dimension.
    qsort_typesafe(p, size, cmp_ppdouble_asc_rev_2d);

#ifdef PARETO_RANK_2D_DEBUG
    for (size_t k = 0; k < size; k++) {
       help_0[k] = p[k][0];
       help_1[k] = p[k][1];
       help_i[k] = row_index_from_ptr(points, p[k], dim);
    }
    fprintf(stderr, "%s():\n-------------------\n>>SORTED:", __FUNCTION__);
    fprintf(stderr, "\nIndex: "); vector_int_fprintf(stderr, help_i, size);
    fprintf(stderr, "\n[0]  : "); vector_fprintf (stderr, help_0, size);
    fprintf(stderr, "\n[1]  : "); vector_fprintf (stderr, help_1, size);
#endif

    double * front_last = malloc(size * sizeof(*front_last));
    int n_front = 0;
    front_last[0] = p[0][0];
    rank[row_index_from_ptr(points, p[0], dim)] = 0; // The first point is in the first front.
    int last_rank = 0;
    for (size_t k = 1; k < size; k++) {
        const double * restrict pk = p[k];
        // Duplicated points are kept in the same front.
        if (pk[0] == p[k-1][0] && pk[1] == p[k-1][1]) {
            rank[row_index_from_ptr(points, pk, dim)] = last_rank;
            continue;
        }
        const double plast = front_last[n_front];
        if (pk[0] < plast) {
            int low = 0;
            if (n_front > 0) {
                int high = n_front + 1;
                do { // Binary search.
                    int mid = low + (high - low) / 2;
                    assert(mid <= n_front);
                    const double pmid = front_last[mid];
                    // FIXME: When mid == n_front, then pk[0] < pmid, so avoid this test.
                    if (pk[0] < pmid)
                        high = mid;
                    else {
                        low = mid + 1;
                    }
                } while (low < high);
            }
            assert(low <= n_front);
            assert(pk[0] < front_last[low]);
            last_rank = low;
        } else {
            n_front++;
            last_rank = n_front;
        }
        front_last[last_rank] = pk[0];
        rank[row_index_from_ptr(points, pk, dim)] = last_rank;
    }
    free(front_last);
#ifdef PARETO_RANK_2D_DEBUG
    {
        n_front++; // count max + 1
        size_t f, i, k;
        int *front_size = calloc(n_front, sizeof(*front_size));
        int ** front = calloc(n_front, sizeof(*front));
        for (k = 0; k < size; k++) {
            f = rank[row_index_from_ptr(points, p[k], dim)];
            if (front_size[f] == 0) {
                front[f] = malloc(size * sizeof(**front));
            }
            front[f][front_size[f]] = k;
            front_size[f]++;
        }
        int * order = malloc(size * sizeof(*order));
        f = 0, k = 0, i = 0;
        do {
            order[i] = front[f][k];
            fprintf (stderr, "\n_front[%zu][%zu] = %d = { %g , %g, %zu, %d }",
                     f, k, order[i],
                     p[order[i]][0], p[order[i]][1],
                     row_index_from_ptr(points, p[order[i]], dim),
                     rank[row_index_from_ptr(points, p[order[i]], dim)]);
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
            help_i[k] = row_index_from_ptr(points, p[order[k]], dim);
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
}

/**
   Brute-force but using find_nondominated_set_kung().

   This takes O(n^2 log^(d-2)(n)). A better algorithm is given by:

   M. T. Jensen. Reducing the run-time complexity of multiobjective EAs: The
   NSGA-II and other algorithms. IEEE Transactions on Evolutionary Computation,
   7(5):503–515, 2003.
*/
static void
pareto_rank_kung(int * restrict rank,
                 const double * restrict points, size_t size, dimension_t dim)
{
    ASSUME(dim >= 2);
    ASSUME(size >= 2);
    memset(rank, 0, sizeof(*rank) * size);

    const bool keep_weakly = true;
    const double ** rows = malloc(2 * size * sizeof(*rows));
    const double ** prev_rows = rows + size;
    for (size_t k = 0; k < size; k++)
        prev_rows[k] = points + dim * k;
    // Same as find_nondominated_set_kung()
    qsort_typesafe(prev_rows, size, cmp_ppdouble_asc_x_nonzero_stable);

    int front = 1;
    while (true) {
        ASSUME(size >= 2);
        for (size_t k = 0; k < size; k++)
            rows[k] = prev_rows[k];
        size_t new_size = maxima_rec(rows, size, dim, keep_weakly);
        // If everything is nondominated or there is only one dominated point,
        // we can stop.
        size -= new_size;
        // size is now the number of dominated points.
        if (size <= 2)
            break;
        // If something is in rows[], it is nondominated -> remove it!
        size_t j = 0;
        while (prev_rows[j] == rows[j]) j++;
        // j was dominated, so it goes in the next front.
        prev_rows[0] = prev_rows[j];
        rank[row_index_from_ptr(points, prev_rows[j], dim)] = front;
        size_t n = 1;
        size_t i = j + 1;
        do  {
            while (prev_rows[i] == rows[j]) {
                i++, j++;
            }
            prev_rows[n] = prev_rows[i];
            rank[row_index_from_ptr(points, prev_rows[i], dim)] = front;
            n++, i++;
        } while (n < size);
        front++;
    }

    if (size != 0) {
        size_t n = 0;
        while (prev_rows[n] == rows[n])
            n++;
        const double * first = prev_rows[n];
        if (size == 1) {
            // Update the only dominated point.
            rank[row_index_from_ptr(points, first, dim)] = front;
        } else {
            assert(size == 2);
            while (prev_rows[n+1] == rows[n])
                n++;
            const double * second = prev_rows[n+1];
            dominance_cmp_t res = vec_cmp_dominance(first, second, dim, keep_weakly);
            if (res == VEC_INCOMPARABLE) {
                rank[row_index_from_ptr(points, first, dim)] = front;
                rank[row_index_from_ptr(points, second, dim)] = front;
            }  else if (res == VEC_A_LT_B) {
                // first[i] <= second[i] for all i, and first[i] < second[i] for at least one i.
                rank[row_index_from_ptr(points, first, dim)] = front;
                rank[row_index_from_ptr(points, second, dim)] = front+1;
            }  else {
                assert(res == VEC_A_GT_B);
                // second[i] <= first[i] for all i, and second[i] < first[i] for at least one i.
                rank[row_index_from_ptr(points, first, dim)] = front+1;
                rank[row_index_from_ptr(points, second, dim)] = front;
            }
        }
    }
    free(rows);
}

/**
   Similar to find_nondominated_bf_impl().

   This takes O(n^3). A better algorithm is given by:

   M. T. Jensen. Reducing the run-time complexity of multiobjective EAs: The
   NSGA-II and other algorithms. IEEE Transactions on Evolutionary Computation,
   7(5):503–515, 2003.
*/
_attr_maybe_unused
static inline void
pareto_rank_brute_force(int * restrict rank,
                        const double * restrict points, size_t size, dimension_t dim)
{
    ASSUME(dim >= 2);
    ASSUME(size >= 2);
    memset(rank, 0, sizeof(*rank) * size);

    const bool keep_weakly = true;
    const double ** rows = malloc(2 * size * sizeof(*rows));
    const double ** new_rows = rows + size;
    for (size_t k = 0; k < size; k++)
        rows[k] = points + dim * k;

    int front = 1;
    while (true) {
        ASSUME(size >= 2);
        size_t new_size = 0;
        size_t min_k = 0;
        for (size_t j = 1; j < size; j++) {
            size_t k = min_k;
            while (rows[k] == NULL)
                k++;
            min_k = k;
            const double * restrict pj = rows[j];
            ASSUME(pj != NULL);
            ASSUME(rows[k] != NULL);
            ASSUME(k < j);
            do {
                const double * restrict pk = rows[k];
                dominance_cmp_t res = vec_cmp_dominance(pk, pj, dim, keep_weakly);
                if (res == VEC_A_LT_B) {
                    // pk[i] <= pj[i] for all i, and pk[i] < pj[i] for at least one i.
                    assert(rows[j]);
                    new_rows[new_size++] = rows[j];
                    rows[j] = NULL;
                    break;
                } else if (res == VEC_A_GT_B) {
                    // pj[i] <= pk[i] for all i, and pj[i] < pk[i] for at least one i.
                    assert(rows[k]);
                    new_rows[new_size++] = rows[k];
                    rows[k] = NULL;
                }
                do {
                    k++;
                } while (k < j && rows[k] == NULL);
            } while (k < j);
        }
        size = new_size;
        if (size <= 2)
            break;
        for (size_t k = 0; k < size; k++)
            rank[row_index_from_ptr(points, new_rows[k], dim)] = front;
        SWAP(rows, new_rows);
        front++;
    }

    if (size == 1) {
        // Update the only dominated point.
        rank[row_index_from_ptr(points, new_rows[0], dim)] = front;
    } else if (size == 2) {
        const double * row0 = new_rows[0];
        const double * row1 = new_rows[1];
        dominance_cmp_t res = vec_cmp_dominance(row0, row1, dim, keep_weakly);
        if (res == VEC_INCOMPARABLE) {
            rank[row_index_from_ptr(points, row0, dim)] = front;
            rank[row_index_from_ptr(points, row1, dim)] = front;
        }  else if (res == VEC_A_LT_B) {
            // row0[i] <= row1[i] for all i, and row0[i] < row1[i] for at least one i.
            rank[row_index_from_ptr(points, row0, dim)] = front;
            rank[row_index_from_ptr(points, row1, dim)] = front+1;
        }  else {
            assert(res == VEC_A_GT_B);
            // row1[i] <= row0[i] for all i, and row1[i] < row0[i] for at least one i.
            rank[row_index_from_ptr(points, row0, dim)] = front+1;
            rank[row_index_from_ptr(points, row1, dim)] = front;
        }
    }
    free(rows);
}

/**
   This is a different implementation of pareto_rank_brute_force() for
   validation.
*/
static void
pareto_rank_naive(int * restrict rank,
                  const double * restrict points, size_t size, dimension_t dim)
{
    ASSUME(dim >= 2);
    ASSUME(size >= 2);
    memset(rank, 0, sizeof(*rank) * size);

    const size_t orig_size = size;
    const bool keep_weakly = true;
    bool * dominated = calloc(size, sizeof(*dominated));
    const double ** p = generate_row_pointers(points, size, dim);
    int front = 1;

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
                rank[row_index_from_ptr(points, p[last_dom_pos], dim)] = front;
            }
            free(dominated);
            free(p);
            return;
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
            rank[row_index_from_ptr(points, p[k], dim)] = front;
        for (k = 0; k < size; k++)
            dominated[k] = false;
        front++;
    }
}


static inline void
check_pareto_rank(const int * restrict rank_true, const double * restrict points,
                  size_t size, dimension_t d)
{
    int * rank = malloc(size * sizeof(*rank));
    pareto_rank_naive(rank, points, size, d);
    for (size_t k = 0; k < size; k++) {
        if (rank[k] != rank_true[k])
            fatal_error(__FILE__ ":%u: rank[%zu]=%d != rank_true[%zu]=%d !",
                        __LINE__, k, rank[k], k, rank_true[k]);
    }
    free(rank);
}

/**
   Returns in rank argument a 0-based rank value that indicates the level of
   dominance of each point (lower means less dominated).
*/
void
pareto_rank(int * restrict rank,
            const double * restrict points, size_t size, dimension_t dim)
{
    if (unlikely(size == 0))
        return;
    if (unlikely(size == 1)) {
        rank[0] = 0;
        return;
    }

    if (likely(dim > 3)) {
        pareto_rank_kung(rank, points, size, dim);
    } else if (dim == 3) {
        pareto_rank_3d(rank, points, size);
    } else if (dim == 2) {
        pareto_rank_2d(rank, points, size);
    } else {
        // FIXME: Handle dim=1 like python does.
        // FIXME: How to handle dim=0? Python returns a vector of zeros.
        return;
    }
    DEBUG1(check_pareto_rank(rank, points, size, dim));
}
