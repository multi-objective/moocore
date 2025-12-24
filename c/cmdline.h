#ifndef CMDLINE_H
#define CMDLINE_H

#include "io.h"

#ifndef CMDLINE_COPYRIGHT_YEARS
#define CMDLINE_COPYRIGHT_YEARS "2007-2025"
#endif

#ifndef CMDLINE_AUTHORS
#define CMDLINE_AUTHORS "Manuel Lopez-Ibanez  <manuel.lopez-ibanez@manchester.ac.uk>\n"
#endif

#define OPTION_HELP_STR \
    " -h, --help          print this summary and exit;\n"
#define OPTION_VERSION_STR \
    "     --version       print version number (and compilation flags) and exit;\n"
#define OPTION_OBJ_STR \
    " -o, --obj=[+|-]...  specify whether each objective should be minimised (-)\n" \
    "                     or maximised (+). By default all are minimised;\n"
#define OPTION_QUIET_STR \
    " -q, --quiet         print as little as possible;\n"
#define OPTION_MAXIMISE_STR \
    "     --maximise      all objectives must be maximised;\n"
#define OPTION_NOCHECK_STR \
    "     --no-check      do not check nondominance of sets (faster but unsafe);\n"
#define OPTION_SEED_STR \
    " -S, --seed=SEED     Seed of the random number generator (positive integer).\n"

#include <stdbool.h>
#include <ctype.h> // for isspace()

extern char *program_invocation_short_name;

static void version(void)
{
#if defined(DEBUG) && DEBUG >= 1
#define DEBUG_LEVEL_STR " [DEBUG=" MOOCORE_STRINGIFY_MACRO(DEBUG) "]"
#else
#define DEBUG_LEVEL_STR ""
#endif
#ifdef MARCH
#define OPTIMISED_FOR_STR " (optimised for "MARCH")"
#else
#define OPTIMISED_FOR_STR ""
#endif
    printf("%s version " VERSION OPTIMISED_FOR_STR DEBUG_LEVEL_STR
           "\n\n", program_invocation_short_name);
    printf(
"Copyright (C) " CMDLINE_COPYRIGHT_YEARS "\n" CMDLINE_AUTHORS "\n"
"This is free software, and you are welcome to redistribute it under certain\n"
"conditions.  See the GNU General Public License for details. There is NO   \n"
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
"\n"        );
}

#ifndef READ_INPUT_WRONG_INITIAL_DIM_ERRSTR
#define READ_INPUT_WRONG_INITIAL_DIM_ERRSTR "-r, --reference"
#endif

static inline void
handle_read_data_error(int err, const char * filename)
{
    switch (err) {
      case 0: /* No error */
          return;

      case READ_INPUT_FILE_EMPTY:
          if (!filename)
              filename = stdin_name;
          errprintf("%s: no input data.", filename);
          exit(EXIT_FAILURE);

      case READ_INPUT_WRONG_INITIAL_DIM:
          errprintf("check the argument of " READ_INPUT_WRONG_INITIAL_DIM_ERRSTR  ".");
          /* fall-through */
      default:
          exit(EXIT_FAILURE);
    }
}

static void
robust_read_double_data(const char * filename, double ** restrict data_p,
                        int * restrict nobjs_p, int ** restrict cumsizes_p,
                        int * restrict nsets_p, bool union_flag)
{
    handle_read_data_error(
        read_double_data(filename, data_p, nobjs_p, cumsizes_p, nsets_p),
        filename);
    assert(*nsets_p > 0);
    assert(*nobjs_p > 1 && *nobjs_p < 256);
    if (union_flag) {
        (*cumsizes_p)[0] = (*cumsizes_p)[*nsets_p - 1];
        *nsets_p = 1;
    }
}

/*
   FILENAME: input filename. If NULL, read stdin.

   REFERENCE: reference point. If NULL, caculate it from the
   input file.

   NOBJ_P: pointer to number of objectives. If NULL, calculate it from
   input file.

*/

static inline size_t
read_reference_set(const char * filename, double ** restrict reference,
                   int * restrict nobj_p)
{
    int tmp_nobj = 0;
    int * cumsizes = NULL;
    int nruns = 0;
    assert(filename != NULL);
    robust_read_double_data(filename, reference, &tmp_nobj, &cumsizes, &nruns,
                            /* union_flag=*/true);
    size_t reference_size = cumsizes[0];
    free(cumsizes);
    if (reference == NULL || reference_size == 0)
        fatal_error("invalid reference set '%s", optarg);

    if (*nobj_p == 0) {
        *nobj_p = tmp_nobj;
    } else if (tmp_nobj != *nobj_p) {
        fatal_error("number of objectives in --obj (%d) and reference set (%d) do not match", *nobj_p, tmp_nobj);
    }
    return reference_size;
}

/* TODO: Handle "1 NAN 3", so NAN means use the minimum/maximum for this
   dimension.  */
static inline double *
read_point(char * str, int * nobj)
{
    int k = 0, size = 10;
    double * point = malloc(size * sizeof(*point));
    char * endp = str;
    char * cursor;
    do {
        cursor = endp;
        if (k == size) {
            size += 10;
            point = realloc(point, size * sizeof(*point));
        }
        point[k] = strtod(cursor, &endp);
        k++;
    } while (cursor != endp);

    // not end of string: error
    while (*cursor != '\0') {
        if (!isspace(*cursor)) {
            free (point);
            return NULL;
        }
        cursor++;
    }
    // no number: error
    if (k == 1) {
        free(point);
        return NULL;
    }
    if (k < size)
        point = realloc(point, k * sizeof(*point));
    *nobj = k - 1;
    return point;
}

static inline double *
robust_read_point(char * restrict optarg, int * restrict nobj, const char * restrict errmsg)
{
    double * point = read_point(optarg, nobj);
    if (point == NULL)
        fatal_error(errmsg, optarg);
    return point;
}

static inline char * m_strcat(const char * a, const char * b)
{
    size_t dest_len = strlen(a) + strlen(b) + 1;
    char * dest = malloc(sizeof(*dest) * dest_len);
    if (unlikely(dest == NULL))
        return NULL;
    strcpy(dest, a);
    strcat(dest, b);
    return dest;
}

static inline FILE *
fopen_outfile(const char ** outfilename, const char * restrict filename,
              const char * restrict suffix)
{
    FILE * outfile = stdout;
    if (filename != stdin_name && suffix) {
        *outfilename = m_strcat(filename, suffix);
        outfile = fopen(*outfilename, "w");
        if (outfile == NULL)
            fatal_error("%s: %s\n", *outfilename, strerror(errno));
    }
    return outfile;
}

static inline void
fclose_outfile(FILE * outfile, const char * filename, const char * outfilename,
               bool verbose_flag)
{
    if (outfilename) {
        if (verbose_flag)
            fprintf(stderr, "# %s -> %s\n", filename, outfilename);
        fclose(outfile);
        free((void *) outfilename);
    }
}

#include <math.h> // INFINITY

static void
data_bounds(double * restrict * restrict minimum_p, double * restrict * restrict maximum_p,
            const double * restrict data, size_t rows, dimension_t nobj)
{
    ASSUME(nobj > 1);
    double * minimum = *minimum_p;
    if (minimum == NULL) {
        minimum = malloc(nobj * sizeof(*minimum));
        for (dimension_t k = 0; k < nobj; k++)
            minimum[k] = INFINITY;
        *minimum_p = minimum;
    }
    double * maximum = *maximum_p;
    if (maximum == NULL) {
        maximum = malloc(nobj * sizeof(*maximum));
        for (dimension_t k = 0; k < nobj; k++)
            maximum[k] = -INFINITY;
        *maximum_p = maximum;
    }

    for (size_t r = 0, n = 0; r < rows; r++) {
        for (dimension_t k = 0; k < nobj; k++, n++) {
            if (maximum[k] < data[n])
                maximum[k] = data[n];
            if (minimum[k] > data[n])
                minimum[k] = data[n];
        }
    }
}

_attr_maybe_unused static void
file_bounds(const char * filename, double * restrict * restrict maximum_p,
            double * restrict * restrict minimum_p, int * restrict dim_p)
{
    double * data = NULL;
    int * cumsizes = NULL;
    int nruns = 0;
    assert(filename != NULL);
    robust_read_double_data(filename, &data, dim_p, &cumsizes, &nruns,
                            /* union_flag=*/true);
    size_t size = cumsizes[0];
    free(cumsizes);
    data_bounds(minimum_p, maximum_p, data, size, (dimension_t) *dim_p);
    free(data);
}

static inline const char * str_is_default(bool flag)
{
    return flag ? "(default)" : "";
}

static inline void set_program_invocation_short_name(char * s)
{
    if (program_invocation_short_name == NULL || program_invocation_short_name[0] == '\0')
        program_invocation_short_name = s;
}

static inline const int *
parse_cmdline_minmax(const int * restrict minmax, const char * restrict optarg, int * restrict nobj_p)
{
    int tmp_nobj = 0, nobj = *nobj_p;

    if (minmax != NULL)
        free((void *) minmax);
    minmax = read_minmax(optarg, &tmp_nobj);
    if (minmax == NULL) {
        errprintf ("invalid argument '%s' for -o, --obj"
                   ", it should be a sequence of '+' or '-'\n", optarg);
        exit(EXIT_FAILURE);
    }
    if (nobj == 0) {
        nobj = tmp_nobj;
    } else if (tmp_nobj != nobj) {
        errprintf ("number of objectives in --obj (%d) and reference set (%d) do not match", tmp_nobj, nobj);
        exit(EXIT_FAILURE);
    }
    *nobj_p = nobj;
    return minmax;
}

static void usage(void);

static inline void default_cmdline_handler(int opt)
{
    switch (opt) {
      case 'V': // --version
          version();
          exit(EXIT_SUCCESS);
      case '?':
          // getopt prints an error message right here
          fprintf(stderr, "Try `%s --help' for more information.\n",
                  program_invocation_short_name);
          exit(EXIT_FAILURE);
      case 'h': // --help
          usage();
          exit(EXIT_SUCCESS);

      default: // LCOV_EXCL_LINE # nocov
          unreachable();
    }
}

_attr_const_func
static inline const char * bool2str(bool b) { return b ? "TRUE" : "FALSE"; }

#endif // !CMDLINE_H
