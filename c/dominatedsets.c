/*************************************************************************

 Calculates the number of Pareto sets from one file that
 dominate the Pareto sets of the other files.

 ---------------------------------------------------------------------

                         Copyright (C) 2007-2014, 2025
          Manuel Lopez-Ibanez  <manuel.lopez-ibanez@manchester.ac.uk>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ----------------------------------------------------------------------
  IMPORTANT NOTE: Please be aware that the fact that this program is
  released as Free Software does not excuse you from scientific
  propriety, which obligates you to give appropriate credit! If you
  write a scientific paper describing research that made substantive
  use of this program, it is your obligation as a scientist to
  acknowledge its use.  Moreover, as a personal note, I would
  appreciate it if you would email manuel.lopez-ibanez@manchester.ac.uk with
  citations of papers referencing this work so I can mention them to
  my funding agent and tenure committee.
 ---------------------------------------------------------------------

 Literature:

 [1] Eckart Zitzler, Lothar Thiele, Marco Laumanns, Carlos M. Fonseca
     and Viviane Grunert da Fonseca. "Performance assessment of
     multiobjective optimizers: an analysis and review," Evolutionary
     Computation, IEEE Transactions on , vol.7, no.2, pp. 117-132,
     April 2003.

 [2] Manuel Lopez-Ibanez, Luis Paquete, and Thomas Stutzle. Hybrid
     population-based algorithms for the bi-objective quadratic
     assignment problem.  Journal of Mathematical Modelling and
     Algorithms, 5(1):111-137, April 2006.

*************************************************************************/

#include "config.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> // for strndup()
#include <unistd.h> // for getopt()
#include <getopt.h> // for getopt_long()

#include "epsilon.h"
#include "nondominated.h"

#define READ_INPUT_WRONG_INITIAL_DIM_ERRSTR "-o, --obj"
#include "cmdline.h"

static void
usage(void)
{
    printf("\n"
           "Usage: %s [OPTIONS] [FILE...]\n\n",
           program_invocation_short_name);

    printf("Calculates the number of Pareto sets from one file that            "
           "        \n"
           "dominate the Pareto sets of the other files.                       "
           "      \n\n"

           "Options:\n" OPTION_HELP_STR OPTION_VERSION_STR
           " -v, --verbose       print some information (time, number of "
           "points, etc.) \n" OPTION_QUIET_STR
           " -p, --percentages   print results also as percentages.            "
           "        \n"
           "     --no-check      do not check nondominance of sets (faster but "
           "unsafe).\n" OPTION_OBJ_STR "\n");
}


static bool verbose_flag = false;
static bool percentages_flag = false;
static bool check_flag = true;

// strnlen() is not available in C99.
static inline size_t
x_strnlen(const char * s, size_t maxlen)
{
    size_t len = 0;
    while (len < maxlen && s[len] != '\0')
        len++;
    return len;
}

// strndup() is not available in C99.
static char *
x_strndup(const char * s, size_t n)
{
    size_t len = x_strnlen(s, n); // Find length up to n or end of string
    char * new_str =
        malloc(len + 1); // Allocate memory (+1 for null terminator)
    if (unlikely(new_str == NULL))
        return NULL;         // malloc failed
    memcpy(new_str, s, len); // Copy the string up to len
    new_str[len] = '\0';     // Null-terminate
    return new_str;
}

void
print_results(char ** filenames, int numfiles, int * nruns, int ** results)
{
    int k, j;
    int max_col_len = 0;
    int max_filename_len = 0;
    int max_result = 0;
    char buffer[32];

    /* longest filename.  */
    for (k = 0; k < numfiles; k++)
        max_filename_len = MAX(max_filename_len, (int)strlen(filenames[k]));

    /* longest number.  */
    for (k = 0; k < numfiles; k++)
        for (j = 0; j < numfiles; j++)
            max_result = MAX(max_result, results[k][j]);
    snprintf(buffer, 32, "%d", max_result);
    buffer[31] = '\0';
    max_col_len = MAX(max_filename_len, (int)strlen(buffer));

    printf("\n\n"
           "Number of times that <row> is better than <column>:\n");

    /* Header row.  */
    printf("\n%*s", max_filename_len, "");
    for (k = 0; k < numfiles; k++) {
        printf(" %*s", max_col_len, filenames[k]);
    }

    for (k = 0; k < numfiles; k++) {
        printf("\n%*s", max_filename_len, filenames[k]);
        for (j = 0; j < numfiles; j++) {
            if (k == j)
                printf(" %*s", max_col_len, "--");
            else
                printf(" %*d", max_col_len, results[k][j]);
        }
    }

    printf("\n");

    if (percentages_flag) {
        max_col_len = MAX(max_col_len, (int)strlen("100.0"));

        printf("\n\n"
               "Percentage of times that <row> is better than <column>:\n");

        /* Header row.  */
        printf("\n%*s", max_filename_len, "");
        for (k = 0; k < numfiles; k++) {
            printf(" %*s", max_col_len, filenames[k]);
        }

        for (k = 0; k < numfiles; k++) {
            printf("\n%*s", max_filename_len, filenames[k]);
            for (j = 0; j < numfiles; j++) {
                if (k == j)
                    printf(" %*s", max_col_len, "--");
                else
                    printf(" %*.1f", max_col_len,
                           results[k][j] * 100.0 / (nruns[k] * nruns[j]));
            }
        }
    }
    printf("\n\n");

    printf("Ranks:");
    for (k = 0; k < numfiles; k++) {
        int rank = 0;
        for (j = 0; j < numfiles; j++) {
            if (k == j)
                continue;
            rank += results[j][k];
        }
        printf(" %3d", rank);
    }
    printf("\n");
}

static inline int
dominance(int dim, const double * a, const double * b,
          const signed char * minmax)
/***********************************************
 return: O  -> if a == b
         1  -> if a dominates b
         -1 -> if a NOT dominates b

***********************************************/
{
    int d;

    /* If any objective is worse, A can't dominate B. */
    for (d = 0; d < dim; d++)
        if ((minmax[d] < 0 && a[d] > b[d]) || (minmax[d] > 0 && a[d] < b[d]))
            return -1;

    /* If any objective is better, then A dominates B. */
    for (d = 0; d < dim; d++)
        if ((minmax[d] < 0 && a[d] < b[d]) || (minmax[d] > 0 && a[d] > b[d]))
            return 1;

    return 0;
}

static int
set_dominates(int dim, const signed char * minmax, const double * points_x,
              int size_x, const double * points_y, int size_y)
{
    bool x_dominates_y = false, x_weakly_dominates_y = false;

    for (int y = 0; y < size_y; y++) {
        x_weakly_dominates_y = false;
        for (int x = 0; x < size_x; x++) {
            DEBUG2(printf("X:"); vector_printf(points_x + x * dim, dim);
                   printf("Y:"); vector_printf(points_y + y * dim, dim););
            int result =
                dominance(dim, &points_x[x * dim], &points_y[y * dim], minmax);
            if (result == 1) {
                DEBUG2(printf("X dominates Y!\n"));
                x_weakly_dominates_y = true;
                x_dominates_y = true;
                break;

            } else if (result == 0) {
                DEBUG2(printf("X weakly dominates Y!\n"));
                x_weakly_dominates_y = true;
                break;
            }
        }
        if (!x_weakly_dominates_y)
            break;
    }

    if (!x_weakly_dominates_y)
        return 1;
    else if (size_x != size_y || x_dominates_y)
        return -1;
    else
        return 0;
}

int
pareto_better(int dim, const signed char * minmax, const double * points_a,
              int size_a, const double * points_b, int size_b)
{
    int result = set_dominates(dim, minmax, points_a, size_a, points_b, size_b);
    if (result == 1) {
        DEBUG2(printf("Trying with B\n"));
        result = set_dominates(dim, minmax, points_b, size_b, points_a, size_a);
        result = -result;
        if (result != 1) {
            DEBUG2(printf("A || B\n"));
            result = 0;
        }
    }

    int result2 = epsilon_additive_ind((dimension_t)dim, minmax, points_a,
                                       size_a, points_b, size_b);

    DEBUG2(
        printf("result = %d, result2 = %d\n", result, result2);
        for (int a = 0; a < size_a;
             a++) { vector_printf(points_a + a * dim, dim); } printf("\n\n");
        for (int b = 0; b < size_b;
             b++) { vector_printf(points_b + b * dim, dim); });
    if (result != result2) {
        printf("result = %d  !=  result2 = %d\n", result, result2);
        abort();
    }
    return result;
}

void
cmpparetos(int dim, const signed char * minmax, const double * points_a,
           int nruns_a, const int * cumsizes_a, int * numbetter_a,
           const double * points_b, int nruns_b, const int * cumsizes_b,
           int * numbetter_b)
{
    *numbetter_a = 0;
    *numbetter_b = 0;

    for (int a = 0, size_a = 0; a < nruns_a; a++) {
        for (int b = 0, size_b = 0; b < nruns_b; b++) {
            int result = pareto_better(
                dim, minmax, points_a + (dim * size_a), cumsizes_a[a] - size_a,
                points_b + (dim * size_b), cumsizes_b[b] - size_b);
            if (result < 0)
                (*numbetter_a)++;
            else if (result > 0)
                (*numbetter_b)++;

            size_b = cumsizes_b[b];
        }
        size_a = cumsizes_a[a];
    }
}

int
main(int argc, char * argv[])
{
    int * nruns = NULL;
    int ** cumsizes = NULL;
    double ** points = NULL;
    int dim = 0;
    char ** filenames;
    int numfiles;
    const signed char * minmax = NULL;

    int k, n, j;
    /* see the man page for getopt_long for an explanation of these fields */
    static const char short_options[] = "hVvqpo:";
    static const struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'V'},
        {"verbose", no_argument, NULL, 'v'},
        {"quiet", no_argument, NULL, 'q'},
        {"percentages", no_argument, NULL, 'p'},
        {"no-check", no_argument, NULL, 'c'},
        {"obj", required_argument, NULL, 'o'},
        {NULL, 0, NULL, 0} /* marks end of list */
    };
    set_program_invocation_short_name(argv[0]);

    int opt; /* it's actually going to hold a char */
    int longopt_index;
    while (0 < (opt = getopt_long(argc, argv, short_options, long_options,
                                  &longopt_index))) {
        switch (opt) {
        case 'q': // --quiet
            verbose_flag = false;
            break;

        case 'v': // --verbose
            verbose_flag = true;
            break;

        case 'p': // --percentages
            percentages_flag = true;
            break;

        case 'c': // --no-check
            check_flag = false;
            break;

        case 'o': // --obj
            minmax = parse_cmdline_minmax(minmax, optarg, &dim);
            break;

        default:
            default_cmdline_handler(opt);
        }
    }

    numfiles = argc - optind;

    if (numfiles <= 1) {
        fprintf(stderr, "%s: error: at least two input files are required.\n",
                program_invocation_short_name);
        usage();
        exit(EXIT_FAILURE);
    }

    filenames = malloc(sizeof(char *) * numfiles);
    points = malloc(sizeof(double *) * numfiles);
    nruns = malloc(sizeof(int) * numfiles);
    cumsizes = malloc(sizeof(int *) * numfiles);

    for (k = 0; k < numfiles; optind++, k++) {
        filenames[k] = argv[optind];
        points[k] = NULL;
        cumsizes[k] = NULL;
        nruns[k] = 0;
        handle_read_data_error(read_double_data(filenames[k], &points[k], &dim,
                                                &cumsizes[k], &nruns[k]),
                               filenames[k]);
    }

    /* Default minmax if not set yet.  */
    if (minmax == NULL)
        minmax = minmax_minimise((dimension_t)dim);

    /* Print filename substitutions.  */
    for (k = 0; k < numfiles; k++) {
        char buffer[32];
        snprintf(buffer, 31, "f%d", k + 1);
        buffer[31] = '\0';
        char * p = x_strndup(buffer, 31);
        if (unlikely(!p))
            fatal_error("%s:%d: malloc failed", __FILE__, __LINE__);
        printf("# %s: %s\n", p, filenames[k]);
        filenames[k] = p;
    }
    printf("\n");

    /* Print some info about input files.  */
    for (k = 0; k < numfiles; k++) {
        printf("# %s: %d (%d", filenames[k], nruns[k], cumsizes[k][0]);
        for (n = 1; n < nruns[k]; n++)
            printf(", %d", cumsizes[k][n]);
        printf(")\n");
    }

    /* Print some info.  */
    printf("# objectives (%d): ", dim);
    for (k = 0; k < dim; k++) {
        printf("%c", (minmax[k] < 0) ? '-' : (minmax[k] > 0) ? '+' : 'i');
    }
    printf("\n");

    if (check_flag) {
        bool check_failed = false;

        for (k = 0; k < numfiles; k++) {
            int size = 0;
            for (n = 0; n < nruns[k]; n++) {
                size_t failed_pos = find_dominated_point(
                    &points[k][dim * size], dim, cumsizes[k][n] - size, minmax);
                if (failed_pos < SIZE_MAX) {
                    fprintf(stderr, "%s: %s: set %d: point %zu is dominated.\n",
                            program_invocation_short_name, filenames[k], n,
                            failed_pos);
                    check_failed = true;
                }
                size = cumsizes[k][n];
            }
        }
        if (check_failed) {
            errprintf("input must be a collection of nondominated sets.");
            exit(EXIT_FAILURE);
        }
    }
    int ** results =
        malloc(sizeof(int) * numfiles * numfiles + sizeof(int *) * numfiles);

    for (k = 0; k < numfiles; k++) {
        results[k] = (int *)(results + numfiles) + k * numfiles;
        for (j = 0; j < numfiles; j++)
            results[k][j] = -1;
    }

    for (k = 0; k < numfiles; k++)
        for (j = k + 1; j < numfiles; j++)
            cmpparetos(dim, minmax, points[k], nruns[k], cumsizes[k],
                       &(results[k][j]), points[j], nruns[j], cumsizes[j],
                       &(results[j][k]));

    print_results(filenames, numfiles, nruns, results);

    if (verbose_flag) {
    }

    return EXIT_SUCCESS;
}
