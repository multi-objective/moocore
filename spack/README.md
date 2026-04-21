Spack package
=============

This is a recipe for installing `moocore` using [Spack](https://spack.readthedocs.io).

To build and install the package, follow these steps


```bash
git clone https://github.com/spack/spack.git
. ./spack/share/spack/setup-env.sh
spack compiler find
spack env create moocore-env
spacktivate moocore-env
spack repo add moocore/spack
spack add py-moocore
spack concretize -f
spack install
```
