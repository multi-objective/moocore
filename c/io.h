#ifndef EAF_INPUT_OUTPUT_H
#define EAF_INPUT_OUTPUT_H

#include "common.h"

static const char stdin_name[] = "<stdin>";

// FIXME: Should this be %-16.15g ?
#define point_printf_format "% 17.16g"
#define point_printf_sep    "\t"

/* Error codes for read_data.  */
enum ERROR_READ_DATA { READ_INPUT_FILE_EMPTY = -1,
                       READ_INPUT_WRONG_INITIAL_DIM = -2,
                       ERROR_FOPEN = -3,
                       ERROR_CONVERSION = -4,
                       ERROR_COLUMNS = -5,
};

int
read_int_data (const char *filename, int **data_p,
               int *nobjs_p, int **cumsizes_p, int *nsets_p);
int
read_double_data (const char *filename, double **data_p,
                  int *nobjs_p, int **cumsizes_p, int *nsets_p);

#ifndef R_PACKAGE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

void vector_fprintf (FILE *stream, const double * vector, int size);
void vector_printf (const double *vector, int size);
void vector_int_fprintf (FILE *stream, const int * vector, int size);
void vector_int_printf (const int *vector, int size);
int read_datasets(const char * filename, double **data_p, int *ncols_p, int *datasize_p);
int write_sets (FILE *outfile, const double *data, int ncols,
                const int *cumsizes, int nruns);
int write_sets_filtered (FILE *outfile, const double *data, int ncols,
                         const int *cumsizes, int nruns,
                         const bool *write_p);

static inline const signed char *
read_minmax (const char *str, int *nobj_p)
{
    signed char * minmax;
    size_t i;
    size_t nobj = (size_t) *nobj_p;

    if (str == NULL) { /* Default all minimised.  */
        assert (nobj > 0);
        minmax = malloc (sizeof(signed char) * nobj);
        for (i = 0; i < nobj; i++)
            minmax[i] = -1;
        return minmax;
    }

    size_t len = strlen(str);
    bool all_ignored = true;
    minmax = malloc (sizeof(signed char) * MAX(len, nobj));
    for (i = 0; i < len; i++) {
        switch (str[i])
        {
        case '+':
            minmax[i] = 1;
            all_ignored = false;
            break;
        case '-':
            minmax[i] = -1;
            all_ignored = false;
            break;
        case '0':
        case 'i':
            minmax[i] = 0;
            break;
        default: /* something unexpected was found */
            return NULL;
            break;
        }
    }

    if (all_ignored == true) {
        warnprintf ("all objectives ignored because of --obj=%s\n", str);
        exit (EXIT_SUCCESS);
    }
    // FIXME: How to adjust minmax dynamically according to the number of objectives?
    if (len < nobj) { // Cycle
        for (i = 0; i < (nobj - len); i++) {
            minmax[len + i] = minmax[i];
        }
    }
    *nobj_p = (int) len;
    return minmax;
}

static inline const bool *
read_bitvector (const char *str, int *nobj_p)
{
    bool * vec;
    size_t i;
    size_t nobj = *nobj_p;

    if (str == NULL) { /* Default all false.  */
        assert (nobj > 0);
        vec = malloc (sizeof(bool) * nobj);
        for (i = 0; i < nobj; i++)
            vec[i] = false;
        return vec;
    }

    size_t len = strlen (str);
    vec = malloc (sizeof(bool) * len);
    for (i = 0; i < len; i++) {
        switch (str[i]) {
          case '1':
              vec[i] = true;
              break;
          case '0':
              vec[i] = false;
              break;
          default: /* something unexpected was found */
              return NULL;
        }
    }
    *nobj_p = (int) len;
    return vec;
}

#endif // R_PACKAGE
#endif // EAF_INPUT_OUTPUT_H
