/*************************************************************************

 ndsort: Perform nondominated sorting in a list of points.

 ---------------------------------------------------------------------
                       Copyright (C) 2011, 2025
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
  my funding body and tenure committee.
 ---------------------------------------------------------------------

 Literature:

*************************************************************************/
#include "config.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>  // for getopt()
#include <getopt.h> // for getopt_long()
#include <math.h>  // for INFINITY

#include "common.h"
#include "hv.h"
#include "nondominated.h" // for normalise()

#define READ_INPUT_WRONG_INITIAL_DIM_ERRSTR "-o, --obj"
#include "cmdline.h"

static void usage(void)
{
    printf("\n"
           "Usage: %s [OPTIONS] [FILE...]\n\n", program_invocation_short_name);

    printf(
"Perform nondominated sorting in a list of points.                        \n\n"

"Options:\n"
OPTION_HELP_STR
OPTION_VERSION_STR
" -v, --verbose       print some information (time, number of points, etc.) \n"
OPTION_QUIET_STR
//" -H, --hypervolume   use hypervolume contribution to break ties            \n"
" -k, --keep-uevs     keep uniquely extreme values                          \n"
" -r, --rank          don't break ties using hypervolume contribution       \n"
OPTION_OBJ_STR
"\n");
}

static bool verbose_flag = false;

static void
fprint_rank (FILE * stream, const int * rank, int size)
{
    int k;
    for (k = 0; k < size; k++) {
        fprintf (stream, "%d\n", rank[k]);
    }
}

static void fprint_vector_double (FILE * stream, const double * vec, int size)
{
    for (int k = 0; k < size; k++)
        fprintf (stream, "%g\n", vec[k]);
}

static bool *
calculate_uev (bool *uev, const double *points, int dim, int size,
               const double *lbound, const double *ubound)
{
    if (uev == NULL) {
        uev = malloc (sizeof(bool) * size);
    }

    for (int j = 0; j < size; j++)
        uev[j] = false;

    for (int i = 0; i < dim; i++) {
        assert (ubound[i] > -INFINITY);
        assert (lbound[i] < INFINITY);
        for (int j = 0; j < size; j++) {
            if (points[j * dim + i] == ubound[i]) {
                uev[j] = true;
                break;
            }
        }
        for (int j = 0; j < size; j++) {
            if (points[j * dim + i] == lbound[i]) {
                uev[j] = true;
                break;
            }
        }
    }
    return uev;
}

int main(int argc, char *argv[])
{
    // See the man page for getopt_long for an explanation of these fields.
    static const char short_options[] = "hVvqkro:";
    static const struct option long_options[] = {
        {"help",       no_argument,       NULL, 'h'},
        {"version",    no_argument,       NULL, 'V'},
        {"verbose",    no_argument,       NULL, 'v'},
        {"quiet",      no_argument,       NULL, 'q'},
//        {"hypervolume",no_argument,       NULL, 'H'},
        {"keep-uevs",  no_argument,       NULL, 'k'},
        {"rank",       no_argument,       NULL, 'r'},
        {"obj",        required_argument, NULL, 'o'},
        {NULL, 0, NULL, 0} /* marks end of list */
    };
    set_program_invocation_short_name(argv[0]);

    int dim = 0;
    const char * filename;
    const signed char *minmax = NULL;
    bool only_rank_flag = false;
//    bool hypervolume_flag = false;
//    bool keep_uevs_flag = false;

    int opt; /* it's actually going to hold a char */
    int longopt_index;
    while (0 < (opt = getopt_long(argc, argv, short_options,
                                  long_options, &longopt_index))) {
        switch (opt) {
        case 'q': // --quiet
            verbose_flag = false;
            break;

        case 'v': // --verbose
            verbose_flag = true;
            break;

        case 'r': // --rank
            only_rank_flag = true;
            break;

        case 'k': // --keep-uevs
//            keep_uevs_flag = true;
            fprintf(stderr, "%s: --keep-uevs not implemented yet!\n",program_invocation_short_name);
            exit(EXIT_FAILURE);
            break;

        case 'o': // --obj
            minmax = parse_cmdline_minmax(minmax, optarg, &dim);
            break;

        default:
            default_cmdline_handler(opt);
        }
    }

    int numfiles = argc - optind;

    if (numfiles <= 0) {/* No input files: read stdin.  */
        filename = NULL;
    } else if (numfiles == 1) {
        filename = argv[optind];
    } else {
        errprintf ("more than one input file not handled yet.");
        exit(EXIT_FAILURE);
    }

    double * points = NULL;
    int * cumsizes = NULL;
    int nsets = 0;
    robust_read_double_data(filename, &points, &dim, &cumsizes, &nsets, /* union_flag=*/true);
    if (filename == NULL)
        filename = stdin_name;
    const int size = cumsizes[0];
    free(cumsizes);

    // Default minmax if not set yet.
    if (minmax == NULL)
        minmax = minmax_minimise((dimension_t) dim);

    if (verbose_flag) {
        printf ("# file: %s\n", filename);
        printf ("# points: %d\n", size);
    }

    int * rank = pareto_rank(points, size, (dimension_t) dim);

    if (only_rank_flag) {
        fprint_rank(stdout, rank, size);

    } else {
        bool *uev = NULL;
        static const double upper_range = 0.9;
        static const double lower_range = 0.0;

        double * order = malloc (sizeof(*order) * size);
        int max_rank = 0;
        for (int k = 0; k < size; k++) {
            if (rank[k] > max_rank) max_rank = rank[k];
            order[k] = rank[k];
        }

        double * data = malloc (sizeof(double) * size * dim);
        double * lbound = malloc(sizeof(double) * dim);
        double * ubound = malloc(sizeof(double) * dim);
        double * ref = malloc(sizeof(double) * dim);

        for (int d = 0; d < dim; d++)
            ref[d] = 1.0;

        max_rank = 1;
        for (int i = 1; i <= max_rank; i++) {
            for (int d = 0; d < dim; d++) {
                lbound[d] = INFINITY;
                ubound[d] = -INFINITY;
            }
            int data_size = 0;
            for (int k = 0; k < size; k++) {
                if (rank[k] != i) continue;
                const double *src = points + k * dim;
                memcpy (data + data_size * dim, src, sizeof(double) * dim);
                data_size++;
                for (int d = 0; d < dim; d++) {
                    if (lbound[d] > src[d]) lbound[d] = src[d];
                    if (ubound[d] < src[d]) ubound[d] = src[d];
                }
            }

            uev = calculate_uev(uev, data, dim, data_size, lbound, ubound);

            normalise(data, data_size, (dimension_t)dim, minmax, AGREE_NONE,
                      lower_range, upper_range, lbound, ubound);

            double * hvc = malloc(sizeof(*hvc) * data_size);
            hv_contributions(hvc, data, data_size, (dimension_t) dim, ref, /*ignore_dominated=*/true);
            /* FIXME: handle uevs: keep_uevs_flag ? uev : NULL);*/
            for (int k = 0, j = 0; k < size; k++) {
                if (rank[k] != i) continue;
                order[k] += (1 - hvc[j++]);
            }
            free (hvc);
        }
        free (data);
        free (lbound);
        free (ubound);
        free (ref);
        fprint_vector_double (stdout, order, size);
        free (order);
    }

    free(rank);
    free(points);
    free((void *) minmax);
    return 0;
}
