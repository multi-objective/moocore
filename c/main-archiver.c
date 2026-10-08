/*************************************************************************

 archiver: Archive one point at a time.

 ---------------------------------------------------------------------

                       Copyright (c) 2026
             Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.

 ----------------------------------------------------------------------

 Relevant literature:

*************************************************************************/
#include "config.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>  // for getopt()
#include <getopt.h> // for getopt_long()

#include "io.h"
#include "archive.h"
#define CMDLINE_COPYRIGHT_YEARS "2026"
#define CMDLINE_AUTHORS "Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>\n"
#include "cmdline.h"

static bool union_flag = false;
static bool always_nd_tree_flag = false;
static int verbose_flag = 1;
static char *suffix = NULL;

static void usage(void)
{
    printf("\n"
           "Usage: %s [OPTIONS] [FILE...]\n\n", program_invocation_short_name);

    printf(
"Calculate the hypervolume of each input set of each FILE. \n"
"With no FILE, or when FILE is -, read standard input.\n\n"

"Options:\n"
OPTION_HELP_STR
OPTION_VERSION_STR
" -v, --verbose       print some information (time, maximum, etc).          \n"
"     --nd-tree       Always use ND-Tree for any dimension.\n"
OPTION_UNION_STR
" -s, --suffix=STRING Create an output file for each input file by appending\n"
"                     this suffix. This is ignored when reading from stdin. \n"
"                     If missing, output is sent to stdout.                 \n"
"\n");
}

// FIXME: Add options to print quality metrics after archiving each input point.
static bool
read_and_archive(FILE * input)
{
    dimension_t dim = 0;
    double point[MOOCORE_DIMENSION_MAX+1];
    // Read the first line as the dimension/template point.
    int status = fread_next_double_point(input, point, &dim);
    if (status != 1)
        fatal_error("reading the first point");
    if (dim < 2)
        fatal_error("At least 2 dimensions are required");

    const NDTreeConfig config = { .max_children = 0, .max_bucket_size = 0, .allow_duplicates = true};
    const bool use_treap = (dim == 2 && !always_nd_tree_flag);
    UnboundedArchive * archive = use_treap
        ? archive_treap_new()
        : archive_ndtree_new(dim, config);

    size_t points_read = 0;
    // Read point until EOF or empty line.
    do  {
        if (archive_add(archive, point, /*x=*/NULL) < 0)
            fatal_error("Memory error!");
        // FIXME: check that the archive remains nondominated
        points_read++;
        status = fread_next_double_point(input, point, &dim);
    } while (status == 1 || (union_flag && status == 0));

    if (status != READ_INPUT_FILE_EMPTY && status != 0)
        fatal_error("reading point from input");

    // We completed one data set
    size_t total_points = archive_total_size(archive);
    // Print result
    if (verbose_flag >= 2) {
        fprintf(stdout,
                "# Points read: %zu\n"
                "# Total points: %zu\n"
                "# Unique points: %zu\n",
                points_read,
                total_points,
                archive_unique_size(archive));
    }
    double * z_list = malloc(sizeof(*z_list) * dim * total_points);
    archive_get_contents(archive, z_list, NULL);
    // FIXME: Add an option to sort the output.
    int n_points = (int) total_points;
    // FIXME: write_sets should take size_t * or unsigned *;
    write_sets(stdout, z_list, dim, &n_points, 1, /*prefix=*/NULL);
    free(z_list);
    if (verbose_flag >= 2)
        archive_print_stats(stderr, archive);
    archive_free(archive);
    return (status == 0);
}


int main(int argc, char *argv[])
{
    // See the man page for getopt_long for an explanation of these fields.
    static const char short_options[] = "hVvqUs:N";
    static const struct option long_options[] = {
        {"help",       no_argument,       NULL, 'h'},
        {"version",    no_argument,       NULL, 'V'},
        {"verbose",    no_argument,       NULL, 'v'},
        {"quiet",      no_argument,       NULL, 'q'},
        {"union",      no_argument,       NULL, 'U'},
        {"suffix",     required_argument, NULL, 's'},
        {"nd-tree",    no_argument,       NULL, 'N'},
        {NULL, 0, NULL, 0} /* marks end of list */
    };

    set_program_invocation_short_name(argv[0]);

    int opt; /* it's actually going to hold a char.  */
    int longopt_index;
    while (0 < (opt = getopt_long (argc, argv, short_options,
                                   long_options, &longopt_index))) {
        switch (opt) {
          case 'N': // --nd-tree
              always_nd_tree_flag = true;
              break;

          case 's': // --suffix
              suffix = optarg;
              break;

          case 'q': // --quiet
              verbose_flag = 0;
              break;

          case 'v': // --verbose
              verbose_flag = 2;
              break;

          case 'U': // --union
              union_flag = true;
              break;

          default:
              default_cmdline_handler(opt);
        }
    }

    int numfiles = argc - optind;
    if (numfiles < 1) { // Read stdin.
        while (read_and_archive(stdin)) {
            fprintf(stdout, "\n");
        }
    }
    return EXIT_SUCCESS;
}
