MOOCORE: Core Mathematical Functions for Multi-Objective Optimization
=====================================================================

[ [**C library**][c-moocore-homepage] ] [ [GitHub][c-moocore-github] ] [![Build status][c-build-badge]][c-build-link]

[ [**R package**][r-moocore-homepage] ] [ [GitHub][r-moocore-github] ] [![Build status][r-build-badge]][r-build-link] [![Code Coverage][r-coverage-badge]][r-coverage-link]

[ [**Python package**][py-moocore-homepage] ] [ [GitHub][py-moocore-github] ] [![Build status][py-build-badge]][py-build-link] [![Code Coverage][py-coverage-badge]][py-coverage-link]

[![pre-commit.ci status](https://results.pre-commit.ci/badge/github/multi-objective/moocore/main.svg)](https://results.pre-commit.ci/latest/github/multi-objective/moocore/main)


**Contributors:**
    [Manuel López-Ibáñez](https://lopez-ibanez.eu),


The goal of this repository is to collect core mathematical functions and algorithms for multi-objective optimization and make them available to different programming languages via similar interfaces. These functions include:

 * Identifying and filtering dominated vectors.
 * Quality metrics such as (weighted) hypervolume, epsilon, IGD, etc.
 * Computation of the Empirical Attainment Function.

The repository is composed of:

 * `c/`: C library and command-line tools.
 * `r/`: An R package that uses the C library.
 * `python/`: A Python package that uses the C library.

Each component is documented in the `README.md` file found under each folder.

[c-build-badge]: https://github.com/multi-objective/moocore/workflows/C/badge.svg
[c-build-link]: https://github.com/multi-objective/moocore/actions/workflows/C.yaml
[c-moocore-homepage]: https://github.com/multi-objective/moocore/tree/main/c#readme
[c-moocore-github]: https://github.com/multi-objective/moocore/tree/main/c#readme
[py-build-badge]: https://github.com/multi-objective/moocore/workflows/Python/badge.svg
[py-build-link]: https://github.com/multi-objective/moocore/actions/workflows/python.yaml
[py-coverage-badge]: https://codecov.io/gh/multi-objective/moocore/branch/main/graph/badge.svg?flag=python
[py-coverage-link]: https://codecov.io/gh/multi-objective/moocore/tree/main/python
[py-moocore-github]: https://github.com/multi-objective/moocore/tree/main/python#readme
[py-moocore-homepage]: https://multi-objective.github.io/moocore/python
[r-build-badge]: https://github.com/multi-objective/moocore/workflows/R/badge.svg
[r-build-link]: https://github.com/multi-objective/moocore/actions/workflows/R.yaml
[r-coverage-badge]: https://codecov.io/gh/multi-objective/moocore/branch/main/graph/badge.svg?flag=R
[r-coverage-link]: https://codecov.io/gh/multi-objective/moocore/tree/main/r
[r-moocore-github]: https://github.com/multi-objective/moocore/tree/main/r#readme
[r-moocore-homepage]: https://multi-objective.github.io/moocore/r/
