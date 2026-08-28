/*************************************************************************

 hvapprox: main program

 ---------------------------------------------------------------------

                       Copyright (c) 2026
             Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, you can obtain one at https://mozilla.org/MPL/2.0/.

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
#define CMDLINE_COPYRIGHT_YEARS "2026"
#define CMDLINE_AUTHORS "Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>\n"
#include "cmdline.h"

static int verbose_flag = 1;
static bool union_flag = false;
static char *suffix = NULL;

enum approx_method_t { DZ2019_MC=1, DZ2019_HW=2, Rphi_FWEp=3, FPRAS=4 };
struct hvapprox_params_t {
    enum approx_method_t method;
    uint32_t seed;
    uint_fast32_t nsamples;
    double epsilon;
    double delta;
};
static const char * approx_method_str[] = {"DZ2019-MC", "DZ2019-HW", "Rphi-FWE+", "FPRAS"};

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
" -m, --method=M      1: Monte-Carlo sampling using normal distribution;    \n"
"                     2: Hua-Wang deterministic sampling.                   \n"
"                     3: Rphi-FWE+ deterministic sampling (default).        \n"
"                     4: FPRAS (Bringmann, Friedrich, 2010).                \n"
" -n, --nsamples=N    Number of Monte-Carlo samples (N is a positive integer).\n"
OPTION_SEED_STR
"                     Only method=1 or method=4.\n"
" -e, --epsilon=E     Desired relative error of the approximation, E > 0.\n"
"                     Only method=4.\n"
" -d, --delta=D       Desired failure probability 0 < D < 1, where (1 - D) \n"
"                     gives the confidence level. Only method=4.\n"
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
              int * restrict nobj_p, struct hvapprox_params_t hvapprox)
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
    const boolvec * maximise = new_boolvec_maximise(nobj, false);
    for (int n = 0, cumsize = 0; n < nruns; cumsize = cumsizes[n], n++) {
        Timer_start ();

        double volume;
        switch (hvapprox.method) {
          case DZ2019_MC:
              volume = hv_approx_normal(&data[nobj * cumsize], cumsizes[n] - cumsize, nobj, reference, maximise, hvapprox.nsamples, hvapprox.seed);
              break;
          case DZ2019_HW:
              volume = hv_approx_hua_wang(&data[nobj * cumsize], cumsizes[n] - cumsize, nobj, reference, maximise, hvapprox.nsamples);
              break;
          case Rphi_FWEp:
              volume = hv_approx_rphi_fang_wang_plus(&data[nobj * cumsize], cumsizes[n] - cumsize, nobj, reference, maximise, hvapprox.nsamples);
              break;
          case FPRAS:
              volume = hv_approx_fpras(&data[nobj * cumsize], cumsizes[n] - cumsize, nobj, reference, maximise, hvapprox.seed, hvapprox.epsilon, hvapprox.delta);
              break;
          default:  // LCOV_EXCL_LINE # nocov
              unreachable();
        }

        if (volume == 0.0)
            fatal_error("none of the points strictly dominates the reference point\n");

        double time_elapsed = Timer_elapsed_virtual();
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
    static const char short_options[] = "hVvqur:s:n:m:S:e:d:";
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
        {"epsilon",    required_argument, NULL, 'e'},
        {"delta",      required_argument, NULL, 'd'},
        {NULL, 0, NULL, 0} /* marks end of list */
    };

    set_program_invocation_short_name(argv[0]);

    double * reference = NULL;
    int nobj = 0;
    struct hvapprox_params_t hvapprox = {
        .method = Rphi_FWEp, .seed = 0, .nsamples = 0, .epsilon = 0.01, .delta = 0.1
    };

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
              if (endp == optarg || *endp != '\0' || value <= 0 || value == LONG_MAX)
                  fatal_error("value of --nsamples must be a positive integer '%s'", optarg);
              hvapprox.nsamples = (uint_fast32_t) value;
              break;
          }

          case 'e': { // --epsilon
              char *endp;
              double value = strtod(optarg, &endp);
              if (endp == optarg || *endp != '\0' || value <= 0 || value == HUGE_VAL || !is_normal(value))
                  fatal_error("value of --epsilon must be a positive floating-point value, not '%s'", optarg);
              hvapprox.epsilon = value;
              break;
          }

          case 'd': { // --delta
              char *endp;
              double value = strtod(optarg, &endp);
              if (endp == optarg || *endp != '\0' || value <= 0 || value >= 1 || !is_normal(value))
                  fatal_error("value of --delta must be a floating-point value within (0, 1), not '%s'", optarg);
              hvapprox.delta = value;
              break;
          }

          case 'm': // --method
              switch (*optarg) {
                case '1':
                    hvapprox.method = DZ2019_MC; break;
                case '2':
                    hvapprox.method = DZ2019_HW; break;
                case '3':
                    hvapprox.method = Rphi_FWEp; break;
                case '4':
                    hvapprox.method = FPRAS; break;
                default:
                    fatal_error("valid values of --method (-m) are: 1, 2, 3, or 4 not '%s'", optarg);
              }
              break;

          case 'S': {// --seed
              char *endp;
              long int value = strtol(optarg, &endp, 10);
              if (endp == optarg || *endp != '\0' || value <= 0)
                  fatal_error("value of --seed must be a positive integer '%s'", optarg);
              hvapprox.seed = (uint32_t) value;
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

    if (hvapprox.nsamples == 0 && hvapprox.method != FPRAS)
        fatal_error("must specify a value for --nsamples, for example, --nsamples 524288");
    if (hvapprox.nsamples != 0 && hvapprox.method == FPRAS)
        fatal_error("--nsamples does not make sense with --method=4");

    if (hvapprox.method == DZ2019_MC || hvapprox.method == FPRAS) {
        if (hvapprox.seed == 0)
            hvapprox.seed = (uint32_t) time(NULL);
    } else if (hvapprox.seed != 0)
        fatal_error("--seed only makes sense with --method=1 or --method=4");

    if (verbose_flag >= 2) {
        printf("# Method: %s\n", approx_method_str[hvapprox.method - 1]);
        if (hvapprox.method == DZ2019_MC || hvapprox.method != FPRAS)
            printf("# seed: %"PRIu32 "\n", hvapprox.seed);
        if (hvapprox.method != FPRAS)
            printf("# nsamples: %lu\n", (unsigned long) hvapprox.nsamples);
        else
            printf("# epsilon: %g\n# delta: %g\n", hvapprox.epsilon, hvapprox.delta);
    }

    int numfiles = argc - optind;
    if (numfiles < 1) /* Read stdin.  */
        hvapprox_file(NULL, reference, NULL, NULL, &nobj, hvapprox);

    else if (numfiles == 1) {
        hvapprox_file (argv[optind], reference, NULL, NULL, &nobj, hvapprox);
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
            hvapprox_file (argv[optind + k], reference, maximum, minimum, &nobj,
                           hvapprox);

        free(minimum);
        free(maximum);
    }

    if (reference) free(reference);
    return EXIT_SUCCESS;
}
