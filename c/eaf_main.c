/*************************************************************************

 eaf: Computes the empirical attainment function (EAF) from a number
 of approximation sets.

 ---------------------------------------------------------------------

                    Copyright (c) 2006, 2007, 2008
                  Carlos Fonseca <cmfonsec@ualg.pt>
          Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>

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


*************************************************************************/
#include "eaf.h"

#include <stdio.h>
#include <stdlib.h> // strtol()
#include <ctype.h>  // isprint()
#include <assert.h>
#include <string.h>
#include <stdbool.h> // for bool, true and false

#include <unistd.h>  // for getopt()
#include <getopt.h> // for getopt_long()
#include <errno.h>
#define CMDLINE_COPYRIGHT_YEARS "2009-2023"
#define CMDLINE_AUTHORS "Carlos Fonseca <cmfonsec@ualg.pt>\n" \
    "Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>\n"

#include "cmdline.h"

static void usage(void)
{
    printf("\nUsage:\t%s [OPTIONS] [FILE...]\n\n", program_invocation_short_name);
    printf(
"Computes the empirical attainment function (EAF) of all input FILEs. \n"
"With no FILE, or when FILE is -, read standard input.\n\n"

"Options:\n"
" -h, --help          print this summary and exit                           \n"
"     --version       print version number (and compilation flags) and exit \n"
" -v, --verbose       print some information (time, input points, output    \n"
"                     points, etc) in stderr. Default is --quiet            \n"
" -o, --output FILE   write output to FILE instead of standard output.      \n"
" -q, --quiet         print just the EAF (as opposed to --verbose)          \n"
" -b, --best          compute best attainment surface                       \n"
" -m, --median        compute median attainment surface                     \n"
" -w, --worst         compute worst attainment surface                      \n"
" -p, --percentile REAL compute the given percentile of the EAF             \n"
" -l, --level  LEVEL    compute the given level of the EAF                  \n"
" -i[FILE], --indices[=FILE]  write attainment indices to FILE.             \n"
"                     If FILE is '-', print to stdout.                      \n"
"                     If FILE is missing use the same file as for output.   \n"
" -d[FILE], --diff[=FILE] write difference between half of runs to FILE.    \n"
"                     If FILE is '-', print to stdout.                      \n"
"                     If FILE is missing use the same file as for output.   \n"
"        , --polygons Write EAF as R polygons.                             \n"
"\n\n"        );
}

// FIXME: How to implement this with const char *str?
static int read_doubles (double *vec, char *str)
{
    char * cursor;
    char * endp = str;
    int k = 0;

    do {
        cursor = endp;
        vec[k] = strtod(cursor, &endp);
        if (cursor == endp && (*endp == ',' || *endp == ';')) {
            endp++;
            continue;
        }
        k++;
    } while (cursor != endp);

    // not end of string: error
    while (*cursor != '\0') {
        if (!isspace(*cursor)) {
            errprintf ("invalid argument to --percentiles '%s'", str);
            exit (EXIT_FAILURE);
        }
        cursor++;
    }

    // no number: error
    if (k == 1) {
        errprintf ("invalid argument to --percentiles '%s'", str);
        exit (EXIT_FAILURE);
    }

    return k - 1;
}

static int read_ints (int *levels, char *str)
{
    char * cursor;
    char * endp = str;
    int k = 0;

    do {
        cursor = endp;
        levels[k] = (int) strtol(cursor, &endp, 10);
        if (cursor == endp && (*endp == ',' || *endp == ';')) {
            endp++;
            continue;
        }
        k++;
    } while (cursor != endp);

    // not end of string: error
    while (*cursor != '\0') {
        if (!isspace(*cursor)) {
            errprintf ("invalid argument to --levels '%s'", str);
            exit (EXIT_FAILURE);
        }
        cursor++;
    }

    // no number: error
    if (k == 1) {
        errprintf ("invalid argument to --levels '%s'", str);
        exit (EXIT_FAILURE);
    }

    return k - 1;
}

static void
eaf_print (eaf_t **eaf, int nobj, int nlevels,
           FILE *coord_file, FILE *indic_file, FILE *diff_file)
{
    int k;
    for (k = 0; k < nlevels; k++) {
        eaf_print_attsurf (eaf[k], nobj, coord_file, indic_file, diff_file);
        if (coord_file)
            fprintf (coord_file, "\n");
        else if (indic_file)
            fprintf(indic_file, "\n");
        else if (diff_file)
            fprintf(diff_file, "\n");
    }
}

void read_input_data (const char *filename, objective_t **data_p,
                      int *nobjs_p, int **cumsizes_p, int *nsets_p)
{
    int error = read_objective_t_data (filename, data_p, nobjs_p, cumsizes_p, nsets_p);
    switch (error) {
      case 0: /* No error */
          break;
      case READ_INPUT_FILE_EMPTY:
      case READ_INPUT_WRONG_INITIAL_DIM:
          break;
      case ERROR_FOPEN:
      case ERROR_CONVERSION:
      case ERROR_COLUMNS:
          exit (EXIT_FAILURE);
      default:
          exit (EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    bool verbose_flag = true;
    bool best_flag   = false;
    bool median_flag = false;
    bool worst_flag  = false;
    bool polygon_flag  = false;
    const char *coord_filename = NULL;
    const char *indic_filename = NULL;
    const char *diff_filename  = NULL;
    FILE *coord_file = stdout;
    FILE *indic_file = NULL;
    FILE *diff_file = NULL;

    int option;
    int longopt_index;
    /* see the man page for getopt_long for an explanation of these fields */
    static const char short_options[] = "hVvqbmwl:p:o:i::d::P";
    static struct option long_options[] = {
        {"help",       no_argument,       NULL, 'h'},
        {"version",    no_argument,       NULL, 'V'},
        {"verbose",    no_argument,       NULL, 'v'},
        {"quiet",      no_argument,       NULL, 'q'},
        {"best",       no_argument,       NULL, 'b'},
        {"median",     no_argument,       NULL, 'm'},
        {"worst",      no_argument,       NULL, 'w'},
        {"output",     required_argument, NULL, 'o'},
        /* The optional_argument must be together with the parameter
           such as -iFILE or -i=FILE, otherwise it will be considered
           an input file. */
        {"indices",    optional_argument, NULL, 'i'},
        {"diff",       optional_argument, NULL, 'd'},
        {"percentile", required_argument, NULL, 'p'},
        {"level",      required_argument, NULL, 'l'},
        {"polygons",   no_argument,       NULL, 'P'},
        {NULL, 0, NULL, 0} /* marks end of list */
    };
#define MAX_LEVELS 50
    int *level = malloc(MAX_LEVELS * sizeof(int));
    int nlevels = 0;
    double *percentile = malloc(MAX_LEVELS * sizeof(double));
    int npercentiles = 0;

    set_program_invocation_short_name(argv[0]);

    while (0 < (option = getopt_long(argc, argv, short_options,
                                     long_options, &longopt_index))) {
        switch (option) {
        case 'l':
            assert(nlevels < MAX_LEVELS);
            nlevels += read_ints(level + nlevels, optarg);
            break;

        case 'p':
            assert(npercentiles < MAX_LEVELS);
            npercentiles += read_doubles(percentile + npercentiles, optarg);
            break;

        case 'o':
	    if (!strcmp(optarg,"-")) {
		coord_file = stdout;
                coord_filename = NULL;
            } else
                coord_filename = optarg;
	    break;

	case 'i':
            if (!optarg) {
                indic_file = stdin; /* Overwrite with coord_file later. */
                indic_filename = NULL;
            }
	    else if (!strcmp(optarg,"-")) {
		indic_file = stdout;
                indic_filename = NULL;
            }
            else {
                indic_filename = optarg;
            }
	    break;

	case 'd':
            if (!optarg) {
                diff_file = stdin; /* Overwrite with coord_file later. */
                diff_filename = NULL;
            }
	    else if (!strcmp(optarg,"-")) {
		diff_file = stdout;
                diff_filename = NULL;
            }
            else {
                diff_filename = optarg;
            }
	    break;

        case 'b':
            best_flag = true;
            break;

        case 'm':
            median_flag = true;
            break;

        case 'w':
            worst_flag = true;
            break;

        case 'P':
            polygon_flag = true;
            break;

        case 'q': // --quiet
            verbose_flag = false;
            break;

        case 'v': // --verbose
            verbose_flag = true;
            break;

        case 'V': // --version
            version();
            exit(EXIT_SUCCESS);

        case 'h': // --help
            usage();
            exit(EXIT_SUCCESS);

        case '?':
            // getopt prints an error message right here
            fprintf(stderr, "Try `%s --help' for more information.\n",
                    program_invocation_short_name);
            exit(EXIT_FAILURE);

        default:
            abort ();
        }
    }

    objective_t *data = NULL;
    int* cumsizes = NULL;
    int nobj = 0, nruns = 0, k;
    if (optind < argc) {
        for (k = optind; k < argc; k++) {
            if (strcmp (argv[k],"-"))
                read_input_data (argv[k], &data, &nobj,
                                 &cumsizes, &nruns);
            else
                read_input_data (NULL, &data, &nobj, &cumsizes, &nruns);
        }
    } else
        read_input_data (NULL, &data, &nobj, &cumsizes, &nruns);


    if (coord_filename) {
        coord_file = fopen (coord_filename, "w");
        if (NULL == coord_file) {
            errprintf ("%s: %s", coord_filename, strerror (errno));
            exit (EXIT_FAILURE);
        }
    }

    /* indic_file is neither NULL nor stdout and no filename was
       given, use same settings as for --output.  */
    if (indic_file && indic_file != stdout && !indic_filename) {
        indic_file = coord_file;
        indic_filename = coord_filename;
    }
    /* If a different filename as for --output was given, open it.  */
    if (indic_filename
        && (!coord_filename || strcmp (indic_filename, coord_filename))) {
        indic_file = fopen (indic_filename, "w");
        if (NULL == indic_file) {
            errprintf ("%s: %s", indic_filename, strerror (errno));
            exit (EXIT_FAILURE);
        }
    }

    /* diff_file is neither NULL nor stdout and no filename was given,
       use same settings as for --output.  */
    if (diff_file && diff_file != stdout && !diff_filename) {
        diff_file = coord_file;
        diff_filename = coord_filename;
    }
    /* If a different filename as for --output and --indices was
       given, open it.  */
    if (diff_filename
        && (!coord_filename || strcmp (diff_filename, coord_filename))
        && (!indic_filename || strcmp (diff_filename, indic_filename))) {
        diff_file = fopen (diff_filename, "w");
        if (NULL == diff_file) {
            errprintf ("%s: %s", diff_filename, strerror (errno));
            exit(EXIT_FAILURE);
        }
    }

    if (best_flag) {
        level[0] = 1;
        nlevels = 1;
    }
    else if (median_flag) {
        level[0] = percentile2level (50, nruns);
        nlevels = 1;
    }
    else if (worst_flag) {
        level[0] = nruns;
        nlevels = 1;
    }
    else if (npercentiles > 0) {
        nlevels = npercentiles;
        for (k = 0; k < nlevels; k++) {
            if (percentile[k] <= 0) {
                errprintf ("arg to --percentile must be higher than 0.\n");
                exit (EXIT_FAILURE);
            } else if (percentile[k] > 100) {
                errprintf ("arg to --percentile must be <= 100.\n");
                exit (EXIT_FAILURE);
            }
            level[k] = percentile2level(percentile[k], nruns);
        }
    }
    else if (nlevels > 0) {
        level = realloc (level, nlevels * sizeof(int));
        for (k = 0; k < nlevels; k++) {
            if (level[k] <= 0) {
                errprintf ("arg to --level must be higher than 0.\n");
                exit (EXIT_FAILURE);
            } else if (level[k] > nruns) {
                errprintf ("arg to --level must not be higher than number of"
                           " approximation sets (%d).\n", nruns);
                exit (EXIT_FAILURE);
            }
        }
    }
    else if (nlevels == 0) {
        nlevels = nruns;
        level = realloc (level, nlevels * sizeof(int));
        for (k = 0; k < nruns; k++) {
            level[k] = k + 1;
        }
    }

    if (verbose_flag) {
        fprintf (stderr, "# objectives (%d): --\n", nobj);
        fprintf (stderr, "# sets: %d\n", nruns);
        fprintf (stderr, "# points: %d\n", cumsizes[nruns - 1]);
        fprintf (stderr, "# calculating levels:");
        for (k = 0; k < nlevels; k++)
            fprintf (stderr, " %d", level[k]);
        fprintf (stderr, "\n");
    }

    eaf_t **eaf = attsurf (data, nobj, cumsizes, nruns, level, nlevels);

    if (polygon_flag) {
        eaf_print_polygon (coord_file, eaf, nobj, nlevels);
        fclose (coord_file);
    } else {
        eaf_print (eaf, nobj, nlevels,
                   coord_file, indic_file, diff_file);
        fclose (coord_file);
        if (indic_file && indic_file != coord_file)
            fclose (indic_file);
        if (diff_file && diff_file != coord_file && diff_file != indic_file)
            fclose (diff_file);
    }

    free(level);
    free(data);
    free(cumsizes);
    eaf_free(eaf, nlevels);
    return 0;
}
