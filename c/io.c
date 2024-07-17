/*****************************************************************************

 I/O functions

 ---------------------------------------------------------------------

                            Copyright (c) 2005-2008
                    Carlos M. Fonseca <cmfonsec@dei.uc.pt>
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

 TODO: things that may or may not improve reading performance.

  * different values for PAGE_SIZE.

  * reading whole lines and then sscanf the line.

  * mmaping the input file (or a big chunk of few megabytes), then
    read, then unmmap.

*****************************************************************************/

#include <stdio.h>
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
static inline void skip_trailing_whitespace (FILE * instream)
{
    ignore_unused_result (fscanf (instream, "%*[ \t\r]"));
}

static inline int fscanf_newline(FILE * instream)
{
    char newline[2];
    return fscanf (instream, "%1[\n]", newline);
}

/* skip full lines starting with # */
static inline int skip_comment_line (FILE * instream)
{
    char newline[2];
    if (!fscanf (instream, "%1[#]%*[^\n]", newline))
        /* and whitespace */
        skip_trailing_whitespace(instream);
    return fscanf_newline(instream);
}


#define objective_t int
#define objective_t_scanf_format "%d"
#define read_objective_t_data read_int_data
#include "io_priv.h"
#undef objective_t
#undef objective_t_scanf_format
#undef read_objective_t_data

#define objective_t double
#define objective_t_scanf_format "%lf"
#define read_objective_t_data read_double_data
#include "io_priv.h"
#undef objective_t
#undef objective_t_scanf_format
#undef read_objective_t_data

/* Convenience wrapper to read_double_data used by Python's moocore.  */
int
read_datasets(const char * filename, double **data_p, int *ncols_p, int *datasize_p)
{
    double * data = NULL;
    int * cumsizes = NULL;
    int nsets = 0, nobjs = 0;
    int error = read_double_data(filename, &data, &nobjs, &cumsizes, &nsets);
    if (error) {
        return error;
    }
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
        newdata[i * ncols + nobjs] = (double) set;
        i++;
        if (i == cumsizes[set - 1])
            set++;
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
vector_fprintf (FILE *stream, const double * vector, int size)
{
    fprintf (stream, point_printf_format, vector[0]);
    for (int k = 1; k < size; k++)
        fprintf (stream, point_printf_sep "" point_printf_format, vector[k]);
}

void
vector_printf (const double *vector, int size)
{
    vector_fprintf (stdout, vector, size);
}

void
vector_int_fprintf (FILE *stream, const int * vector, int size)
{
    for (int k = 0; k < size; k++)
        fprintf (stream, "%d ", vector[k]);
}

void
vector_int_printf (const int *vector, int size)
{
    vector_int_fprintf (stdout, vector, size);
}

int
write_sets (FILE *outfile, const double *data, int ncols,
            const int *cumsizes, int nruns)
{
    int size = 0;
    int set = 0;

    for (set = 0; set < nruns; set++) {
        for (; size < cumsizes[set]; size++) {
            vector_fprintf (outfile, &data[ncols * size], ncols);
            fprintf (outfile, "\n");
        }
        fprintf (outfile, "\n");
    }
    return 0;
}

int
write_sets_filtered (FILE *outfile, const double *data, int ncols,
                     const int *cumsizes, int nruns, const bool *write_p)
{
    int size = 0;
    for (int set = 0; set < nruns; set++) {
        for (; size < cumsizes[set]; size++) {
            if (write_p[size]) {
                vector_fprintf (outfile, &data[ncols * size], ncols);
                fprintf (outfile, "\n");
            }
        }
        fprintf (outfile, "\n");
    }
    return 0;
}
#endif // R_PACKAGE
