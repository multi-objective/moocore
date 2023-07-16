/*************************************************************************

 ndsort: Perform nondominated sorting in a list of points.

 ---------------------------------------------------------------------
                       Copyright (c) 2011
          Manuel Lopez-Ibanez  <manuel.lopez-ibanez@manchester.ac.uk>

 This program is free software (software libre); you can redistribute
 it and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2 of the
 License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, you can obtain a copy of the GNU
 General Public License at <http://www.gnu.org/copyleft/gpl.html>

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

*************************************************************************/


#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h> // for isspace()

#include <unistd.h>  // for getopt()
#include <getopt.h> // for getopt_long()
#include <math.h>  // for INFINITY
#include "nondominated.h" // for normalise()
#define READ_INPUT_WRONG_INITIAL_DIM_ERRSTR "-o, --obj"
#include "cmdline.h"
#include "hv.h"

static void usage(void)
{
    printf("\n"
           "Usage: %s [OPTIONS] [FILE...]\n\n", program_invocation_short_name);

    printf(
"Perform nondominated sorting in a list of points.                        \n\n"

"Options:\n"
OPTION_HELP_STR
OPTION_VERSION_STR
" -h, --help          give  this summary and exit.                          \n"
"     --version       print version number and exit.                        \n"
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
    int nsets = 0;
    int *cumsizes = NULL;
    double *points = NULL;
    int dim = 0;
    const char *filename;
    const signed char *minmax = NULL;
    bool only_rank_flag = false;
//    bool hypervolume_flag = false;
//    bool keep_uevs_flag = false;

    /* see the man page for getopt_long for an explanation of these fields */
    static struct option long_options[] = {
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

    int opt; /* it's actually going to hold a char */
    int longopt_index;
    while (0 < (opt = getopt_long(argc, argv, "hVvqkro:",
                                  long_options, &longopt_index))) {
        switch (opt) {
        case 'V': // --version
            version();
            exit(EXIT_SUCCESS);

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
            minmax = read_minmax (optarg, &dim);
            if (minmax == NULL) {
                fprintf(stderr, "%s: invalid argument '%s' for -o, --obj\n",
                        program_invocation_short_name,optarg);
                exit(EXIT_FAILURE);
            }
            break;

        case '?':
            // getopt prints an error message right here
            fprintf(stderr, "Try `%s --help' for more information.\n",
                    program_invocation_short_name);
            exit(EXIT_FAILURE);
        case 'h':
            usage();
            exit(EXIT_SUCCESS);
        default: // should never happen
            abort();
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

    /* FIXME: Instead of this strange call, create a wrapper read_data_robust. */
    int err = read_double_data (filename, &points, &dim, &cumsizes, &nsets);
    if (!filename) filename = stdin_name;
    handle_read_data_error (err, filename);

    const int size = cumsizes[0] = cumsizes[nsets - 1];
    nsets = 1;

    /* Default minmax if not set yet.  */
    if (minmax == NULL)
        minmax = read_minmax (NULL, &dim);

    if (verbose_flag) {
        printf ("# file: %s\n", filename);
        printf ("# points: %d\n", size);
    }

    int * rank = pareto_rank (points, dim, size);


    if (only_rank_flag) {

        fprint_rank (stdout, rank, size);

    } else {
        bool *uev = NULL;
        static const double upper_range = 0.9;
        static const double lower_range = 0.0;

        double * order = malloc (sizeof(double) * size);
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

            uev = calculate_uev (uev, data, dim, data_size, lbound, ubound);

            normalise (data, dim, data_size, minmax, AGREE_NONE,
                       lower_range, upper_range,
                       lbound, ubound);

            double *hvc = malloc (sizeof(double) * data_size);
            hv_contributions (hvc, data, dim, data_size, ref);
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

    free (rank);
    free (cumsizes);
    free (points);
    free ((void *) minmax);
    return 0;
}
