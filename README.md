**moocore**: Core Mathematical Functions for Multi-Objective Optimization
=====================================================================

|Homepage | Code | Build status | Coverage | Package | Downloads |
| :---: | :---: | :---: | :---: | :---: | :---: |
| [**C library**][c-moocore-homepage] | [GitHub][c-moocore-github] |[![Build status][c-build-badge]][c-build-link]| |
| [**R package**][r-moocore-homepage] | [GitHub][r-moocore-github] |[![Build status][r-build-badge]][r-build-link] [![r-universe build status][r-universe-build-badge]][r-universe-build-link]|[![Code Coverage][r-coverage-badge]][r-coverage-link]|[![CRAN version](https://www.r-pkg.org/badges/version-last-release/moocore)][r-moocore-cran] [![CRAN Status](https://badges.cranchecks.info/worst/moocore.svg)][r-moocore-cran-results] [![r-universe version][r-universe-version-badge]][r-universe-version]|[![CRAN Downloads](https://cranlogs.r-pkg.org/badges/grand-total/moocore)][r-moocore-cran]|
| [**Python package**][py-moocore-homepage] | [GitHub][py-moocore-github] |[![Build status][py-build-badge]][py-build-link]|[![Code Coverage][py-coverage-badge]][py-coverage-link]|[![PyPI - Version](https://img.shields.io/pypi/v/moocore)][py-moocore-pypi]|[![PyPI - Downloads](https://img.shields.io/pypi/dm/moocore?color=blue)][py-moocore-pypi-stats]|
| [**MATLAB library**][matlab-moocore-homepage] | [GitHub][matlab-moocore-github] |[![Build status][matlab-build-badge]][matlab-build-link]||||

[![pre-commit.ci status](https://results.pre-commit.ci/badge/github/multi-objective/moocore/main.svg)](https://results.pre-commit.ci/latest/github/multi-objective/moocore/main)
[![Testsuite status][testsuite-badge]][testsuite-link]

**Contributors:**
    [Manuel López-Ibáñez](https://lopez-ibanez.eu),
    [Carlos M. Fonseca](https://eden.dei.uc.pt/~cmfonsec/),
    [Luís Paquete](https://eden.dei.uc.pt/~paquete/),
    Andreia P. Guerreiro,
    Mickaël Binois,
    Leonardo C.T. Bezerra,
    Fergus Rooney,
    [Lennart Schäpermeier](https://schaepermeier.github.io).

---------------------------------------

The goal of the [**moocore** project](https://github.com/multi-objective/moocore/) is to collect and document fast implementations of core mathematical functions and algorithms for multi-objective optimization and make them available to different programming languages via similar interfaces. These functions include:

 * Generate and transform nondominated sets.
 * Identify and filter dominated vectors.
 * Quality metrics such as (weighted) hypervolume, epsilon, IGD, etc.
 * Computation of the Empirical Attainment Function. The empirical attainment function (EAF) describes the probabilistic
distribution of the outcomes obtained by a stochastic algorithm in the
objective space.

Most critical functionality is implemented in C, with the R, Python and MATLAB packages providing convenient interfaces to the C code.

The repository is composed of:

 * `c/`: C library and command-line tools.
 * `r/`: An R package that uses the C library.
 * `python/`: A Python package that uses the C library.
 * `matlab/`: MATLAB functions that use the C library.

Each component is documented in the `README.md` file found under each folder.

In addition to the tests within each component, there is a separate [comprehensive **testsuite**](https://github.com/multi-objective/testsuite) that is run before each release.

Who is using `moocore`?
-----------------------

The following projects currently use `moocore`:

 - [`mooplot`](https://github.com/multi-objective/mooplot)
 - [`DESDEO`](https://github.com/industrial-optimization-group/DESDEO/)
 - [`GPareto`](https://github.com/mbinois/GPareto)
 - [`IOHInspector`](https://github.com/IOHprofiler/IOHinspector?tab=readme-ov-file)
 - [`Liger`](https://www.ide.uk/project-liger)
 - [`bonobench`](https://github.com/schaepermeier/bonobench)
 - [`gMOIP`](https://cran.r-project.org/web/packages/gMOIP/index.html)
 - [`jMetalPy`](https://jmetalpy.readthedocs.io/en/develop/)
 - [`pymoo`](https://pymoo.org/)
 - [`seqme`](https://github.com/szczurek-lab/seqme)
 - [QuantumAnnealingMOO](https://github.com/andrew-d-king/QuantumAnnealingMOO) (D-Wave Quantum Inc)


Citing `moocore`
----------------

There is no publication associated with `moocore` yet.

If you are using specific functions in your research, for example, to [compute the hypervolume](https://multi-objective.github.io/moocore/python/reference/generated/moocore.hypervolume.html), you should cite the papers that proposed the algorithms implemented in `moocore`.  They are referenced in the documentation of each function.

If you want to explicitly acknowledge `moocore`, please add a link to [this Github repository](https://github.com/multi-objective/moocore).


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
[py-moocore-pypi-stats]: https://pypistats.org/packages/moocore
[r-build-badge]: https://github.com/multi-objective/moocore/actions/workflows/R.yml/badge.svg?event=push
[r-build-link]: https://github.com/multi-objective/moocore/actions/workflows/R.yml
[r-coverage-badge]: https://codecov.io/gh/multi-objective/moocore/branch/main/graph/badge.svg?flag=R
[r-coverage-link]: https://app.codecov.io/gh/multi-objective/moocore/tree/main/r
[r-moocore-cran]: https://cran.r-project.org/package=moocore
[r-moocore-cran-results]: https://cran.r-project.org/web/checks/check_results_moocore.html
[r-moocore-github]: https://github.com/multi-objective/moocore/tree/main/r#readme
[r-moocore-homepage]: https://multi-objective.github.io/moocore/r/
[r-universe-build-badge]: https://github.com/r-universe/multi-objective/actions/workflows/build.yml/badge.svg
[r-universe-build-link]: https://github.com/r-universe/multi-objective/actions/workflows/build.yml
[r-universe-version]: https://multi-objective.r-universe.dev/moocore
[r-universe-version-badge]: https://multi-objective.r-universe.dev/badges/moocore
[testsuite-badge]: https://github.com/multi-objective/testsuite/actions/workflows/moocore.yml/badge.svg
[testsuite-link]: https://github.com/multi-objective/testsuite/actions/workflows/moocore.yml
[matlab-moocore-homepage]: https://github.com/multi-objective/moocore/tree/main/matlab#readme
[matlab-moocore-github]: https://github.com/multi-objective/moocore/tree/main/matlab#readme
[matlab-build-badge]: https://github.com/multi-objective/moocore/actions/workflows/matlab.yml/badge.svg?event=push
[matlab-build-link]: https://github.com/multi-objective/moocore/actions/workflows/matlab.yml
