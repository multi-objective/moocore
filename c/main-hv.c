/*************************************************************************

 hv: main program

 ---------------------------------------------------------------------

                       Copyright (c) 2010
                  Carlos M. Fonseca <cmfonsec@dei.uc.pt>
             Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>
                    Luis Paquete <paquete@dei.uc.pt>

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
 General Public License at:
                 http://www.gnu.org/copyleft/gpl.html
 or by writing to:
           Free Software Foundation, Inc., 59 Temple Place,
                 Suite 330, Boston, MA 02111-1307 USA

 ----------------------------------------------------------------------

 Relevant literature:

 [1]  C. M. Fonseca, L. Paquete, and M. Lopez-Ibanez. An
      improved dimension-sweep algorithm for the hypervolume
      indicator. In IEEE Congress on Evolutionary Computation,
      pages 1157-1163, Vancouver, Canada, July 2006.

 [2]  Nicola Beume, Carlos M. Fonseca, Manuel Lopez-Ibanez, Luis Paquete, and
      J. Vahrenhold.  On the complexity of computing the hypervolume
      indicator. IEEE Transactions on Evolutionary Computation,
      13(5):1075-1082, 2009.

*************************************************************************/
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h> // for isspace()

#include <unistd.h>  // for getopt()
#include <getopt.h> // for getopt_long()

#include "hv.h"
#include "timer.h"
#define CMDLINE_COPYRIGHT_YEARS "2010-2023"
#define CMDLINE_AUTHORS "Carlos M. Fonseca <cmfonsec@dei.uc.pt>\n" \
    "Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>\n" \
    "Luis Paquete <paquete@dei.uc.pt>\n"
#include "cmdline.h"

static int verbose_flag = 1;
static bool union_flag = false;
static bool shift_flag = false;
static char *suffix = NULL;

static void usage(void)
{
    printf("\n"
           "Usage: %s [OPTIONS] [FILE...]\n\n", program_invocation_short_name);

    printf(
"Calculate the hypervolume of each input set of each FILE. \n"
"With no FILE, or when FILE is -, read standard input.\n\n"

"Options:\n"
" -h, --help          print this summary and exit.                          \n"
"     --version       print version number and exit.                        \n"
" -v, --verbose       print some information (time, maximum, etc).          \n"
" -q, --quiet         print just the hypervolume (as opposed to --verbose). \n"
" -u, --union         treat all input sets within a FILE as a single set.   \n"
" -r, --reference=POINT use POINT as reference point. POINT must be within  \n"
"                     quotes, e.g., \"10 10 10\". If no reference point is  \n"
/*
"                     given, it is taken as the maximum for each coordinate \n"
"                     from the union of all input points.        \n"
*/
"                     given, it is taken as max + 0.1 * (max - min) for each\n"
"                     coordinate from the union of all input points.        \n"
" -s, --suffix=STRING Create an output file for each input file by appending\n"
"                     this suffix. This is ignored when reading from stdin. \n"
"                     If missing, output is sent to stdout.                 \n"
" -S, --shift         Shift the data before calculating the hypervolume.    \n"
"                     This consumes more memory but it is faster.           \n"
"\n");
}

static double *
read_reference(char * str, int *nobj)
{
    char * cursor;

    int k = 0, size = 10;

    double * reference = malloc(size * sizeof(double));
    char * endp = str;

    do {
        cursor = endp;
        if (k == size) {
            size += 10;
            reference = realloc(reference, size * sizeof(double));
        }
        reference[k] = strtod(cursor, &endp);
        k++;
    } while (cursor != endp);

    // not end of string: error
    while (*cursor != '\0') {
        if (!isspace(*cursor)) return NULL;
        cursor++;
    }

    // no number: error
    if (k == 1) return NULL;

    *nobj = k - 1;
    return reference;
}

static void
data_bounds (double **minimum, double **maximum,
            const double *data, int nobj, int rows)
{
    int k = 0;
    int r = 0;

    if (*minimum == NULL) {
        *minimum = malloc (nobj * sizeof(double));
        for (k = 0; k < nobj; k++)
            (*minimum)[k] = data[k];
        r = 1;
    }

    if (*maximum == NULL) {
        *maximum = malloc (nobj * sizeof(double));
        for (k = 0; k < nobj; k++)
            (*maximum)[k] = data[k];
        r = 1;
    }

    for (; r < rows; r++) {
        for (int n = 0; n < nobj; n++, k++) {
            if ((*maximum)[n] < data[k])
                (*maximum)[n] = data[k];
            if ((*minimum)[n] > data[k])
                (*minimum)[n] = data[k];
        }
    }
}

static void
file_range (const char *filename, double **maximum_p, double **minimum_p,
            int *dim_p)
{
    double *data = NULL;
    int *cumsizes = NULL;
    int nruns = 0;
    int dim = *dim_p;
    double *maximum = *maximum_p;
    double *minimum = *minimum_p;

    handle_read_data_error (
        read_double_data (filename, &data, &dim, &cumsizes, &nruns), filename);

    data_bounds (&minimum, &maximum, data, dim, cumsizes[nruns-1]);

    *dim_p = dim;
    *maximum_p = maximum;
    *minimum_p = minimum;

    free (data);
    free (cumsizes);
}

/*
   FILENAME: input filename. If NULL, read stdin.

   REFERENCE: reference point. If NULL, use MAXIMUM.

   MAXIMUM: maximum objective vector. If NULL, caculate it from the
   input file.

   NOBJ_P: pointer to number of objectives. If NULL, calculate it from
   input file.

*/

static void
hv_file (const char *filename, double *reference,
         double *maximum, double *minimum, int *nobj_p)
{
    double *data = NULL;
    int *cumsizes = NULL;
    int cumsize;
    int nruns = 0;
    int n;
    int nobj = *nobj_p;
    char *outfilename = NULL;
    FILE *outfile = stdout;

    int err = read_double_data (filename, &data, &nobj, &cumsizes, &nruns);
    if (!filename) filename = stdin_name;
    handle_read_data_error (err, filename);

    if (filename != stdin_name && suffix) {
        size_t outfilename_len = strlen(filename) + strlen(suffix) + 1;
        outfilename = malloc (sizeof(char) * outfilename_len);
        strcpy (outfilename, filename);
        strcat (outfilename, suffix);

        outfile = fopen (outfilename, "w");
        if (outfile == NULL) {
            errprintf ("%s: %s\n", outfilename, strerror(errno));
            exit (EXIT_FAILURE);
        }
    }

    if (union_flag) {
        cumsizes[0] = cumsizes[nruns - 1];
        nruns = 1;
    }

    if (verbose_flag >= 2)
        printf("# file: %s\n", filename);

    bool needs_minimum = (minimum == NULL);
    if (needs_minimum) {
        data_bounds (&minimum, &maximum, data, nobj, cumsizes[nruns-1]);
        if (verbose_flag >= 2) {
            printf ("# minimum:");
            vector_printf (minimum, nobj);
            printf ("\n");
            printf ("# maximum:");
            vector_printf (maximum, nobj);
            printf ("\n");
        }
    }

    if (reference != NULL) {
        for (n = 0; n < nobj; n++) {
            if (reference[n] <= maximum[n]) {
                warnprintf ("%s: some points do not strictly dominate "
                            "the reference point and they will be discarded",
                            filename);
                break;
            }
        }
    } else {
        reference = malloc(nobj * sizeof(double));
        for (n = 0; n < nobj; n++) {
            /* default reference point is: */
            reference[n] = maximum[n] + 0.1 * (maximum[n] - minimum[n]);
            /* so that extreme points have some influence. */
        }
    }

    if (verbose_flag >= 2) {
        printf ("# reference:");
        vector_printf (reference, nobj);
        printf ("\n");
    }

    for (n = 0, cumsize = 0; n < nruns; cumsize = cumsizes[n], n++) {
        Timer_start ();

        double volume = (shift_flag)
            ? fpli_hv_shift (&data[nobj * cumsize], nobj, cumsizes[n] - cumsize, reference)
            : fpli_hv (&data[nobj * cumsize], nobj, cumsizes[n] - cumsize, reference);

        if (volume == 0.0) {
            errprintf ("none of the points strictly dominates the reference point\n");
            exit (EXIT_FAILURE);
        }

        double time_elapsed = Timer_elapsed_virtual ();

        fprintf (outfile, "%-17.15g\n", volume);
        if (verbose_flag >= 2)
            fprintf (outfile, "# Time: %f seconds\n", time_elapsed);
    }

    if (outfilename) {
        if (verbose_flag)
            fprintf (stderr, "# %s -> %s\n", filename, outfilename);
        fclose (outfile);
        free (outfilename);
    }
    free (data);
    free (cumsizes);
    if (needs_minimum) {
        free (minimum);
        free (maximum);
    }
    *nobj_p = nobj;
}

int main(int argc, char *argv[])
{
    double *reference = NULL;
    int nobj = 0;
    int opt; /* it's actually going to hold a char.  */
    int longopt_index;

    /* see the man page for getopt_long for an explanation of these fields.  */
    static struct option long_options[] = {
        {"help",       no_argument,       NULL, 'h'},
        {"version",    no_argument,       NULL, 'V'},
        {"verbose",    no_argument,       NULL, 'v'},
        {"quiet",      no_argument,       NULL, 'q'},
        {"reference",  required_argument, NULL, 'r'},
        {"union",      no_argument,       NULL, 'u'},
        {"suffix",     required_argument, NULL, 's'},
        {"shift",      no_argument,       NULL, 'S'},
        {NULL, 0, NULL, 0} /* marks end of list */
    };

    set_program_invocation_short_name(argv[0]);

    while (0 < (opt = getopt_long (argc, argv, "hVvqur:s:S",
                                   long_options, &longopt_index))) {
        switch (opt) {
        case 'r': // --reference
            reference = read_reference (optarg, &nobj);
            if (reference == NULL) {
                errprintf ("invalid reference point '%s'",
                           optarg);
                exit (EXIT_FAILURE);
            }
            break;

        case 'u': // --union
            union_flag = true;
            break;

        case 's': // --suffix
            suffix = optarg;
            break;

        case 'S': // --shift
            shift_flag = true;
            break;

        case 'V': // --version
            version();
            exit(EXIT_SUCCESS);

        case 'q': // --quiet
            verbose_flag = 0;
            break;

        case 'v': // --verbose
            verbose_flag = 2;
            break;

        case '?':
            // getopt prints an error message right here
            fprintf (stderr, "Try `%s --help' for more information.\n",
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

    if (numfiles < 1) /* Read stdin.  */
        hv_file (NULL, reference, NULL, NULL, &nobj);

    else if (numfiles == 1) {
        hv_file (argv[optind], reference, NULL, NULL, &nobj);
    }
    else {
        int k;
        double *maximum = NULL;
        double *minimum = NULL;
        if (reference == NULL) {
            /* Calculate the maximum among all input files to use as
               reference point.  */
            for (k = 0; k < numfiles; k++)
                file_range (argv[optind + k], &maximum, &minimum, &nobj);

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
            hv_file (argv[optind + k], reference, maximum, minimum, &nobj);

        free(minimum);
        free(maximum);
    }

    if (reference) free(reference);
    return EXIT_SUCCESS;
}
