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

#include <stdbool.h>
#include <ctype.h> // for isspace()

extern char *program_invocation_short_name;

static void version(void)
{
#ifdef MARCH
#define OPTIMISED_FOR_STR " (optimised for "MARCH")"
#else
#define OPTIMISED_FOR_STR ""
#endif
    printf("%s version " VERSION OPTIMISED_FOR_STR
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
handle_read_data_error (int err, const char *filename)
{
    switch (err) {
      case 0: /* No error */
          break;

      case READ_INPUT_FILE_EMPTY:
          if (!filename)
              filename = stdin_name;
          errprintf ("%s: no input data.", filename);
          exit (EXIT_FAILURE);

      case READ_INPUT_WRONG_INITIAL_DIM:
          errprintf ("check the argument of " READ_INPUT_WRONG_INITIAL_DIM_ERRSTR  ".");
          /* fall-through */
      default:
          exit (EXIT_FAILURE);
    }
}

/*
   FILENAME: input filename. If NULL, read stdin.

   REFERENCE: reference point. If NULL, caculate it from the
   input file.

   NOBJ_P: pointer to number of objectives. If NULL, calculate it from
   input file.

*/

static inline int
read_reference_set (double **reference_p, const char *filename, int *nobj_p)
{
    double *reference = NULL;
    int *cumsizes = NULL;
    int nruns = 0;
    int nobj = *nobj_p;
    int reference_size;

    handle_read_data_error(
        read_double_data (filename, &reference, &nobj, &cumsizes, &nruns),
        filename);
    if (!filename)
        filename = stdin_name;
    reference_size = cumsizes[nruns - 1];
    free(cumsizes);
    *nobj_p = nobj;
    *reference_p = reference;
    return reference_size;
}

/* TODO: Handle "1 NAN 3", so NAN means use the minimum/maximum for this
   dimension.  */
_attr_maybe_unused static double *
read_point(char * str, int * nobj)
{
    int k = 0, size = 10;
    double * point = malloc(size * sizeof(double));
    char * endp = str;
    char * cursor;
    do {
        cursor = endp;
        if (k == size) {
            size += 10;
            point = realloc(point, size * sizeof(double));
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
        point = realloc(point, k * sizeof(double));
    *nobj = k - 1;
    return point;
}

#include <math.h> // INFINITY

static void
data_bounds(double **minimum_p, double **maximum_p,
            const double *data, int nobj, int rows)
{
    ASSUME(nobj > 1);
    ASSUME(nobj <= 32);
    int k;

    double * minimum = *minimum_p;
    if (minimum == NULL) {
        minimum = malloc (nobj * sizeof(double));
        for (k = 0; k < nobj; k++)
            minimum[k] = INFINITY;
        *minimum_p = minimum;
    }
    double * maximum = *maximum_p;
    if (maximum == NULL) {
        maximum = malloc (nobj * sizeof(double));
        for (k = 0; k < nobj; k++)
            maximum[k] = -INFINITY;
        *maximum_p = maximum;
    }

    for (int r = 0, n = 0; r < rows; r++) {
        for (k = 0; k < nobj; k++, n++) {
            if (maximum[k] < data[n])
                maximum[k] = data[n];
            if (minimum[k] > data[n])
                minimum[k] = data[n];
        }
    }
}

_attr_maybe_unused static void
file_bounds (const char *filename, double **maximum_p, double **minimum_p,
             int *dim_p)
{
    double *data = NULL;
    int *cumsizes = NULL;
    int nruns = 0;
    handle_read_data_error(
        read_double_data(filename, &data, dim_p, &cumsizes, &nruns),
        filename);
    data_bounds(minimum_p, maximum_p, data, *dim_p, cumsizes[nruns - 1]);
    free(data);
    free(cumsizes);
}

static inline char * m_strcat(const char * a, const char * b)
{
    size_t dest_len = strlen(a) + strlen(b) + 1;
    char *dest = malloc (sizeof(char) * dest_len);
    if (dest == NULL) return NULL;
    strcpy (dest, a);
    strcat (dest, b);
    return dest;
}

static inline const char *str_is_default(bool flag)
{
    return flag ? "(default)" : "";
}

static inline void set_program_invocation_short_name(char *s)
{
    if (program_invocation_short_name == NULL || program_invocation_short_name[0] == '\0')
        program_invocation_short_name = s;
}

static inline const signed char *
parse_cmdline_minmax(const signed char * minmax, const char *optarg, int *nobj_p)
{
    int tmp_nobj = 0, nobj = *nobj_p;

    if (minmax != NULL)
        free((void *) minmax);
    minmax = read_minmax (optarg, &tmp_nobj);
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
      default:
          unreachable();
    }
}
#endif
