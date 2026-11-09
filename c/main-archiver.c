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
#include "nds_forest.h"

#define CMDLINE_COPYRIGHT_YEARS "2026"
#define CMDLINE_AUTHORS "Manuel Lopez-Ibanez <manuel.lopez-ibanez@manchester.ac.uk>\n"
#include "cmdline.h"

static bool union_flag = false;
static bool always_nd_tree_flag = false;
static bool nds_flag = false;
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
"     --nds           NDS-archive (currently NDS-Forest).\n"
"     --strategy=     NDS-Forest insertion strategy: linear, front-binary, \n"
"                     previous-rank, zero-then-binary, dominance-guided-binary.\n"
OPTION_UNION_STR
" -s, --suffix=STRING Create an output file for each input file by appending\n"
"                     this suffix. This is ignored when reading from stdin. \n"
"                     If missing, output is sent to stdout.                 \n"
"\n");
}

static bool
check_no_missing_points(NDSForest *archiver, size_t expected_n)
{
    size_t nfronts = nds_forest_len(archiver);
    size_t total_points = 0;
    for (size_t i = 0; i < nfronts; i++)
        total_points += nds_forest_front_total_size(archiver, i);
    return total_points == expected_n;
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

    const NDTreeConfig config = { .max_children = 0, .max_bucket_size = 0};
    const bool use_treap = (dim == 2 && !always_nd_tree_flag);
    const bool keep_duplicates = true, x_is_null = true;
    UnboundedArchive * archive = use_treap
        ? archive_treap_new(keep_duplicates, x_is_null)
        : archive_ndtree_new(dim, config, keep_duplicates, x_is_null);

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

static bool
read_and_nds_archive(FILE * input, int strategy)
{
    dimension_t dim = 0;
    double point[MOOCORE_DIMENSION_MAX+1];
    // Read the first line as the dimension/template point.
    int status = fread_next_double_point(input, point, &dim);
    if (status != 1)
        fatal_error("reading the first point");
    if (dim < 2)
        fatal_error("At least 2 dimensions are required");

    const bool use_treap = (dim == 2 && !always_nd_tree_flag);
    const bool x_is_null = true;
    const bool do_timing = (verbose_flag >= 2);
    NDSForest * archive = use_treap
        ? nds_forest_treap_new(strategy, x_is_null, do_timing)
        : nds_forest_ndtree_new(dim, strategy, /*max_children=*/0, /*max_bucket_size=*/0, x_is_null, do_timing);

    size_t points_read = 0;
    // Read point until EOF or empty line.
    do  {
        if (!nds_forest_add(archive, point, /*x=*/NULL))
            fatal_error("Memory error!");

        points_read++;
        if (!check_no_missing_points(archive, points_read))
            fatal_error("missing points when adding point %zu =[%g, %g, ...]\n", points_read, point[0], point[1]);

        status = fread_next_double_point(input, point, &dim);
    } while (status == 1 || (union_flag && status == 0));

    if (status != READ_INPUT_FILE_EMPTY && status != 0)
        fatal_error("reading point from input");

    // We completed one data set
    size_t nfronts = nds_forest_len(archive);
    // Print result
    if (verbose_flag >= 2) {
        fprintf(stdout,
                "# Query time: %-20.12g\n"
                "# Update time: %-20.12g\n"
                "# Total points: %zu\n"
                "# Unique points: %zu\n"
                "# Number of fronts: %zu\n",
                archive->query_time, archive->update_time,
                nds_forest_total_size(archive),
                nds_forest_unique_size(archive),
                nfronts);
        fprintf(stdout, "# Front total sizes: ");
        if (nfronts > 0) {
            fprintf(stdout, "%zu", nds_forest_front_total_size(archive, 0));
            for (size_t i = 1; i < nfronts; i++)
                fprintf(stdout, ", %zu", nds_forest_front_total_size(archive, i));
            fprintf(stdout, "\n");
        }
        fprintf(stdout, "# Front unique sizes: ");
        if (nfronts > 0) {
            fprintf(stdout, "%zu", nds_forest_front_unique_size(archive, 0));
            for (size_t i = 1; i < nfronts; i++)
                fprintf(stdout, ", %zu", nds_forest_front_unique_size(archive, i));
            fprintf(stdout, "\n");
        }
    }

    size_t total_points = 0;
    for (size_t i = 0; i < nfronts; i++) {
        size_t npoint = nds_forest_front_total_size(archive, i);
        total_points += npoint;
        // FIXME: Realloc
        double * z_list = malloc(sizeof(*z_list) * dim * npoint);
        nds_forest_front_get_contents(archive, i, z_list, NULL);
        // FIXME: Add an option to sort the output.
        int n_points = (int) npoint;
        // FIXME: write_sets should take size_t * or unsigned *;
        char prefix[256];
        snprintf(prefix, 255, "%zu ", i);
        write_sets(stdout, z_list, dim, &n_points, 1, prefix);
        free(z_list);
    }
    nds_forest_free(archive);
    assert(total_points == points_read);
    return (status == 0);
}


int main(int argc, char *argv[])
{
    // See the man page for getopt_long for an explanation of these fields.
    static const char short_options[] = "hVvqUs:NnS:";
    static const struct option long_options[] = {
        {"help",       no_argument,       NULL, 'h'},
        {"version",    no_argument,       NULL, 'V'},
        {"verbose",    no_argument,       NULL, 'v'},
        {"quiet",      no_argument,       NULL, 'q'},
        {"union",      no_argument,       NULL, 'U'},
        {"suffix",     required_argument, NULL, 's'},
        {"nd-tree",    no_argument,       NULL, 'N'},
        {"nds",        no_argument,       NULL, 'n'},
        {"strategy",   required_argument, NULL, 'S'},
        {NULL, 0, NULL, 0} /* marks end of list */
    };

    set_program_invocation_short_name(argv[0]);

    int nds_forest_insertion_strategy = LINEAR;

    int opt; /* it's actually going to hold a char.  */
    int longopt_index;
    while (0 < (opt = getopt_long (argc, argv, short_options,
                                   long_options, &longopt_index))) {
        switch (opt) {
          case 'S': // --strategy
              nds_forest_insertion_strategy = parse_nds_forest_strategy(optarg);
              if (nds_forest_insertion_strategy == UNKNOWN_STRATEGY)
                  fatal_error("unknown value (%s) of --strategy=", optarg);
              break;

          case 'n': // --nds
              nds_flag = true;
              break;

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
        if (nds_flag) {
            while (read_and_nds_archive(stdin, nds_forest_insertion_strategy)) {
                fprintf(stdout, "\n");
            }
        } else {
            while (read_and_archive(stdin)) {
                fprintf(stdout, "\n");
            }
        }
    }
    return EXIT_SUCCESS;
}
