
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
