
How to release
==============

1. Bump version number in `c/Makefile`, `r/DESCRIPTION` and `python/pyproject.toml`

    - c/Makefile: Remove `$(REVISION)` and set to `${NEXT_VERSION}`.
    - r/DESCRIPTION: Remove .900 and set to `${NEXT_VERSION}`.
    - python/pyproject.toml: Remove `.dev[0-9]`.

1. `git ci -a -m "Prepare to release v${NEXT_VERSION}"`

1. Submit to CRAN.

1. Once accepted, `make closeversion`

1. Update dates and version number in `python/doc/source/whatsnew/index.rst`

1. Publish a release in github to automatically submit to PyPi.

1. Bump version number ${NEXT_VERSION} and update:

    - c/Makefile: Append `$(REVISION)`
    - r/DESCRIPTION: Append .900.
    - python/pyproject.toml: set to `${NEXT_VERSION}.dev0`.

1. `git ci -a -m "Start development version"`
