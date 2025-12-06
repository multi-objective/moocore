/*************************************************************************

 hvapprox: main program

 ---------------------------------------------------------------------

                       Copyright (c) 2025
             Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ----------------------------------------------------------------------

 Relevant literature:

*************************************************************************/
#include "config.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>  // for getopt()
#include <getopt.h> // for getopt_long()
#include <time.h> // time()
#include <limits.h> // LONG_MAX
#include <inttypes.h> // PRIu32

#include "timer.h"
#include "nondominated.h"
#include "hvapprox.h"
#define CMDLINE_COPYRIGHT_YEARS "2025"
#define CMDLINE_AUTHORS "Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>\n"
#include "cmdline.h"

static int verbose_flag = 1;
static bool union_flag = false;
static char *suffix = NULL;

enum approx_method_t { DZ2019_MC=1, DZ2019_HW=2 };

static void usage(void)
{
    printf("\n"
           "Usage: %s [OPTIONS] [FILE...]\n\n", program_invocation_short_name);

    printf(
"Approximate the hypervolume value of each input set of each FILE. \n"
"The approximation uses (quasi-)Monte-Carlo sampling, thus gets more accurate with larger\n"
"values of --nsamples. With no FILE, or when FILE is -, read standard input.\n\n"

"Options:\n"
OPTION_HELP_STR
OPTION_VERSION_STR
" -v, --verbose       print some information (time, maximum, etc).          \n"
" -q, --quiet         print just the hypervolume (as opposed to --verbose). \n"
" -u, --union         treat all input sets within a FILE as a single set.   \n"
" -r, --reference=POINT use POINT as reference point. POINT must be within  \n"
"                     quotes, e.g., \"10 10 10\". If no reference point is  \n"
"                     given, it is taken as max + 0.1 * (max - min) for each\n"
"                     coordinate from the union of all input points.        \n"
" -s, --suffix=STRING Create an output file for each input file by appending\n"
"                     this suffix. This is ignored when reading from stdin. \n"
"                     If missing, output is sent to stdout.                 \n"
" -n, --nsamples=N    Number of Monte-Carlo samples (N is a positive integer).\n"
" -m, --method=M      1: Monte-Carlo sampling using normal distribution;    \n"
"                     2: Hua-Wang deterministic sampling (default).         \n"
OPTION_SEED_STR
"                     Only method=1.                                        \n"
"\n");
}

/*
   FILENAME: input filename. If NULL, read stdin.

   REFERENCE: reference point. If NULL, use MAXIMUM.

   MAXIMUM: maximum objective vector. If NULL, caculate it from the
   input file.

   NOBJ_P: pointer to number of objectives. If NULL, calculate it from
   input file.

*/

// FIXME: There is a similar function in main-hv.c
static void
hvapprox_file(const char * filename, double * restrict reference,
              double * restrict maximum, double * restrict minimum,
              int * restrict nobj_p,
              uint_fast32_t nsamples, enum approx_method_t hv_approx_method, uint32_t seed)
{
    double * data = NULL;
    int * cumsizes = NULL;
    int nruns = 0;
    robust_read_double_data(filename, &data, nobj_p, &cumsizes, &nruns, union_flag);
    if (filename == NULL)
        filename = stdin_name;
    dimension_t nobj = (dimension_t) *nobj_p;

    const char * outfilename = NULL;
    FILE * outfile = fopen_outfile(&outfilename, filename, suffix);

    if (verbose_flag >= 2)
        printf("# file: %s\n", filename);

    bool needs_minimum = (minimum == NULL);
    if (needs_minimum) {
        data_bounds(&minimum, &maximum, data, cumsizes[nruns-1], nobj);
        if (verbose_flag >= 2) {
            printf ("# minimum:   ");
            vector_printf (minimum, nobj);
            printf ("\n");
            printf ("# maximum:   ");
            vector_printf (maximum, nobj);
            printf ("\n");
        }
    }

    if (reference != NULL) {
        for (dimension_t n = 0; n < nobj; n++) {
            if (reference[n] <= maximum[n]) {
                warnprintf ("%s: some points do not strictly dominate "
                            "the reference point and they will be discarded",
                            filename);
                break;
            }
        }
    } else {
        reference = malloc(nobj * sizeof(*reference));
        for (dimension_t n = 0; n < nobj; n++) {
            // Default reference point is:
            reference[n] = maximum[n] + 0.1 * (maximum[n] - minimum[n]);
            // so that extreme points have some influence.
        }
    }

    if (verbose_flag >= 2) {
        printf ("# reference: ");
        vector_printf (reference, nobj);
        printf ("\n");
    }

    // Minimise everything by default.
    const bool * maximise = new_bool_maximise(nobj, false);
    for (int n = 0, cumsize = 0; n < nruns; cumsize = cumsizes[n], n++) {
        Timer_start ();

        double volume;
        switch (hv_approx_method) {
          case DZ2019_MC:
              volume = hv_approx_normal(&data[nobj * cumsize], cumsizes[n] - cumsize, nobj, reference, maximise, nsamples, seed);
              break;
          case DZ2019_HW:
              volume = hv_approx_hua_wang(&data[nobj * cumsize], cumsizes[n] - cumsize, nobj, reference, maximise, nsamples);
              break;
          default:
              unreachable();  // COVR_EXCL_LINE # nocov
        }

        if (volume == 0.0) {
            fatal_error("none of the points strictly dominates the reference point\n");
        }

        double time_elapsed = Timer_elapsed_virtual ();

        fprintf (outfile, indicator_printf_format "\n", volume);
        if (verbose_flag >= 2)
            fprintf (outfile, "# Time: %f seconds\n", time_elapsed);
    }

    fclose_outfile(outfile, filename, outfilename, verbose_flag);
    free((void *) maximise);
    free(data);
    free(cumsizes);
    if (needs_minimum) {
        free (minimum);
        free (maximum);
    }
}

int main(int argc, char *argv[])
{
    // See the man page for getopt_long for an explanation of these fields.
    static const char short_options[] = "hVvqur:s:n:m:S:";
    static const struct option long_options[] = {
        {"help",       no_argument,       NULL, 'h'},
        {"version",    no_argument,       NULL, 'V'},
        {"verbose",    no_argument,       NULL, 'v'},
        {"quiet",      no_argument,       NULL, 'q'},
        {"reference",  required_argument, NULL, 'r'},
        {"union",      no_argument,       NULL, 'u'},
        {"suffix",     required_argument, NULL, 's'},
        {"method",     required_argument, NULL, 'm'},
        {"nsamples",   required_argument, NULL, 'n'},
        {"seed",       required_argument, NULL, 'S'},
        {NULL, 0, NULL, 0} /* marks end of list */
    };

    set_program_invocation_short_name(argv[0]);

    double * reference = NULL;
    int nobj = 0;
    uint32_t seed = 0;
    uint_fast32_t nsamples = 0;
    enum approx_method_t hv_approx_method = DZ2019_HW;

    int opt; /* it's actually going to hold a char.  */
    int longopt_index;
    while (0 < (opt = getopt_long (argc, argv, short_options,
                                   long_options, &longopt_index))) {
        switch (opt) {
          case 'r': // --reference
              reference = robust_read_point(optarg, &nobj, "invalid reference point '%s'");
              break;

          case 'u': // --union
              union_flag = true;
              break;

          case 's': // --suffix
              suffix = optarg;
              break;

          case 'n': { // --nsamples
              char *endp;
              long int value = strtol(optarg, &endp, 10);
              if (endp == optarg || *endp != '\0' || value <= 0 || value == LONG_MAX) {
                  fatal_error("value of --nsamples must be a positive integer '%s'", optarg);
              }
              nsamples = (uint_fast32_t) value;
              break;
          }

          case 'm': // --method
              switch (*optarg) {
                case '1':
                    hv_approx_method = DZ2019_MC; break;
                case '2':
                    hv_approx_method = DZ2019_HW; break;
                default:
                    fatal_error("valid values of --method (-m) are: 1 or 2, not '%s'", optarg);
              }
              break;

          case 'S': {// --seed
              char *endp;
              long int value = strtol(optarg, &endp, 10);
              if (endp == optarg || *endp != '\0' || value <= 0) {
                  fatal_error("value of --seed must be a positive integer '%s'", optarg);
              }
              seed = (uint32_t) value;
              break;
          }
        case 'q': // --quiet
            verbose_flag = 0;
            break;

        case 'v': // --verbose
            verbose_flag = 2;
            break;

        default:
            default_cmdline_handler(opt);
        }
    }

    if (nsamples == 0) {
        fatal_error("must specify a value for --nsamples, for example, --nsamples 100000");
    }

    if (seed == 0) {
        if (hv_approx_method == DZ2019_MC)
            seed = (uint32_t) time(NULL);
    } else if (hv_approx_method == DZ2019_HW) {
        fatal_error("cannot use --seed with --method=2");
    }

    if (verbose_flag >= 2)
        printf("# seed: %"PRIu32 "\n# nsamples: %lu\n", seed, (unsigned long) nsamples);

    int numfiles = argc - optind;
    if (numfiles < 1) /* Read stdin.  */
        hvapprox_file(NULL, reference, NULL, NULL, &nobj, nsamples, hv_approx_method, seed);

    else if (numfiles == 1) {
        hvapprox_file (argv[optind], reference, NULL, NULL, &nobj, nsamples, hv_approx_method, seed);
    } else {
        int k;
        double * maximum = NULL;
        double * minimum = NULL;
        if (reference == NULL) {
            /* Calculate the maximum among all input files to use as
               reference point.  */
            for (k = 0; k < numfiles; k++)
                file_bounds(argv[optind + k], &maximum, &minimum, &nobj);

            if (verbose_flag >= 2) {
                printf ("# maximum:");
                vector_printf (maximum, nobj);
                printf ("\n");
                printf ("# minimum:");
                vector_printf (minimum, nobj);
                printf ("\n");
            }
        }
        for (k = 0; k < numfiles; k++)
            hvapprox_file (argv[optind + k], reference, maximum, minimum, &nobj, nsamples, hv_approx_method, seed);

        free(minimum);
        free(maximum);
    }

    if (reference) free(reference);
    return EXIT_SUCCESS;
}
