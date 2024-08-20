#ifndef CMDLINE_H
#define CMDLINE_H

#include <stdbool.h>
#include "io.h"

#ifndef CMDLINE_COPYRIGHT_YEARS
#define CMDLINE_COPYRIGHT_YEARS "2007-2023"
#endif

#ifndef CMDLINE_AUTHORS
#define CMDLINE_AUTHORS "Manuel Lopez-Ibanez  <manuel.lopez-ibanez@manchester.ac.uk>\n"
#endif

#define OPTION_HELP_STR \
    " -h, --help          print this summary and exit;                          \n"
#define OPTION_VERSION_STR \
    "     --version       print version number and exit;                        \n"
#define OPTION_OBJ_STR \
    " -o, --obj=[+|-]...  specify whether each objective should be minimised (-)\n" \
    "                     or maximised (+). By default all are minimised;       \n"
#define OPTION_QUIET_STR \
    " -q, --quiet          print as little as possible                           \n"

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

    int err = read_double_data (filename, &reference, &nobj, &cumsizes, &nruns);
    if (!filename) filename = stdin_name;
    handle_read_data_error (err, filename);
    assert (nruns == 1);
    reference_size = cumsizes[0];

    free (cumsizes);
    *nobj_p = nobj;
    *reference_p = reference;
    return reference_size;
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
#endif
