**moocore**: Core Algorithms for Multi-Objective Optimization
=============================================================

<!-- badges: start -->
[![PyPI - Version](https://img.shields.io/pypi/v/moocore)][py-moocore-pypi]
[![PyPI - Downloads](https://img.shields.io/pypi/dm/moocore?color=blue)][py-moocore-pypi]
[![Python build status][py-build-badge]][py-build-link]
[![coverage][py-coverage-badge]][py-coverage-link]
<!-- badges: end -->

[ [**Homepage**][py-moocore-homepage] ]
[ [**GitHub**][py-moocore-github] ]


**Contributors:**
    [Manuel López-Ibáñez](https://lopez-ibanez.eu),
    Fergus Rooney.

---------------------------------------

Introduction
============

The goal of **moocore** is to collect fast implementations of core mathematical functions and algorithms for multi-objective optimization. These functions include:

 * Identifying and filtering dominated vectors.
 * Quality metrics such as (weighted) hypervolume, epsilon, IGD, etc.
 * Computation of the Empirical Attainment Function. The empirical attainment function (EAF) describes the probabilistic
distribution of the outcomes obtained by a stochastic algorithm in the
objective space.

**Keywords**: empirical attainment function, summary attainment surfaces, EAF
differences, multi-objective optimization, bi-objective optimization,
performance measures, performance assessment


R package
---------

There is also a `moocore` package for R: https://multi-objective.github.io/moocore/r


[py-build-badge]: https://github.com/multi-objective/moocore/actions/workflows/python.yml/badge.svg?event=push
[py-build-link]: https://github.com/multi-objective/moocore/actions/workflows/python.yml
[py-coverage-badge]: https://codecov.io/gh/multi-objective/moocore/branch/main/graph/badge.svg?flag=python
[py-coverage-link]: https://app.codecov.io/gh/multi-objective/moocore/tree/main/python
[py-moocore-github]: https://github.com/multi-objective/moocore/tree/main/python#readme
[py-moocore-homepage]: https://multi-objective.github.io/moocore/python
[py-moocore-pypi]: https://pypi.org/project/moocore/
