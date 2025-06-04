
How to release
==============

1. Bump version number `PACKAGEVERSION` in `Makefile`.

1. `./release.py`

1. `git ci -a -m "Prepare to release v${PACKAGEVERSION}"`

1. R release (within `r/`):

  1. `make releasecheck`
  1. Check reverse dependencies.
  1. Update `cran-comments.md`
  1. Submit to CRAN: `make submit`
  1. Once accepted, `make closeversion`

1. Publish a release in github to automatically submit to PyPi:

  1. [Create release](https://github.com/multi-objective/moocore/releases/new)

1. Bump version number ${PACKAGEVERSION} in `Makefile` and update:

    - c/Makefile: Append `$(REVISION)`
    - r/DESCRIPTION: Append .900
    - python/pyproject.toml: set to `${PACKAGEVERSION}.dev0`.

1. `git ci -a -m "Start development version"`
