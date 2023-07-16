/*************************************************************************

 epsilon:

 ---------------------------------------------------------------------

                       Copyright (c) 2010
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
 General Public License at:
                 http://www.gnu.org/copyleft/gpl.html
 or by writing to:
           Free Software Foundation, Inc., 59 Temple Place,
                 Suite 330, Boston, MA 02111-1307 USA

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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> // for isspace()

#include <unistd.h>  // for getopt()
#include <getopt.h> // for getopt_long()
#include "epsilon.h"
#include "nondominated.h"
#include "cmdline.h"

static bool verbose_flag = false;
static bool additive_flag = true;
static const char *suffix = NULL;

static void usage(void)
{
    printf("\n"
           "Usage:\n"
           "       %s [OPTIONS] [FILES] \n"
           "       %s [OPTIONS] < [INPUT] > [OUTPUT]\n\n",
           program_invocation_short_name, program_invocation_short_name);

    printf(
"Calculates the epsilon measure for the Pareto sets given as input\n\n"

"Options:\n"
" -h, --help           give  this summary and exit.                          \n"
"     --version        print version number and exit.                        \n"
" -v, --verbose        print some information (time, number of points, etc.) \n"
" -q, --quiet          print as little as possible                           \n"
" -a, --additive       epsilon additive value %s                             \n"
" -m, --multiplicative epsilon multiplicative value %s                       \n"
" -r, --reference FILE file that contains the reference set                  \n"
" -o, --obj [+|-]...   specify whether each objective should be              \n"
"                      minimised (-) or maximised (+) (default all minimised)\n"
" -s, --suffix=STRING Create an output file for each input file by appending\n"
"                     this suffix. This is ignored when reading from stdin. \n"
"                     If missing, output is sent to stdout.                 \n"
"\n", str_is_default(additive_flag), str_is_default(!additive_flag));
}

static void
do_file (const char *filename, double *reference, int reference_size,
         int *nobj_p, const signed char * minmax)
{
    double *data = NULL;
    int *cumsizes = NULL;
    int nruns = 0;
    int nobj = *nobj_p;

    int err = read_double_data (filename, &data, &nobj, &cumsizes, &nruns);
    if (!filename) filename = stdin_name;
    handle_read_data_error (err, filename);

    char *outfilename = NULL;
    FILE *outfile = stdout;
    if (filename != stdin_name && suffix) {
        outfilename = m_strcat(filename, suffix);
        outfile = fopen (outfilename, "w");
        if (outfile == NULL) {
            errprintf ("%s: %s\n", outfilename, strerror(errno));
            exit (EXIT_FAILURE);
        }
    }
#if 0
    if (union_flag) {
        cumsizes[0] = cumsizes[nruns - 1];
        nruns = 1;
    }
#endif
    /* Default minmax if not set yet.  */
    bool free_minmax = false;
    if (minmax == NULL) {
        minmax = read_minmax (NULL, &nobj);
        free_minmax = true;
    }

    if (verbose_flag)
        printf("# file: %s\n", filename);

    int cumsize;
    int n;
    for (n = 0, cumsize = 0; n < nruns; cumsize = cumsizes[n], n++) {
        // double time_elapsed = 0;
        //Timer_start ();
        double epsilon = (additive_flag)
            ? epsilon_additive_minmax (nobj,  minmax,
                                       &data[nobj * cumsize], cumsizes[n] - cumsize,
                                       reference, reference_size)
            : epsilon_mult_minmax (nobj,  minmax,
                                   &data[nobj * cumsize], cumsizes[n] - cumsize,
                                   reference, reference_size);
        //        time_elapsed = Timer_elapsed_virtual ();
        fprintf (outfile, "%-16.15g\n", epsilon);
        if ((additive_flag && epsilon < 0)
            || (!additive_flag && epsilon < 1)) {
            errprintf ("%s: some points are not  dominated by the reference set",
                       filename);
            exit (EXIT_FAILURE);
        }/*
        if (verbose_flag)
            fprintf (outfile, "# Time: %f seconds\n", time_elapsed);
         */
    }

    if (outfilename) {
        if (verbose_flag)
            fprintf (stderr, "# %s -> %s\n", filename, outfilename);
        fclose (outfile);
        free (outfilename);
    }
    free (data);
    free (cumsizes);
    if (free_minmax) free( (void *) minmax);
    *nobj_p = nobj;
}

int main(int argc, char *argv[])
{
    double *reference = NULL;
    int reference_size = 0;
    int nobj = 0;
    const signed char *minmax = NULL;
    /* see the man page for getopt_long for an explanation of these fields */
    static struct option long_options[] = {
        {"help",       no_argument,       NULL, 'h'},
        {"version",    no_argument,       NULL, 'V'},
        {"verbose",    no_argument,       NULL, 'v'},
        {"quiet",      no_argument,       NULL, 'q'},
        {"additive",   no_argument,       NULL, 'a'},
        {"multiplicative",   no_argument, NULL, 'm'},
        {"reference",  required_argument, NULL, 'r'},
        {"suffix",     required_argument, NULL, 's'},
        {"obj",        required_argument, NULL, 'o'},

        {NULL, 0, NULL, 0} /* marks end of list */
    };

    set_program_invocation_short_name(argv[0]);

    int opt; /* it's actually going to hold a char */
    int longopt_index;
    while (0 < (opt = getopt_long(argc, argv, "hVvqamr:us:o:",
                                  long_options, &longopt_index))) {
        switch (opt) {
        case 'a': // --additive
            additive_flag = true;
            break;

        case 'm': // --multiplicative
            additive_flag = false;
            break;

        case 'o': // --obj
            minmax = read_minmax (optarg, &nobj);
            if (minmax == NULL) {
                fprintf(stderr, "%s: invalid argument '%s' for -o, --obj\n",
                        program_invocation_short_name,optarg);
                exit(EXIT_FAILURE);
            }
            break;

        case 'r': // --reference
            reference_size = read_reference_set (&reference, optarg, &nobj);
            if (reference == NULL || reference_size <= 0) {
                errprintf ("invalid reference set '%s", optarg);
                exit (EXIT_FAILURE);
            }
            break;

        case 's': // --suffix
            suffix = optarg;
            break;

        case 'V': // --version
            version();
            exit(EXIT_SUCCESS);

        case 'q': // --quiet
            verbose_flag = false;
            break;

        case 'v': // --verbose
            verbose_flag = true;
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

    if (verbose_flag)
        fprintf (stderr, (additive_flag)
                 ? "# Additive epsilon indicator\n"
                 : "# Multiplicative epsilon indicator\n");

    if (reference == NULL) {
        errprintf ("a reference set must be provided (--reference)");
        exit (EXIT_FAILURE);
    }

    int numfiles = argc - optind;
    if (numfiles < 1) {/* Read stdin.  */
        do_file (NULL, reference, reference_size, &nobj, minmax);
    } else if (numfiles == 1) {
        do_file (argv[optind], reference, reference_size, &nobj, minmax);
    } else {
        int k;
        /* FIXME: Calculate the nondominated front among all input
           files to use as reference set.  */
#if 0
        if (reference == NULL) {
            reference_size =
                calculate_nondominated (&reference, data, nobj, cumsizes[nruns-1],
                                        minmax);
            write_sets (stderr, reference, nobj, &reference_size, 1);
        }
        for (k = 0; k < numfiles; k++)
            nondominatedfile_range (argv[optind + k], &maximum, &minimum, &nobj);

        if (verbose_flag >= 2) {
            printf ("# maximum:");
            vector_printf (maximum, nobj);
            printf ("\n");
            printf ("# minimum:");
            vector_printf (minimum, nobj);
            printf ("\n");
        }
#endif
        for (k = 0; k < numfiles; k++)
            do_file (argv[optind + k], reference, reference_size, &nobj, minmax);
    }

    return EXIT_SUCCESS;
}
