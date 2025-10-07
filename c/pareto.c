#include "nondominated.h"

/*
   Nondominated sorting in 2D in O(n log n) from:

   M. T. Jensen. Reducing the run-time complexity of multiobjective
   EAs: The NSGA-II and other algorithms. IEEE Transactions on
   Evolutionary Computation, 7(5):503–515, 2003.
*/
#if DEBUG >= 2
#include "io.h"
#endif
static int *
pareto_rank_2D (const double * points, size_t size)
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
    if (size <= 0)
        return NULL;
    if (dim == 2) {
        int * rank = pareto_rank_2D(points, size);
        DEBUG1(check_pareto_rank(rank, points, size, 2));
        return rank;
    }

    return pareto_rank_naive(points, size, dim);
}
