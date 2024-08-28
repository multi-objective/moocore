**moocore**: Core Algorithms for Multi-Objective Optimization
=============================================================

<!-- badges: start -->
[![CRAN version](https://www.r-pkg.org/badges/version-last-release/moocore)][r-moocore-cran]
[![CRAN Status](https://badges.cranchecks.info/worst/moocore.svg)][r-moocore-cran-results]
[![CRAN Downloads](https://cranlogs.r-pkg.org/badges/grand-total/moocore)][r-moocore-cran]
[![R build status][r-build-badge]][r-build-link]
[![coverage][r-coverage-badge]][r-coverage-link]
<!-- badges: end -->

[ [**Homepage**][r-moocore-homepage] ]
[ [**GitHub**][r-moocore-github] ]

**Maintainer:** [Manuel López-Ibáñez](https://lopez-ibanez.eu)

**Contributors:**
    [Manuel López-Ibáñez](https://lopez-ibanez.eu),
    [Carlos M. Fonseca](https://eden.dei.uc.pt/~cmfonsec/),
    [Luís Paquete](https://eden.dei.uc.pt/~paquete/),
    and Mickaël Binois.

---------------------------------------

Introduction
============

This webpage documents the `moocore` R package. There is also a [`moocore` Python package](https://multi-objective.github.io/moocore/python)


The goal of the [**moocore** project](https://github.com/multi-objective/moocore/) is to collect fast implementations of core mathematical functions and algorithms for multi-objective optimization and make them available to different programming languages via similar interfaces. These functions include:

 * Identifying and filtering dominated vectors.
 * Quality metrics such as (weighted) hypervolume, epsilon, IGD, etc.
 * Computation of the Empirical Attainment Function. The empirical attainment function (EAF) describes the probabilistic
distribution of the outcomes obtained by a stochastic algorithm in the
objective space.

Most critical functionality is implemented in C, with the R and Python packages providing convenient interfaces to the C code.


**Keywords**: empirical attainment function, summary attainment surfaces, EAF
differences, multi-objective optimization, bi-objective optimization,
performance measures, performance assessment

**Relevant literature:**

 1. <a name="LopPaqStu09emaa"/>Manuel López-Ibáñez, Luís Paquete, and Thomas Stützle. [Exploratory Analysis of Stochastic Local Search Algorithms in Biobjective Optimization](https://doi.org/10.1007/978-3-642-02538-9_9). In T. Bartz-Beielstein, M. Chiarandini, L. Paquete, and M. Preuss, editors, *Experimental Methods for the Analysis of Optimization Algorithms*, pages 209–222. Springer, Berlin, Germany, 2010.<br>
    (This chapter is also available in a slightly extended form as Technical Report TR/IRIDIA/2009-015).<br>
    [ [bibtex](https://lopez-ibanez.eu/LopezIbanez_bib.html#LopPaqStu09emaa) |
    doi: [10.1007/978-3-642-02538-9_9](https://doi.org/10.1007/978-3-642-02538-9_9)
    | [Presentation](https://lopez-ibanez.eu/doc/gecco2010moworkshop.pdf) ]


Download and installation
-------------------------

The **moocore** package is implemented in R. Therefore,
a basic knowledge of R is recommended to make use of all features.

The first step before installing the **moocore** package is to [install R](https://cran.r-project.org/). Once R is installed in the system, there are two methods for installing the **moocore** package:

 1. Install within R (automatic download, internet connection required). Invoke
    R, then

    ```r
        install.packages("moocore")
    ```

 2. [Download the **moocore** package from CRAN][r-moocore-cran] (you may also need to download and install
    first the [package matrixStats](https://cran.r-project.org/package=matrixStats)), and invoke at the command-line:

    ```bash
        R CMD INSTALL <package>
    ```
    where `<package>` is one of the three versions available: `.tar.gz` (Unix/BSD/GNU/Linux), `.tgz` (MacOS X), or `.zip` (Windows).

Search the [R documentation](https://cran.r-project.org/faqs.html) if you need more help to install an R package on your system.

Compiled command-line executables for various purposes can be found under `system.file(package="moocore", "bin")`.

If you wish to be notified of bugfixes and new versions, please subscribe to the [low-volume emo-list](https://lists.dei.uc.pt/mailman/listinfo/emo-list), where announcements will be made.

[ [Download **moocore** package from CRAN][r-moocore-cran] ]  [ [Documentation][r-moocore-homepage] ]  [ [Development version (GitHub)][r-moocore-github] ]


GitHub (Development version)
----------------------------

If you wish to try the development version, you can install it by executing the
following commands within the R console:

```R
    R> install.packages("devtools")
    R> devtools::install_github("multi-objective/moocore/r")
```

Usage
-----

Once the **moocore** package is installed, the following R commands will give more information:
```r
    library(moocore)
    ?eaf
    ?read_datasets
```


License
--------

This software is Copyright (C) 2024 Manuel López-Ibáñez, Carlos M. Fonseca, Luís Paquete, Mickaël Binois.

This program is free software (software libre); you can redistribute it and/or
modify it under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your option)
any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) for more details.

**IMPORTANT NOTE**: Please be aware that the fact that this program is released
as Free Software does not excuse you from scientific propriety, which obligates
you to give appropriate credit! If you write a scientific paper describing
research that made substantive use of this program, it is your obligation as a
scientist to (a) mention the fashion in which this software was used in the
Methods section; (b) mention the algorithm in the References section. The
appropriate citation is:

 * Manuel López-Ibáñez, Luís Paquete, and Thomas Stützle. **Exploratory Analysis of Stochastic Local Search Algorithms in Biobjective Optimization.** In T. Bartz-Beielstein, M. Chiarandini, L. Paquete, and M. Preuss, editors, *Experimental Methods for the Analysis of Optimization Algorithms*, pages 209–222. Springer, Berlin, Germany, 2010.  doi: 10.1007/978-3-642-02538-9_9

Moreover, as a personal note, I would appreciate it if you would email
`manuel.lopez-ibanez@manchester.ac.uk` with citations of papers referencing
this work so I can mention them to my funding agent and tenure committee.

[r-build-badge]: https://github.com/multi-objective/moocore/workflows/R.yml/badge.svg?event=push
[r-build-link]: https://github.com/multi-objective/moocore/actions/workflows/R.yml
[r-coverage-badge]: https://codecov.io/gh/multi-objective/moocore/branch/main/graph/badge.svg?flag=R
[r-coverage-link]: https://app.codecov.io/gh/multi-objective/moocore/tree/main/r
[r-moocore-github]: https://github.com/multi-objective/moocore/tree/main/r#readme
[r-moocore-homepage]: https://multi-objective.github.io/moocore/r/
[r-moocore-cran]: https://cran.r-project.org/package=moocore
[r-moocore-cran-results]: https://cran.r-project.org/web/checks/check_results_moocore.html
