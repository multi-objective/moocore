/*************************************************************************

 nondominated:

 ---------------------------------------------------------------------

                       Copyright (c) 2007-2008, 2025
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


******************************************************************************/
#include "config.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> // for isspace()
#include <float.h>
#include <unistd.h> // for getopt()
#include <getopt.h> // for getopt_long()
#include <math.h>   // for log10()

#include "nondominated.h"

#define READ_INPUT_WRONG_INITIAL_DIM_ERRSTR \
    "either -o, --obj, -u, --upper or -l, --lower"
#include "cmdline.h"


static int verbose_flag = false;
static bool union_flag = false;
static bool check_flag = true;
static bool filter_flag = false;
static bool normalise_flag = false;
static bool force_bounds_flag = false;
static const char * suffix = "_dat";

static void
usage(void)
{
    printf("\n"
           "Usage:\n"
           "       %s [OPTIONS] [FILES] \n"
           "       %s [OPTIONS] < [INPUT] > [OUTPUT]\n\n",
           program_invocation_short_name, program_invocation_short_name);

    printf("Obtain information and perform some operations on the nondominated "
           "sets "
           "given as input. \n\n"

           "Options:\n" OPTION_HELP_STR OPTION_VERSION_STR
           " -v, --verbose       print some extra information;                 "
           "        \n" OPTION_QUIET_STR OPTION_NOCHECK_STR
               OPTION_OBJ_STR OPTION_MAXIMISE_STR
           " -u, --upper-bound POINT defines an upper bound to check, e.g. "
           "\"10 5 30\";\n"
           " -l, --lower-bound POINT defines a lower bound to check;\n"
           " -U, --union         consider each file as a whole approximation "
           "set,      \n"
           "                     (by default, approximation sets are separated "
           "by an   \n"
           "                     empty line within a file);                    "
           "        \n"
           " -s, --suffix=STRING suffix to add to output files. Default is "
           "\"%s\".     \n"
           "                     The empty string means overwrite the input "
           "file.      \n"
           "                     This is ignored when reading from stdin "
           "because output\n"
           "                     is sent to stdout.                            "
           "        \n"
           "\n"
           " The following options OVERWRITE output files:\n"
           " -a, --agree=<max|min> transform objectives so all are maximised "
           "(or       \n"
           "                       minimised). See also the option --obj.      "
           "        \n"
           " -f, --filter        check and filter out dominated points;        "
           "        \n"
           " -b, --force-bound   remove points that do not satisfy the bounds; "
           "        \n"
           " -n, --normalise RANGE normalise all objectives to a range, e.g., "
           "\"1 2\". \n"
           "                       If bounds are given with -l and -u, they "
           "are used   \n"
           "                       for the normalisation.                      "
           "        \n"
           " -L, --log=[1|0]...  specify whether each objective should be "
           "transformed  \n"
           "                     to logarithmic scale (1) or not (0).          "
           "        \n"
           "\n",
           suffix);
}

static bool
read_range(char * str, double * lower, double * upper)
{
    char * endp;

    *lower = strtod(str, &endp);
    if (str == endp) return false;
    str = endp;

    *upper = strtod(str, &endp);
    if (str == endp) return false;
    str = endp;

    // not end of string: error
    while (*str != '\0') {
        if (!isspace(*str)) return false;
        str++;
    }

    return true;
}

static inline bool
any_less_than(const double * a, const double * b, int nobj)
{
    for (int d = 0; d < nobj; d++)
        if (a[d] < b[d]) return true;

    return false;
}

static void
logarithm_scale(double * points, int nobj, int size, const bool * logarithm)
{
    int k, d;

    assert(logarithm);

    for (d = 0; d < nobj; d++) {
        if (!logarithm[d]) continue;
        for (k = 0; k < size; k++)
            points[k * nobj + d] = log10(points[k * nobj + d]);
    }
}

static bool
force_bounds(double * points, int nobj, int * cumsizes, int nsets,
             const double * lbound, const double * ubound)
{
    int n, n2, k;
    int size = cumsizes[nsets - 1];
    bool * outbounds = malloc(sizeof(bool) * size);
    int outbounds_found = -1;
    for (n = size - 1; n >= 0; n--) {
        if (any_less_than(&points[n * nobj], lbound, nobj) ||
            any_less_than(ubound, &points[n * nobj], nobj)) {
            outbounds[n] = true;
            outbounds_found = n;
        } else
            outbounds[n] = false;
    }

    if (outbounds_found < 0) {
        if (verbose_flag >= 2) fprintf(stderr, "# out of bounds: 0\n");
        free(outbounds);
        return false;
    }

    int * ssizes = malloc(sizeof(int) * nsets);
    ssizes[0] = cumsizes[0];
    for (k = 1; k < nsets; k++) ssizes[k] = cumsizes[k] - cumsizes[k - 1];

    /* Find the set of the first out-of-bounds point.  */
    for (k = 0; outbounds_found >= cumsizes[k]; k++);

    /* Delete it.  */
    ssizes[k]--;

    /* Delete the rest of them.  */
    for (n = outbounds_found, n2 = outbounds_found + 1; k < nsets; k++) {
        while (n2 < cumsizes[k]) {
            if (outbounds[n2]) {
                n2++;
                ssizes[k]--;
            } else {
                memcpy(&points[n * nobj], &points[n2 * nobj],
                       sizeof(double) * nobj);
                n++, n2++;
            }
        }
    }

    if (verbose_flag >= 2) fprintf(stderr, "# out of bounds: %d\n", n2 - n);

    cumsizes[0] = ssizes[0];
    for (k = 1; k < nsets; k++) cumsizes[k] = ssizes[k] + cumsizes[k - 1];

    free(ssizes);
    free(outbounds);
    return true;
}

static bool
check_nondominated(const char * filename, const double * points, int nobj,
                   const int * cumsizes, int nruns, const signed char * minmax,
                   const signed char agree, bool ** nondom_p)
{
    bool * nondom = nondom_p ? *nondom_p : NULL;
    bool free_nondom = false;
    if (nondom == NULL) {
        free_nondom = true;
        nondom = nondom_init(cumsizes[nruns - 1]);
    }

    bool first_time = true;
    bool dominated_found = false;
    int filename_len = (int)MAX(strlen(filename), strlen("filename"));
    for (int n = 0, cumsize = 0; n < nruns; cumsize = cumsizes[n], n++) {
        size_t old_size = cumsizes[n] - cumsize;
        size_t new_size =
            find_nondominated_set_agree(&points[nobj * cumsize], nobj, old_size,
                                        minmax, agree, &nondom[cumsize]);

        if (verbose_flag >= 2) {
            if (first_time) {
                fprintf(stderr, "# %*s\tset\tsize\tnondom\tdom\n",
                        filename_len - 2, "filename");
                first_time = false;
            }
            fprintf(stderr, "%-*s\t%d\t%zu\t%zu\t%zd\n", filename_len, filename,
                    n + 1, old_size, new_size, old_size - new_size);
        } else if (verbose_flag && new_size < old_size) {
            if (first_time) {
                fprintf(stderr, "%-*s\tset\tdom\n", filename_len, "filename");
                first_time = false;
            }
            fprintf(stderr, "%-*s\t%d\t%zd dominated\n", filename_len, filename,
                    n + 1, old_size - new_size);
        }

        if (new_size < old_size) {
            dominated_found = true;
        } else if (new_size > old_size) { /* This can't happen.  */
            fatal_error("%s:%d: a bug happened: new_size > old_size!\n",
                        __FILE__, __LINE__);
        }
    }

    if (nondom_p)
        *nondom_p = nondom;
    else if (free_nondom)
        free(nondom);

    return dominated_found;
}

static void
print_file_info(FILE * stream, const char * filename, int nobj,
                const signed char * minmax)
{
    /* Print some info about input files.  */
    fprintf(stream, "# file: %s\n", filename);
    fprintf(stream, "# objectives (%d): ", nobj);
    for (int n = 0; n < nobj; n++) {
        fprintf(stream, "%c",
                (minmax[n] < 0) ? '-' : ((minmax[n] > 0) ? '+' : 'i'));
    }
    fprintf(stream, "\n");
}

static void
print_output_header(FILE * stream, const char * filename, int nobj,
                    const signed char * minmax, signed char agree,
                    double lrange, double urange, const double * lbound,
                    const double * ubound, const bool * logarithm)
{
    print_file_info(stream, filename, nobj, minmax);
    fprintf(stream, "# agree: %s\n",
            (agree < 0) ? "min" : ((agree > 0) ? "max" : "no"));

    if (logarithm) {
        fprintf(stream, "# logarithm: ");
        for (int n = 0; n < nobj; n++) {
            fprintf(stream, "%c", logarithm[n] ? '1' : '0');
        }
        fprintf(stream, "\n");
    }

    if (normalise_flag)
        fprintf(stream,
                "# range: " point_printf_format " " point_printf_format "\n",
                lrange, urange);

    fprintf(stream, "# lower bound: ");
    vector_fprintf(stream, lbound, nobj);
    fprintf(stream, "\n");
    fprintf(stream, "# upper bound: ");
    vector_fprintf(stream, ubound, nobj);
    fprintf(stream, "\n");
}

static void
print_input_info(FILE * stream, const char * filename, int nobj,
                 const int * cumsizes, int nruns, const signed char * minmax,
                 const double * minimum, const double * maximum)
{
    print_file_info(stream, filename, nobj, minmax);
    fprintf(stream, "# sets: %d\n", nruns);
    fprintf(stream, "# sizes: %d", cumsizes[0]);
    for (int n = 1; n < nruns; n++)
        fprintf(stream, ", %d", cumsizes[n] - cumsizes[n - 1]);
    fprintf(stream, "\n# points: %d\n", cumsizes[nruns - 1]);
    fprintf(stream, "# minimum: ");
    vector_fprintf(stream, minimum, nobj);
    fprintf(stream, "\n# maximum: ");
    vector_fprintf(stream, maximum, nobj);
    fprintf(stream, "\n");
}

static bool
process_file(const char * filename, const signed char * minmax, int * nobj_p,
             signed char agree, double lrange, double urange, double * lbound,
             double * ubound, double ** minimum_p, double ** maximum_p,
             bool check_minimum, bool check_maximum, bool maximise_all_flag,
             const bool * logarithm)
{
    bool * nondom = NULL;
    bool logarithm_flag = false;

    double * points = NULL;
    int nobj = *nobj_p;
    int * cumsizes = NULL;
    int nsets = 0;

    handle_read_data_error(
        read_double_data(filename, &points, &nobj, &cumsizes, &nsets),
        filename);
    ASSUME(nobj > 1 && nobj < 128);

    if (!filename) filename = stdin_name;

    if (union_flag) {
        cumsizes[0] = cumsizes[nsets - 1];
        nsets = 1;
    }

    /* Default minmax if not set yet.  */
    bool free_minmax = false;
    if (minmax == NULL) {
        minmax = maximise_all_flag ? minmax_maximise((dimension_t)nobj)
                                   : minmax_minimise((dimension_t)nobj);
        free_minmax = true;
    }

    double * minimum = NULL;
    double * maximum = NULL;
    data_bounds(&minimum, &maximum, points, nobj, cumsizes[nsets - 1]);

    if (verbose_flag >= 2)
        print_input_info(stderr, filename, nobj, cumsizes, nsets, minmax,
                         minimum, maximum);

    if (lbound == NULL)
        lbound = minimum;
    else if (check_minimum && !force_bounds_flag &&
             any_less_than(minimum, lbound, nobj)) {
        errprintf("%s: found vector smaller than lower bound:", filename);
        vector_fprintf(stderr, minimum, nobj);
        fprintf(stderr, "\n");
        exit(EXIT_FAILURE);
    }

    if (ubound == NULL)
        ubound = maximum;
    else if (check_maximum && !force_bounds_flag &&
             any_less_than(ubound, maximum, nobj)) {
        errprintf("%s: found vector larger than upper bound:", filename);
        vector_fprintf(stderr, maximum, nobj);
        fprintf(stderr, "\n");
        exit(EXIT_FAILURE);
    }

    if (force_bounds_flag) {
        force_bounds(points, nobj, cumsizes, nsets, lbound, ubound);
    }

    double * log_lbound = NULL;
    double * log_ubound = NULL;

    if (logarithm) {
        log_lbound = malloc(sizeof(double) * nobj);
        log_ubound = malloc(sizeof(double) * nobj);

        memcpy(log_lbound, lbound, sizeof(double) * nobj);
        memcpy(log_ubound, ubound, sizeof(double) * nobj);

        for (int d = 0; d < nobj; d++) {
            if (!logarithm[d]) continue;
            log_lbound[d] = log10(lbound[d]);
            log_ubound[d] = log10(ubound[d]);
            logarithm_flag = true;
        }

        if (logarithm_flag) {
            lbound = log_lbound;
            ubound = log_ubound;
            logarithm_scale(points, nobj, cumsizes[nsets - 1], logarithm);
        }
    }

    if (agree)
        agree_objectives(points, nobj, cumsizes[nsets - 1], minmax, agree);

    if (normalise_flag)
        normalise(points, nobj, cumsizes[nsets - 1], minmax, agree, lrange,
                  urange, lbound, ubound);

    bool dominated_found = false;
    /* Check sets.  */
    if (check_flag || filter_flag)
        dominated_found =
            check_nondominated(filename, points, nobj, cumsizes, nsets, minmax,
                               agree, filter_flag ? &nondom : NULL);

    if (verbose_flag >= 2)
        fprintf(stderr, "# nondominated: %s\n",
                dominated_found ? "FALSE" : "TRUE");

    /* Write out nondominated sets.  */
    if (filter_flag || agree || normalise_flag || force_bounds_flag ||
        logarithm_flag) {

        const char * outfilename = "<stdout>";
        FILE * outfile = stdout;
        if (filename != stdin_name) {
            outfilename = m_strcat(filename, suffix);
            outfile = fopen(outfilename, "w");
            if (outfile == NULL)
                fatal_error("%s: %s\n", outfilename, strerror(errno));
        }
        if (verbose_flag)
            print_output_header(outfile, filename, nobj, minmax, agree, lrange,
                                urange, lbound, ubound, logarithm);

        if (filter_flag && dominated_found)
            write_sets_filtered(outfile, points, nobj, cumsizes, nsets, nondom);
        else
            write_sets(outfile, points, nobj, cumsizes, nsets);

        if (verbose_flag)
            fprintf(stderr, "# %s -> %s\n", filename, outfilename);
        if (outfile != stdout) {
            fclose(outfile);
            free((void *)outfilename);
        }
    }

    free(points);
    free(cumsizes);
    if (free_minmax) free((void *)minmax);
    if (nondom) free(nondom);
    if (log_lbound) free(log_lbound);
    if (log_ubound) free(log_ubound);

    *minimum_p = minimum;
    *maximum_p = maximum;
    *nobj_p = nobj;

    if (verbose_flag >= 2) fprintf(stderr, "#\n");

    return dominated_found;
}

int
main(int argc, char * argv[])
{
    signed char agree = 0;
    double lower_range = 0.0;
    double upper_range = 0.0;
    double * lower_bound = NULL;
    double * upper_bound = NULL;

    const signed char * minmax = NULL;
    bool maximise_all_flag = false;
    const bool * logarithm = NULL;
    int nobj = 0;

    /* see the man page for getopt_long for an explanation of these fields */
    static const char short_options[] = "hVvqfo:a:n:u:l:Us:b";
    static const struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'V'},
        {"verbose", no_argument, NULL, 'v'},
        {"quiet", no_argument, NULL, 'q'},
        {"no-check", no_argument, NULL, 'c'},
        {"filter", no_argument, NULL, 'f'},
        {"force-bounds", no_argument, NULL, 'b'},
        {"obj", required_argument, NULL, 'o'},
        {"agree", required_argument, NULL, 'a'},
        {"maximise", no_argument, NULL, 'M'},
        {"maximize", no_argument, NULL, 'M'},
        {"normalise", required_argument, NULL, 'n'},
        {"upper-bound", required_argument, NULL, 'u'},
        {"lower-bound", required_argument, NULL, 'l'},
        {"union", no_argument, NULL, 'U'},
        {"suffix", required_argument, NULL, 's'},
        {"log", required_argument, NULL, 'L'},
        {NULL, 0, NULL, 0} /* marks end of list */
    };
    set_program_invocation_short_name(argv[0]);

    int opt; /* it's actually going to hold a char */
    int longopt_index;
    while (0 < (opt = getopt_long(argc, argv, short_options, long_options,
                                  &longopt_index))) {
        switch (opt) {
        case 'q': // --quiet
            verbose_flag = 0;
            break;

        case 'v': // --verbose
            verbose_flag = 2;
            break;

        case 'c': // --no-check
            check_flag = false;
            break;

        case 'f': // --filter
            filter_flag = true;
            check_flag = true;
            break;

        case 'b': // --force-bounds
            force_bounds_flag = true;
            break;

        case 'U': // --union
            union_flag = true;
            break;

        case 'o': // --obj
            minmax = parse_cmdline_minmax(minmax, optarg, &nobj);
            break;

        case 'a': // --agree
            if (!strcmp(optarg, "max"))
                agree = 1;
            else if (!strcmp(optarg, "min"))
                agree = -1;
            else {
                fatal_error("invalid argument '%s' for -a, --agree"
                            ", it should be either \'min\' or \'max\'\n",
                            optarg);
            }
            break;

        case 'M': // --maximise
            maximise_all_flag = true;
            break;

        case 'n': // --normalise
            normalise_flag = true;
            if (!read_range(optarg, &lower_range, &upper_range)) {
                fatal_error("invalid range '%s' for -n, --normalise"
                            ", use for example -n \"1 2\"\n",
                            optarg);
            } else if (lower_range >= upper_range) {
                fatal_error("lower range must be smaller than upper range for "
                            "-n, --normalise\n");
            }
            break;

        case 'u': // --upper-bound
            upper_bound = robust_read_point(optarg, &nobj,
                                            "invalid upper bound point '%s'");
            break;

        case 'l': // --lower-bound
            lower_bound = robust_read_point(optarg, &nobj,
                                            "invalid lower bound point '%s'");
            break;

        case 's': // --suffix
            suffix = optarg;
            break;

        case 'L': // --log
            logarithm = read_bitvector(optarg, &nobj);
            if (logarithm == NULL)
                fatal_error("invalid argument to --log '%s'", optarg);
            break;

        default:
            default_cmdline_handler(opt);
        }
    }

    if (lower_bound && upper_bound &&
        any_less_than(upper_bound, lower_bound, nobj)) {
        fatal_error("upper bound must be higher than lower bound.");
    }

    int numfiles = argc - optind;
    double * minimum = NULL;
    double * maximum = NULL;

    if (numfiles <= 1) { /* <= 0 means: No input files: read stdin.  */
        bool dominated_found =
            process_file((numfiles == 1) ? argv[optind] : NULL, minmax, &nobj,
                         agree, lower_range, upper_range, lower_bound,
                         upper_bound, &minimum, &maximum,
                         /*check_minimum=*/true, /*check_maximum=*/true,
                         maximise_all_flag, logarithm);
        free(minimum);
        free(maximum);
        free((void *)minmax);
        return filter_flag ? EXIT_SUCCESS : dominated_found;
    }

    int k;
    bool dominated_found = false;
    if (!lower_bound || !upper_bound) {
        /* Calculate the bounds among all input files.  */
        minimum = NULL, maximum = NULL;
        for (k = 0; k < numfiles; k++)
            file_bounds(argv[optind + k], &minimum, &maximum, &nobj);

        k = 0;
    } else {
        /* If the bounds were given, initialize minimum and maximum.  */
        dominated_found = process_file(
            argv[optind], minmax, &nobj, agree, lower_range, upper_range,
            lower_bound, upper_bound, &minimum, &maximum,
            /*check_minimum=*/true, /*check_maximum=*/true, maximise_all_flag,
            logarithm);
        k = 1;
    }

    for (; k < numfiles; k++) {
        double * tmp_maximum = NULL;
        double * tmp_minimum = NULL;

        if (process_file(argv[optind + k], minmax, &nobj, agree, lower_range,
                         upper_range, (lower_bound) ? lower_bound : minimum,
                         (upper_bound) ? upper_bound : maximum, &tmp_minimum,
                         &tmp_maximum, lower_bound != NULL, upper_bound != NULL,
                         maximise_all_flag, logarithm))
            dominated_found = true;

        /* If the bounds were given, the real minimum and maximum
           must be calculated as we process the files.  */
        if (lower_bound && upper_bound)
            for (int n = 0; n < nobj; n++) {
                if (minimum[n] > tmp_minimum[n]) minimum[n] = tmp_minimum[n];
                if (maximum[n] < tmp_maximum[n]) maximum[n] = tmp_maximum[n];
            }
        free(tmp_minimum);
        free(tmp_maximum);
    }
    if (verbose_flag) {
        printf("# Total files: %d\n", numfiles);
        printf("# Total minimum:");
        vector_printf(minimum, nobj);
        printf("\n");
        printf("# Total maximum:");
        vector_printf(maximum, nobj);
        printf("\n");
        printf("# Nondominated: %s\n", dominated_found ? "FALSE" : "TRUE");
    }
    free(minimum);
    free(maximum);
    return filter_flag ? EXIT_SUCCESS : dominated_found;
}
