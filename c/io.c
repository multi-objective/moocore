/*****************************************************************************

 I/O functions

 ---------------------------------------------------------------------

                         Copyright (c) 2005-2008, 2025
                    Carlos M. Fonseca <cmfonsec@dei.uc.pt>
          Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ----------------------------------------------------------------------

 TODO: things that may or may not improve reading performance.

  * different values for PAGE_SIZE.

  * reading whole lines and then sscanf the line.

  * mmaping the input file (or a big chunk of few megabytes), then
    read, then unmmap.

*****************************************************************************/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include "common.h"
#include "io.h"

/* FIXME: Do we need to handle the following weird files? */
/*
                   fscanf (instream, "%*[ \t]");
                   retval = fscanf (instream, "%1[\r\n]", newline);
                   // We do not consider that '\r\n' starts a new set.
                   if (retval == 1 && newline[0] == '\r')
		    fscanf (instream, "%*[\n]");
                */
static inline int
skip_trailing_whitespace(FILE * instream)
{
    int c;
    do {
        c = fgetc(instream);
    } while (c == ' ' || c == '\t' || unlikely(c == '\r'));

    if (c == '\n') return 1;
    if (unlikely(c == EOF)) return EOF;
    ungetc(c, instream); // Push the non-whitespace character back
    return 0;
}

/* skip full lines starting with # */
static inline int
skip_comment_line(FILE * instream)
{
    int c = skip_trailing_whitespace(instream);
    if (c == 1) return 1;
    if (unlikely(c == EOF)) return EOF;

    c = fgetc(instream);
    if (unlikely(c == '#')) {
        do { // Skip the rest of the line
            c = fgetc(instream);
            if (c == '\n') return 1;
            if (unlikely(c == EOF)) return EOF;
        } while (true);
    }

    if (c == '\n') return 1;
    if (unlikely(c == EOF)) return EOF;

    ungetc(c, instream); // Not a comment, push back
    return 0;
}

int
fread_double(FILE * instream, double * number)
{
    int c;
    // Skip leading whitespace.
    do {
        c = fgetc(instream);
    } while (c == ' ' || c == '\t' || c == '\r');

    if (unlikely(c == EOF)) return EOF;

    char buffer[128];
    size_t i = 0;
    do {
        buffer[i] = (char)c;
        i++;
        c = fgetc(instream);
        if (isspace(c) || unlikely(c == EOF)) break;
        if (unlikely(i == sizeof(buffer) - 1)) return 0; // Number is too long.
    } while (true);

    if (c == '\n') ungetc(c, instream); // Push back newlines

    buffer[i] = '\0';
    char * endptr;
    *number = strtod(buffer, &endptr);
    if (unlikely(endptr == buffer)) return 0; // Invalid conversion

    return 1; // Success !
}

int
fread_int(FILE * instream, int * number)
{
    int c;
    // Skip leading whitespace.
    do {
        c = fgetc(instream);
    } while (c == ' ' || c == '\t' || c == '\r');

    if (c == EOF) return EOF;

    char buffer[64];
    size_t i = 0;
    do {
        buffer[i] = (char)c;
        i++;
        c = fgetc(instream);
        if (isspace(c) || c == EOF) break;
        if (i == sizeof(buffer) - 1) return 0; // Number is too long.
    } while (true);

    if (c == '\n') ungetc(c, instream); // Push back newlines.

    buffer[i] = '\0';

    char * endptr;
    long value = strtol(buffer, &endptr, /*base=*/10);
    if (endptr == buffer) return 0; // Invalid conversion.

    if (value < INT_MIN || value > INT_MAX) return 0; // Out of range.

    *number = (int)value;
    return 1; // Success !
}

#define objective_t int
#define fread_objective_t fread_int
#define read_objective_t_data read_int_data
#include "io_priv.h"
#undef objective_t
#undef fread_objective_t
#undef read_objective_t_data

#define objective_t double
#define fread_objective_t fread_double
#define read_objective_t_data read_double_data
#include "io_priv.h"
#undef objective_t
#undef fread_objective_t
#undef read_objective_t_data

/* Convenience wrapper to read_double_data used by Python's moocore.  */
int
read_datasets(const char * filename, double ** data_p, int * ncols_p,
              int * datasize_p)
{
    double * data = NULL;
    int * cumsizes = NULL;
    int nsets = 0, nobjs = 0;
    int error = read_double_data(filename, &data, &nobjs, &cumsizes, &nsets);
    if (unlikely(error)) return error;

    int ncols = nobjs + 1; // For the column 'set'
    int nrows = cumsizes[nsets - 1];
    int datasize = ncols * nrows * sizeof(double);
    double * newdata = malloc(datasize);
    int set = 1;
    int i = 0;
    while (i < nrows) {
        for (int j = 0; j < nobjs; j++) {
            newdata[i * ncols + j] = data[i * nobjs + j];
        }
        newdata[i * ncols + nobjs] = (double)set;
        i++;
        if (i == cumsizes[set - 1]) set++;
    }
    free(data);
    free(cumsizes);

    *data_p = newdata;
    *ncols_p = ncols;
    *datasize_p = datasize;
    return 0;
}

#ifndef R_PACKAGE
void
vector_fprintf(FILE * stream, const double * vector, int size)
{
    ASSUME(size > 0);
    fprintf(stream, point_printf_format, vector[0]);
    for (int k = 1; k < size; k++)
        fprintf(stream, point_printf_sep "" point_printf_format, vector[k]);
}

void
vector_printf(const double * vector, int size)
{
    vector_fprintf(stdout, vector, size);
}

void
vector_int_fprintf(FILE * stream, const int * vector, int size)
{
    ASSUME(size > 0);
    for (int k = 0; k < size; k++) fprintf(stream, "%d ", vector[k]);
}

void
vector_int_printf(const int * vector, int size)
{
    vector_int_fprintf(stdout, vector, size);
}

int
write_sets(FILE * outfile, const double * data, int ncols, const int * cumsizes,
           int nruns)
{
    int size = 0;
    ASSUME(nruns > 0);
    for (int set = 0; set < nruns; set++) {
        ASSUME(cumsizes[set] >= 0);
        if (set > 0) fprintf(outfile, "\n");
        for (; size < cumsizes[set]; size++) {
            vector_fprintf(outfile, &data[ncols * size], ncols);
            fprintf(outfile, "\n");
        }
    }
    return 0;
}

int
write_sets_filtered(FILE * outfile, const double * data, int ncols,
                    const int * cumsizes, int nruns, const bool * write_p)
{
    int size = 0;
    ASSUME(nruns > 0);
    for (int set = 0; set < nruns; set++) {
        ASSUME(cumsizes[set] >= 0);
        if (set > 0) fprintf(outfile, "\n");
        for (; size < cumsizes[set]; size++) {
            if (write_p[size]) {
                vector_fprintf(outfile, &data[ncols * size], ncols);
                fprintf(outfile, "\n");
            }
        }
    }
    return 0;
}
#endif // R_PACKAGE
