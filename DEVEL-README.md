
How to release
==============

1. Run [testsuite workflow](https://github.com/multi-objective/testsuite/actions/workflows/moocore.yml)

1. `make clean`

1. `./release.py`

1. `git ci -a -m "Prepare to release v${PACKAGEVERSION}"`

1. R release (within `r/`):

    1. `make releasebuild` and check which files are included in the package.
    1. `make releasecheck`
    1. [Check reverse dependencies](https://github.com/multi-objective/moocore/actions/workflows/revdepcheck.yml)
    1. Update `cran-comments.md`
    1. Submit to CRAN: `make submit`
    1. While waiting, run benchmarks `make benchmarks`

1. [Publish a release in github](https://github.com/multi-objective/moocore/releases/new) to automatically submit to PyPi.

1. `./release.py --dev NEW_VERSION`

1. `git ci -a -m "Start development of v{NEW_VERSION}"`


Building the code
=================

C code
-----

See [*Compilation*](https://github.com/multi-objective/moocore/tree/main/c#compilation) to compile the C code under `c/`.

R and Python packages
---------------------

Under `r/` or `python/`, the commands `make build` and `make install` will build and install the R and Python packages.


Testing
========

The C code and the Python and R packages have their own testsuites. See [C testsuite](https://github.com/multi-objective/moocore/tree/main/c#testsuite) to setup and run the C testsuite.

The Python and R testsuites are run automatically via Github Actions for every push and pull request. You can run then manually by running `make test` under `r/` or `python/`.

You can run all testsuites at once by executing at the top-level:

    make test


How to extend the Python API
============================

`moocore` consists of C code called by R and Python. In both cases, the C code is compiled into a dynamic shared library.

Python uses CFFI to call the C code: https://github.com/multi-objective/moocore/blob/main/python/src/moocore/_ffi_build.py

CFFI is fast and makes very easy to extend the C API.  Just add the declaration of the function you want to export to a [`libmoocore.h`](https://github.com/multi-objective/moocore/blob/main/python/src/moocore/libmoocore.h).


How to extend the R API
=======================

Extending the R API requires adding a C wrapper function that converts from R types to C types and export that C function to R. See [`Rmoocore.c`](https://github.com/multi-objective/moocore/blob/main/r/src/Rmoocore.c) and [`init.h`](https://github.com/multi-objective/moocore/blob/main/r/src/init.h)


Documentation
=============


> [!IMPORTANT]
> The documentation of the R and Python packages should be as consistent as possible.

A major difference is that Python has top-level pages that describe common functionality, [Python example](https://multi-objective.github.io/moocore/python/reference/functions.metrics.html), and documentation for individual functions such as [`igd_plus()`](https://multi-objective.github.io/moocore/python/reference/generated/moocore.igd_plus.html#moocore.igd_plus), whereas R has the option to describe multiple related functions in the same page [R example](https://multi-objective.github.io/moocore/r/reference/igd.html).

In Python, one must decide where the documentation should be placed (in the common page or in the page of an individual function). In any case, the goals are to avoid duplicating the same text in different parts of the Python documentation and that the text is as similar as possible between Python and R.

References
----------

> [!IMPORTANT]
> `moocore` provides accurate references to the original sources of mathematical concepts and algorithms.

Bibliographic entries should be taken from the [IRIDIA BibTeX Repository](https://iridia-ulb.github.io/references/).
If the BibTeX entry does not exist, [contribute it](https://github.com/iridia-ulb/references/blob/master/README.md#contributing-to-the-iridia-bibtex-repository).
If the entry already exists, you just need to add its label to the file
[`bibkeys.txt`](https://github.com/multi-objective/moocore/blob/main/bibkeys.txt) and run [`update_bib.sh`](https://github.com/multi-objective/moocore/blob/main/update_bib.sh).
