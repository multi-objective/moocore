#include <stdlib.h>
#include <stdio.h>
#include <string.h> /* for strerror() */
#include <errno.h> /* for errno  */

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)
#define objective_t_str STR(objective_t)

#define PAGE_SIZE 4096           /* allocate one page at a time      */
#define DATA_INC (PAGE_SIZE/sizeof(objective_t))

/*
 * Read an array of objective values from a stream.  This function may
 * be called repeatedly to add data to an existing data set.
 *
 *  nobjs : number of objectives, also the number of columns.
 */
int
read_objective_t_data (const char *filename, objective_t **data_p,
                       int *nobjs_p, int **cumsizes_p, int *nsets_p)
{
    int nobjs = *nobjs_p;        /* number of objectives (and columns).  */
    int *cumsizes = *cumsizes_p; /* cumulative sizes of data sets.       */
    int nsets    = *nsets_p;     /* number of data sets.                 */
    objective_t *data = *data_p;

    int errorcode = 0;
    FILE *instream;

    if (filename == NULL) {
        instream = stdin;
        filename = stdin_name; /* used to diagnose errors.  */
    } else if (NULL == (instream = fopen (filename,"rb"))) {
        errprintf ("%s: %s", filename, strerror (errno));
        return ERROR_FOPEN;
    }

    int ntotal;			/* the current element of (*datap) */
    int datasize;
    int sizessize;
    if (nsets == 0) {
        ntotal = 0;
        sizessize = 0;
        datasize = 0;
    } else {
        ntotal = nobjs * cumsizes[nsets - 1];
        sizessize = (int) ((nsets - 1) / DATA_INC + 1) * DATA_INC;
        datasize  = (int) ((ntotal - 1) / DATA_INC + 1) * DATA_INC;
    }

    /* if size is equal to zero, this is equivalent to free().
       That is, reinitialize the data structures.  */
    cumsizes = realloc (cumsizes, sizessize * sizeof(int));
    data = realloc (data, datasize * sizeof(objective_t));

    /* skip over leading whitespace, comments and empty lines.  */
    int retval;			/* return value for fscanf */
    int column = 0,
        line = 0;
    do {
        line++;
        /* skip full lines starting with # */
        retval = skip_comment_line (instream);
    } while (retval == 1);

    if (retval == EOF) { /* faster than !feof() */
        errorcode = READ_INPUT_FILE_EMPTY;
        goto read_data_finish;
    }

    do {
        /* beginning of data set */
	if (nsets == sizessize) {
            sizessize += DATA_INC;
            cumsizes = realloc(cumsizes, sizessize * sizeof(int));
        }

	cumsizes[nsets] = (nsets == 0) ? 0 : cumsizes[nsets - 1];

        do {
            /* beginning of row */
	    column = 0;

	    do {
                /* new column */
                column++;
                objective_t number;
                if (fscanf (instream, objective_t_scanf_format, &number) != 1) {
                    char buffer[64];
                    if (fscanf (instream, "%60[^ \t\r\n]", buffer) != 1) {
                        errprintf ("%s: line %d column %d: "
                                   "read error or unexpected end of file",
                                   filename, line, column);
                    } else {
                        errprintf ("%s: line %d column %d: "
                                   "could not convert string `%s' to %s (format: %s)",
                                   filename, line, column, buffer, objective_t_str, objective_t_scanf_format);
                    }
                    errorcode = ERROR_CONVERSION;
                    goto read_data_finish;
                }

                if (ntotal == datasize) {
                    datasize += DATA_INC;
                    data = realloc (data, datasize * sizeof(objective_t));
                }
                data[ntotal] = number;
                ntotal++;
                DEBUG2(fprintf(stderr, "%s:%d:%d(%d) %d (set %d) = "
                               point_printf_format "\n",
                               filename, line, column, nobjs,
                               cumsizes[nsets], nsets, (double)number));

                /* skip possible trailing whitespace */
                skip_trailing_whitespace(instream);
                retval = fscanf_newline(instream);
	    } while (retval == 0);

	    if (!nobjs)
		nobjs = column;
            else if (column == nobjs)
                ; /* OK */
            else if (cumsizes[0] == 0) { /* just finished first row.  */
                errprintf ("%s: line %d: input has dimension %d"
                           " while previous data has dimension %d",
                           filename,line, column, nobjs);
                errorcode = READ_INPUT_WRONG_INITIAL_DIM;
                goto read_data_finish;
            } else {
                errprintf ("%s: line %d has different number of columns (%d)"
                           " from first row (%d)\n",
                           filename, line, column, nobjs);
                errorcode = ERROR_COLUMNS;
                goto read_data_finish;
 	    }
	    cumsizes[nsets]++;

            /* look for an empty line */
            line++;
            retval = skip_comment_line (instream);
	} while (retval == 0);

	nsets++; /* new data set */
        DEBUG2(fprintf (stderr, "%s: set %d, read %d rows\n",
                    filename, nsets, cumsizes[nsets - 1]));
        /* skip over successive empty lines */
        do {
            line++;
            retval = skip_comment_line (instream);
        } while (retval == 1);

    } while (retval != EOF); /* faster than !feof() */

    /* adjust to real size (saves memory but probably slower).  */
    cumsizes = realloc (cumsizes, nsets * sizeof(int));
    data = realloc (data, ntotal * sizeof(objective_t));

read_data_finish:

    *nobjs_p = nobjs;
    *nsets_p = nsets;
    *cumsizes_p = cumsizes;
    *data_p = data;

    if (instream != stdin)
        fclose(instream);

    return errorcode;
}

#undef PAGE_SIZE
#undef DATA_INC
#undef QUOTE
#undef STR
