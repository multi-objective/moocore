
How to release
==============

1. Run [testsuite workflow](https://github.com/multi-objective/testsuite/actions/workflows/moocore.yml)

1. `./release.py`

1. `git ci -a -m "Prepare to release v${PACKAGEVERSION}"`

1. R release (within `r/`):

    1. `make releasebuild` and check which files are included in the package.
    1. `make releasecheck`
    1. [Check reverse dependencies](https://github.com/multi-objective/moocore/actions/workflows/revdepcheck.yml)
    1. Update `cran-comments.md`
    1. Submit to CRAN: `make submit`

1. [Publish a release in github](https://github.com/multi-objective/moocore/releases/new) to automatically submit to PyPi.

1. `./release.py --dev NEW_VERSION`

1. `git ci -a -m "Start development of v{NEW_VERSION}"`


How to extend the Python API
============================

`moocore` consists of C code called by R and Python. In both cases, the C code is compiled into a dynamic shared library.

Python uses CFFI to call the C code: https://github.com/multi-objective/moocore/blob/main/python/src/moocore/_ffi_build.py

CFFI is fast and makes very easy to extend the C API.  Just add the declaration of the function you want to export to a [`libmoocore.h`](https://github.com/multi-objective/moocore/blob/main/python/src/moocore/libmoocore.h).


How to extend the R API
=======================

Extending the R API requires adding a C wrapper function that converts from R types to C types and export that C function to R. See [`Rmoocore.c`](https://github.com/multi-objective/moocore/blob/main/r/src/Rmoocore.c) and [`init.h`](https://github.com/multi-objective/moocore/blob/main/r/src/init.h)


Code Formatting
===============

C/C++ Code
----------

C/C++ code in the `c/` directory is automatically formatted using `clang-format` via pre-commit hooks.

The formatting style is configured in `.clang-format` and aims to match the existing code style:
- 4-space indentation
- K&R style (function return type on separate line)
- 80-character column limit
- Space on both sides of pointer declarations (e.g., `type * name`)

To manually format C/C++ files:

```bash
clang-format -i c/*.c c/*.h
```

The pre-commit hook will automatically format C/C++ files when you commit changes.
