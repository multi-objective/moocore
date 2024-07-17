#include "nondominated.h"

struct point_2d_front {
    const double *p;
    unsigned int i;
    unsigned int f;
};

static int
point_2d_front_cmp (const void * a, const void * b)
{
    const double *pa = ((const struct point_2d_front *)a)->p;
    const double *pb = ((const struct point_2d_front *)b)->p;

    const double x0 = pa[0];
    const double x1 = pa[1];
    const double y0 = pb[0];
    const double y1 = pb[1];

    /* FIXME: Use ?: */
   if (x0 < y0)
        return -1;
    else if (x0 > y0)
        return 1;
    else if (x1 < y1)
        return -1;
    else if (x1 > y1)
        return 1;
    else
        return 0;
}

/*
   Nondominated sorting in 2D in O(n log n) from:

   M. T. Jensen. Reducing the run-time complexity of multiobjective
   EAs: The NSGA-II and other algorithms. IEEE Transactions on
   Evolutionary Computation, 7(5):503–515, 2003.

   FIXME: Could we avoid creating a copy of the points? Yes, see find_nondominated_set_2d_()
*/
static int *
pareto_rank_2D (const double *points, int size)
{
    const int dim = 2;
    int k;

    struct point_2d_front *data = malloc(sizeof(struct point_2d_front) * size);
    for (k = 0; k < size; k++) {
        data[k].p = &points[k * dim];
        data[k].i = (unsigned) k;
        data[k].f = 0;
    }

#if DEBUG >= 2
#define PARETO_RANK_2D_DEBUG
    double *help_0 = malloc (size * sizeof(double));
    double *help_1 = malloc (size * sizeof(double));
    int *   help_i = malloc (size * sizeof(int));

    for (k = 0; k < size; k++) {
        help_0[k] = data[k].p[0];
        help_1[k] = data[k].p[1];
        help_i[k] = data[k].i;
    }
    fprintf(stderr, "%s():\n-------------------\n>>INPUT:", __FUNCTION__);
    //fprintf(stderr, "\nIndex: "); vector_int_fprintf_fmt (stderr, help_i, size, "%4d");
    fprintf(stderr, "\n[0]  : "); vector_fprintf (stderr, help_0, size);
    fprintf(stderr, "\n[1]  : "); vector_fprintf (stderr, help_1, size);
#endif

    qsort (data, size, sizeof(struct point_2d_front), point_2d_front_cmp);

#ifdef PARETO_RANK_2D_DEBUG
    for (k = 0; i < size; k++) {
       help_0[k] = data[k].p[0];
       help_1[k] = data[k].p[1];
       help_i[k] = data[k].i;
    }
    fprintf(stderr, "%s():\n-------------------\n>>SORTED:", __FUNCTION__);
    fprintf(stderr, "\nIndex: "); vector_int_fprintf_fmt (stderr, help_i, size, "%4d");
    fprintf(stderr, "\n[0]  : "); vector_fprintf (stderr, help_0, size);
    fprintf(stderr, "\n[1]  : "); vector_fprintf (stderr, help_1, size);
#endif

    int n_front = 0;
    int * front_last = malloc (size * sizeof(int));
    front_last[0] = 0;
    data[0].f = 0; /* The first point is in the first front. */
    for (k = 1; k < size; k++) {
        const double *p = data[k].p;
        if (p[1] < data[front_last[n_front]].p[1]) {
            int low = 0;
            int high = n_front + 1;
            do {
                int mid = low + (high - low) /2;
                eaf_assert (mid <= n_front);
                const double *pmid = data[front_last[mid]].p;
                if (p[1] < pmid[1])
                    high = mid;
                else if (p[1] > pmid[1] || (p[1] == pmid[1] && p[0] > pmid[0]))
                    low = mid + 1;
                else { // Duplicated points are assigned to the same front.
                    low = mid;
                    break;
                }
            } while (low < high);
            eaf_assert (low <= n_front);
            eaf_assert (p[1] < data[front_last[low]].p[1]
                        || (p[1] == data[front_last[low]].p[1]
                            && p[0] == data[front_last[low]].p[0]));
            front_last[low] = k;
            data[k].f = low;
        } else if (p[1] == data[front_last[n_front]].p[1]
                   && p[0] == data[front_last[n_front]].p[0]) {
            front_last[n_front] = k;
            data[k].f = n_front;
        } else {
            n_front++;
            front_last[n_front] = k;
            data[k].f = n_front;
        }
    }
    free (front_last);
#ifdef PARETO_RANK_2D_DEBUG
    {
        n_front++; // count max + 1
        int f, i;
        int *front_size = calloc(nfront, sizeof(int));
        int ** front = calloc(nfront, sizeof(int *));
        for (k = 0; k < size; k++) {
            f = data[k].f;
            if (front_size[f] == 0) {
                front[f] = malloc (size * sizeof(int));
            }
            front[f][front_size[f]] = k;
            front_size[f]++;
        }
        int *order = malloc (size * sizeof(int));
        f = 0, k = 0, i = 0;
        do {
            order[i] = front[f][k];
            fprintf (stderr, "\nfront[%d][%d] = %d = { %g , %g, %d, %d }",
                     f, k, front[f][k],
                     data[front[f][k]].p[0], data[front[f][k]].p[1],
                     data[front[f][k]].i, data[front[f][k]].f);
            i++, k++;
            if (k == front_size[f]) { f++; k = 0; }
        } while (f != n_front);

        for (f = 0; f < n_front; f++)
            free(front[f]);
        free(front);
        free(front_size);

        for (k = 0; i < size; k++) {
            help_0[k] = data[order[k]].p[0];
            help_1[k] = data[order[k]].p[1];
            help_i[k] = data[order[k]].i;
        }
        fprintf(stderr, "%s():\n-------------------\n>>OUTPUT:", __FUNCTION__);
        fprintf(stderr, "\nIndex: "); vector_int_fprintf_fmt (stderr, help_i, size, "%4d");
        fprintf(stderr, "\n[0]  : "); vector_fprintf (stderr, help_0, size);
        fprintf(stderr, "\n[1]  : "); vector_fprintf (stderr, help_1, size);

        free (order);

    }
    free (help_0);
    free (help_1);
    free (help_i);
    exit(1);
#endif

    int * rank = malloc(size * sizeof(int));
    for (k = 0; k < size; k++) {
        rank[data[k].i] = data[k].f + 1;
    }
    free (data);
    return rank;
}

static bool dominates(const double *pj, const double * pk, int dim)
{
    bool j_leq_k = true;
    for (int d = 0; d < dim; d++) {
        j_leq_k = j_leq_k && (pj[d] <= pk[d]);
    }
    return j_leq_k;

}

/* FIXME: This takes O(n^3). Look at

   M. T. Jensen. Reducing the run-time complexity of multiobjective
   EAs: The NSGA-II and other algorithms. IEEE Transactions on
   Evolutionary Computation, 7(5):503–515, 2003.
*/
int *
pareto_rank (const double *points, int dim, int size)
{
    int * rank2 = NULL;
    if (dim == 2) {
        rank2 = pareto_rank_2D(points, size);
#if DEBUG == 0
        return rank2;
#endif
    }

    int * rank = malloc(size *  sizeof(int));
    for (int k = 0; k < size; k++) {
        rank[k] = 1;
    }

    int level = 2;
    bool nothing_new;
    do {
        nothing_new = true;
        for (int j = 0; j < size; j++) {
            eaf_assert(rank[j] <= level);
            /* is already dominated or belongs to a previous front? */
            if (rank[j] != level - 1) continue;

            for (int k = 0; k < size; k++) {
                if (k == j) continue;
                if (rank[k] != level - 1) continue;
                const double *pj = points + j * dim;
                const double *pk = points + k * dim;
                bool j_leq_k = dominates(pj, pk, dim);
                bool k_leq_j = dominates(pk, pj, dim);
                if (j_leq_k && !k_leq_j) {
                    nothing_new = false;
                    rank[k]++;
                } else if (!j_leq_k && k_leq_j) {
                    nothing_new = false;
                    rank[j]++;
                    break;
                }
            }
        }
        level++;
    } while (!nothing_new);

    if (rank2 != NULL) {
        for (int k = 0; k < size; k++) {
            eaf_assert(rank[k] == rank2[k]);
        }
        free(rank2);
    }
    return rank;
}
