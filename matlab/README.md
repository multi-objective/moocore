**moocore**: Core Mathematical Functions for Multi-Objective Optimization
=========================================================================

<!-- badges: start -->
[ [**MATLAB library**][c-moocore-homepage] ] [ [GitHub][c-moocore-github] ] [![Build status][matlab-build-badge]][c-build-link] [![Testsuite status][testsuite-badge]][testsuite-link]
<!-- badges: end -->

**Contributors:**
    [Manuel López-Ibáñez](https://lopez-ibanez.eu), Guillaume Jacquenot.



Introduction
============

This is the MATLAB library of the [**moocore** project](https://github.com/multi-objective/moocore). This README describes just this component. There is also a [`moocore` C library and command-line tools][c-moocore-homepage], [`moocore` Python package][py-moocore-homepage] and a [`moocore` R package][r-moocore-homepage].


The [**moocore** project](https://github.com/multi-objective/moocore/) collects fast implementations of core mathematical functions and algorithms for multi-objective optimization and makes them available to different programming languages via similar interfaces. These functions include:

 * Identifying and filtering dominated vectors.
 * Quality metrics such as (weighted) hypervolume, epsilon, IGD, etc.
 * Computation of the Empirical Attainment Function. The empirical attainment
   function (EAF) describes the probabilistic distribution of the outcomes
   obtained by a stochastic algorithm in the objective space.

The goal of **moocore** is to provide an implementation of these functions that is efficient, thoroughly tested, well-documented, multi-platform (Windows, Linux, MacOS), multi-language (C, R, Python, and MATLAB so far), with minimal dependencies and reusable from other libraries and packages, thus providing an easy-to-use and high-performance building block.

Most critical functionality is implemented in C, with the R, Python and MATLAB packages providing convenient interfaces to the C code.

**Keywords**: empirical attainment function, summary attainment surfaces, EAF differences, multi-objective optimization, bi-objective optimization, performance measures, performance assessment


Compilation
===========

The default compilation is done with:

    make all

This uses the GNU Octave tool [`mkoctfile`](https://linux.die.net/man/1/mkoctfile), but you can use MATLAB [`mex`](https://www.mathworks.com/help/matlab/ref/mex.html) with:

    make all MEX=mex

If you do not want to see the command line of each compiler
invocation, also pass `S=1` to `make`.

Examples
========

The functionality in MATLAB is currently limited to the computation of the [hypervolume](https://multi-objective.github.io/moocore/python/reference/functions.metrics.html#hypervolume-metric).  Help in extending the MATLAB interface is welcome.

You can see the example at `hypervolume.m` and run it with:

    make test

The example uses [`octave-cli`](https://docs.octave.org), but it should be easy to run it under MATLAB.

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
[matlab-moocore-homepage]: https://github.com/multi-objective/moocore/tree/main/matlab#readme
[matlab-moocore-github]: https://github.com/multi-objective/moocore/tree/main/matlab#readme
[matlab-build-badge]: https://github.com/multi-objective/moocore/actions/workflows/matlab.yml/badge.svg?event=push
[matlab-build-link]: https://github.com/multi-objective/moocore/actions/workflows/matlab.yml
