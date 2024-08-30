**moocore**: Core Mathematical Functions for Multi-Objective Optimization
=========================================================================
<!-- badges: start -->
[ [**C library**][c-moocore-homepage] ] [ [GitHub][c-moocore-github] ] [![Build status][c-build-badge]][c-build-link]
<!-- badges: end -->

**Contributors:**
    [Manuel López-Ibáñez](https://lopez-ibanez.eu),
    [Carlos M. Fonseca](https://eden.dei.uc.pt/~cmfonsec/),
    [Luís Paquete](https://eden.dei.uc.pt/~paquete/),
    Andreia P. Guerreiro.



Introduction
============

This is the C library and associated command-line tools of the [**moocore** project](https://github.com/multi-objective/moocore). This README describes just this component. There is also a [`moocore` Python package][python-moocore-homepage] and a [`moocore` R package][r-moocore-homepage].


The goal of the [**moocore** project](https://github.com/multi-objective/moocore/) is to collect fast implementations of core mathematical functions and algorithms for multi-objective optimization and make them available to different programming languages via similar interfaces. These functions include:

 * Identifying and filtering dominated vectors.
 * Quality metrics such as (weighted) hypervolume, epsilon, IGD, etc.
 * Computation of the Empirical Attainment Function. The empirical attainment function (EAF) describes the probabilistic
distribution of the outcomes obtained by a stochastic algorithm in the
objective space.

Most critical functionality is implemented in C, with the R and Python packages providing convenient interfaces to the C code.

**Keywords**: empirical attainment function, summary attainment surfaces, EAF differences, multi-objective optimization, bi-objective optimization, performance measures, performance assessment


Compilation
===========

We recommend that you compile specifically for your architecture using GCC option `-march=`. The default compilation is done with:

    make nondominated

This uses the option `"-march=native"`. If your GCC version does not support `"native"`, you can give an explicit architecture:

    make nondominated march=i686

See the GCC manual for the names of the architectures supported by your version of GCC.


Command-line tools
=================

nondominated
------------

Obtain information and perform filtering operations on the nondominated sets given as input.


License
=========

See the LICENSE file and the COPYRIGHTS file.


[c-build-badge]: https://github.com/multi-objective/moocore/actions/workflows/C.yml/badge.svg?event=push
[c-build-link]: https://github.com/multi-objective/moocore/actions/workflows/C.yml
[c-moocore-github]: https://github.com/multi-objective/moocore/tree/main/c#readme
[c-moocore-homepage]: https://github.com/multi-objective/moocore/tree/main/c#readme
[py-build-badge]: https://github.com/multi-objective/moocore/actions/workflows/python.yml/badge.svg?event=push
[py-build-link]: https://github.com/multi-objective/moocore/actions/workflows/python.yml
[py-coverage-badge]: https://codecov.io/gh/multi-objective/moocore/branch/main/graph/badge.svg?flag=python
[py-coverage-link]: https://app.codecov.io/gh/multi-objective/moocore/tree/main/python
[py-moocore-github]: https://github.com/multi-objective/moocore/tree/main/python#readme
[py-moocore-homepage]: https://multi-objective.github.io/moocore/python
[py-moocore-pypi]: https://pypi.org/project/moocore/
[r-build-badge]: https://github.com/multi-objective/moocore/actions/workflows/R.yml/badge.svg?event=push
[r-build-link]: https://github.com/multi-objective/moocore/actions/workflows/R.yml
[r-coverage-badge]: https://codecov.io/gh/multi-objective/moocore/branch/main/graph/badge.svg?flag=R
[r-coverage-link]: https://app.codecov.io/gh/multi-objective/moocore/tree/main/r
[r-moocore-cran]: https://cran.r-project.org/package=moocore
[r-moocore-cran-results]: https://cran.r-project.org/web/checks/check_results_moocore.html
[r-moocore-github]: https://github.com/multi-objective/moocore/tree/main/r#readme
[r-moocore-homepage]: https://multi-objective.github.io/moocore/r/
