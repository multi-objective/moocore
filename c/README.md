**moocore**: Core Mathematical Functions for Multi-Objective Optimization
=========================================================================

<!-- badges: start -->
[ [**C library**][c-moocore-homepage] ] [ [GitHub][c-moocore-github] ] [![Build status][c-build-badge]][c-build-link] [![Testsuite status][testsuite-badge]][testsuite-link]
<!-- badges: end -->

**Contributors:**
    [Manuel López-Ibáñez](https://lopez-ibanez.eu),
    [Carlos M. Fonseca](https://eden.dei.uc.pt/~cmfonsec/),
    [Luís Paquete](https://eden.dei.uc.pt/~paquete/),
    Andreia P. Guerreiro.



Introduction
============

This is the C library and associated command-line tools of the [**moocore** project](https://github.com/multi-objective/moocore). This README describes just this component. There is also a [`moocore` Python package][py-moocore-homepage] and a [`moocore` R package][r-moocore-homepage].


The [**moocore** project](https://github.com/multi-objective/moocore/) collects fast implementations of core mathematical functions and algorithms for multi-objective optimization and makes them available to different programming languages via similar interfaces. These functions include:

 * Identifying and filtering dominated vectors.
 * Quality metrics such as (weighted) hypervolume, epsilon, IGD, etc.
 * Computation of the Empirical Attainment Function. The empirical attainment
   function (EAF) describes the probabilistic distribution of the outcomes
   obtained by a stochastic algorithm in the objective space.

The goal of **moocore** is to provide an implementation of these functions that is efficient, thoroughly tested, well-documented, multi-platform (Windows, Linux, MacOS), multi-language (C, R, and Python, so far), with minimal dependencies and reusable from other libraries and packages, thus providing an easy-to-use and high-performance building block.

Most critical functionality is implemented in C, with the R and Python packages providing convenient interfaces to the C code.

**Keywords**: empirical attainment function, summary attainment surfaces, EAF differences, multi-objective optimization, bi-objective optimization, performance measures, performance assessment


Compilation
===========

We recommend that you compile specifically for your architecture using GCC option `-march=`. The default compilation is done with:

    make all

This uses the option `"-march=native"`. If your GCC version does not support `"native"`, you can give an explicit architecture, e.g.,

    make all MARCH=x86-64-v2

See the [GCC manual](https://gcc.gnu.org/onlinedocs/gcc/Submodel-Options.html) for the names of the architectures supported by your version of GCC.

You can compile the code with various runtime checks enabled using the option `DEBUG=1`.  This will slow down the code significantly.  If this is too slow for your purposes, you can disable the [sanitizers](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html#index-fsanitize_003daddress) using `make all DEBUG=1 SANITIZERS=""`.

The build system will try to pick good compiler flags for you, but if you need to, you can override them by passing the option `OPT_CFLAGS`. For example, to disable compiler optimization and enabled debug symbols, you could run:

    make all OPT_CFLAGS="-O0 -g"


If you do not want to see the command line of each compiler
invocation, you pass `S=1` to `make`.


Embedding (shared library)
==================================

Most functions are implemented in headers such as `nondominated.h`. For other functions, it is possible to build a shared library for embedding the code into your own C/C++ programs:

    make shlibs

Functions exported by the library are marked with `MOOCORE_API`.


Testsuite
=========

The **moocore** executables are validated using a [comprehensive testsuite](https://github.com/multi-objective/testsuite). Running the testsuite requires [Python](https://www.python.org/downloads/) `>= 3.10` and additional Python packages (see [`testsuite/requirements.txt`](https://github.com/multi-objective/testsuite/blob/main/requirements.txt)).  To run the testsuite yourself, follow these steps:

```bash
# Download moocore
git clone https://github.com/multi-objective/moocore moocore
# Download the testsuite
git clone https://github.com/multi-objective/testsuite moocore/testsuite
# Install required python packages
python3 -m pip install -r testsuite/requirements.txt
# Testing
make -C moocore/c/ test
# Timing
make -C moocore/c/ time
```

Under `moocore/c/`, `make test` will compile the code with `DEBUG=1 SANTIZERS="" OPT_CFLAGS="-Og -g3"`. This is useful for debugging failures.

`make time` will compile the code with `DEBUG=0 SANITIZERS="" OPT_CLAGS="-O3 -ftlo -march=native"`. This is useful for benchmarking code and making sure compiler optimizations do not break anything.

Neither `make test` nor `make time` recompile the code that has not been modified. So you can compile the code first with your preferred compiler flags and then run the testsuite. For example, if you compile with `make all DEBUG=1`, the code will be compiler with several [sanitizers](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html#index-fsanitize_003daddress) enabled. You can then call `make test` to run the testsuite with the sanitizers enabled.  This is much slower, but very helpful to track crashes.

Under `c/` there are various targets to run only parts of the testsuite, for example:

    make test-hv
    make test-nondominated
    ...



Command-line executables
========================

Input format
------------

All command-line tools can read the input as a filename in the command line:

    hv datafile

or from standard input:

    cat datafile | hv

In an input file, each point is given in a separate line, and each coordinate
within a line is separated by whitespace. An empty line denotes a separate
set. Most tools have an `--union` option that ignores those empty lines.  All
tools assume that all objectives must be minimized but this can be changed via
options `--maximise` or `--obj=`.  Also, maximization objectives may be
multiplied by -1 to convert them to minimization (see the option `--agree` of `nondominated`).


nondominated
------------

Check dominance, filter and transform the sets given as input.

```
Usage:
       nondominated [OPTIONS] [FILES]
       nondominated [OPTIONS] < [INPUT] > [OUTPUT]

Options:
 -h, --help          print this summary and exit;
     --version       print version number (and compilation flags) and exit;
 -v, --verbose       print some extra information;
 -q, --quiet         print as little as possible;
     --no-check      do not check nondominance of sets (faster but unsafe);
 -o, --obj=[+|-]...  specify whether each objective should be minimised (-)
                     or maximised (+). By default all are minimised;
     --maximise      all objectives must be maximised;
 -u, --upper-bound POINT defines an upper bound to check, e.g. "10 5 30";
 -l, --lower-bound POINT defines a lower bound to check;
 -U, --union         consider each file as a whole approximation set,
                     (by default, approximation sets are separated by an
                     empty line within a file);
 -s, --suffix=STRING suffix to add to output files. Default is "_dat".
                     The empty string means overwrite the input file.
                     This is ignored when reading from stdin because output
                     is sent to stdout.

 The following options OVERWRITE output files:
 -a, --agree=<max|min> transform objectives so all are maximised (or
                       minimised). See also the option --obj.
 -f, --filter        check and filter out dominated points;
 -b, --force-bound   remove points that do not satisfy the bounds;
 -n, --normalise RANGE normalise all objectives to a range, e.g., "1 2".
                       If bounds are given with -l and -u, they are used
                       for the normalisation.
 -L, --log=[1|0]...  specify whether each objective should be transformed
                     to logarithmic scale (1) or not (0).
```

hv
--

Computes the exact hypervolume. A reference point can be given by the option `-r`.

    hv -r "10 10 10" data

If no reference point is given, the default is `maximum + 0.1 * (maximum - minimum)`, where the maximum and minimum values are calculated for each
coordinate from the union of all input points.

For other options, check the output of `hv --help`.


hvapprox
--------

Approximate the hypervolume value of each input set of each input file.  The
approximation uses (quasi-)Monte-Carlo sampling, thus gets more accurate with
larger values of `--nsamples`. With no file, or when file is `-`, read standard
input.

```
Usage: hvapprox [OPTIONS] [FILE...]
       hvapprox [OPTIONS] < [INPUT] > [OUTPUT]

Options:
 -h, --help          print this summary and exit;
     --version       print version number (and compilation flags) and exit;
 -v, --verbose       print some information (time, maximum, etc).
 -q, --quiet         print just the hypervolume (as opposed to --verbose).
 -u, --union         treat all input sets within a FILE as a single set.
 -r, --reference=POINT use POINT as reference point. POINT must be within
                     quotes, e.g., "10 10 10". If no reference point is
                     given, it is taken as max + 0.1 * (max - min) for each
                     coordinate from the union of all input points.
 -s, --suffix=STRING Create an output file for each input file by appending
                     this suffix. This is ignored when reading from stdin.
                     If missing, output is sent to stdout.
 -n, --nsamples=N    Number of Monte-Carlo samples (N is a positive integer).
 -m, --method=M      1: Monte-Carlo sampling using normal distribution;
                     2: Hua-Wang deterministic sampling (default).
 -S, --seed=SEED     Seed of the random number generator (positive integer).
                     Only method=1.
```

epsilon
-------

Calculates the epsilon measure for the sets given as input.

```
Usage:
       epsilon [OPTIONS] [FILES]
       epsilon [OPTIONS] < [INPUT] > [OUTPUT]

Options:
 -h, --help          print this summary and exit;
     --version       print version number (and compilation flags) and exit;
 -v, --verbose        print some information (time, number of points, etc.).
 -q, --quiet         print as little as possible;
 -a, --additive       epsilon additive value (default).
 -m, --multiplicative epsilon multiplicative value .
 -r, --reference FILE file that contains the reference set
 -o, --obj=[+|-]...  specify whether each objective should be minimised (-)
                     or maximised (+). By default all are minimised;
     --maximise      all objectives must be maximised;
 -s, --suffix=STRING  Create an output file for each input file by appending
                      this suffix. This is ignored when reading from stdin.
                      If missing, output is sent to stdout.
```

igd
---

Calculates quality metrics related to the generational distance (GD, IGD, IGD+, avg Hausdorff distance).

```
Usage:
       igd [OPTIONS] [FILES]
       igd [OPTIONS] < [INPUT] > [OUTPUT]


Options:
 -h, --help          print this summary and exit;
     --version       print version number (and compilation flags) and exit;
 -v, --verbose       print some information (time, number of points, etc.)
 -q, --quiet         print as little as possible;
   , --gd            report classical GD
   , --igd           report classical IGD
   , --gd-p          report GD_p (p=1 by default)
   , --igd-p         (default) report IGD_p (p=1 by default)
   , --igd-plus      report IGD+
   , --hausdorff     report avg Hausdorff distance = max (GD_p, IGD_p)
 -a, --all           compute everything
 -p,                 exponent that averages the distances
 -r, --reference FILE file that contains the reference set
 -o, --obj=[+|-]...  specify whether each objective should be minimised (-)
                     or maximised (+). By default all are minimised;
     --maximise      all objectives must be maximised;
 -s, --suffix=STRING Create an output file for each input file by appending
                     this suffix. This is ignored when reading from stdin.
                     If missing, output is sent to stdout.
```

eaf
---

Computes the empirical attainment function (EAF) of all input files.
With no file, or when file is `-`, read standard input.

```
Usage:  eaf [OPTIONS] [FILE...]

Options:
 -h, --help          print this summary and exit;
     --version       print version number (and compilation flags) and exit;
 -v, --verbose       print some information (time, input points, output
                     points, etc) in stderr. Default is --quiet
 -o, --output FILE   write output to FILE instead of standard output.
 -q, --quiet         print just the EAF (as opposed to --verbose)
 -b, --best          compute best attainment surface
 -m, --median        compute median attainment surface
 -w, --worst         compute worst attainment surface
 -p, --percentile REAL compute the given percentile of the EAF
 -l, --level  LEVEL    compute the given level of the EAF
 -i[FILE], --indices[=FILE]  write attainment indices to FILE.
                     If FILE is '-', print to stdout.
                     If FILE is missing use the same file as for output.
 -d[FILE], --diff[=FILE] write difference between half of runs to FILE.
                     If FILE is '-', print to stdout.
                     If FILE is missing use the same file as for output.
        , --polygons Write EAF as R polygons.
```

dominatedsets
-------------

Calculates the number of sets from one file that dominate the sets of the other
files.

```
Usage: dominatedsets [OPTIONS] [FILE...]

Options:
 -h, --help          print this summary and exit;
     --version       print version number (and compilation flags) and exit;
 -v, --verbose       print some information (time, number of points, etc.)
 -q, --quiet         print as little as possible;
 -p, --percentages   print results also as percentages.
     --no-check      do not check nondominance of sets (faster but unsafe).
 -o, --obj=[+|-]...  specify whether each objective should be minimised (-)
                     or maximised (+). By default all are minimised;
```

ndsort
------

Perform nondominated sorting in a list of points.

```
Usage: ndsort [OPTIONS] [FILE...]

Options:
 -h, --help          print this summary and exit;
     --version       print version number (and compilation flags) and exit;
 -v, --verbose       print some information (time, number of points, etc.)
 -q, --quiet         print as little as possible;
 -k, --keep-uevs     keep uniquely extreme values
 -r, --rank          don't break ties using hypervolume contribution
 -o, --obj=[+|-]...  specify whether each objective should be minimised (-)
                     or maximised (+). By default all are minimised;
```

License
========

See the [LICENSE](/LICENSE) and [COPYRIGHTS](/r/inst/COPYRIGHTS) files.


[c-build-badge]: https://github.com/multi-objective/moocore/actions/workflows/C.yml/badge.svg?event=push
[c-build-link]: https://github.com/multi-objective/moocore/actions/workflows/C.yml
[c-moocore-github]: https://github.com/multi-objective/moocore/tree/main/c#readme
[c-moocore-homepage]: https://github.com/multi-objective/moocore/tree/main/c#readme
[py-build-badge]: https://github.com/multi-objective/moocore/actions/workflows/python.yml/badge.svg?event=push
[py-build-link]: https://github.com/multi-objective/moocore/actions/workflows/python.yml
[py-coverage-badge]: https://codecov.io/gh/multi-objective/moocore/branch/main/graph/badge.svg?flag=python
[py-coverage-link]: https://app.codecov.io/gh/multi-objective/moocore/tree/main/python
[py-moocore-github]: https://github.com/multi-objective/moocore/tree/main/python#readme
[py-moocore-homepage]: https://multi-objective.github.io/moocore/python/
[py-moocore-pypi]: https://pypi.org/project/moocore/
[r-build-badge]: https://github.com/multi-objective/moocore/actions/workflows/R.yml/badge.svg?event=push
[r-build-link]: https://github.com/multi-objective/moocore/actions/workflows/R.yml
[r-coverage-badge]: https://codecov.io/gh/multi-objective/moocore/branch/main/graph/badge.svg?flag=R
[r-coverage-link]: https://app.codecov.io/gh/multi-objective/moocore/tree/main/r
[r-moocore-cran-results]: https://cran.r-project.org/web/checks/check_results_moocore.html
[r-moocore-cran]: https://cran.r-project.org/package=moocore
[r-moocore-github]: https://github.com/multi-objective/moocore/tree/main/r#readme
[r-moocore-homepage]: https://multi-objective.github.io/moocore/r/
[testsuite-badge]: https://github.com/multi-objective/testsuite/actions/workflows/moocore.yml/badge.svg
[testsuite-link]: https://github.com/multi-objective/testsuite/actions/workflows/moocore.yml
