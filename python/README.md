[ [**Homepage**](py-moocore-homepage) ]
[ [**GitHub**](py-moocore-github) ]
[![Python build status][py-build-badge]][py-build-link]
[![coverage][py-coverage-badge]][py-coverage-link]


# eaf


## TODO basic Python setup
- [x] basic Python package structure (https://packaging.python.org/en/latest/tutorials/packaging-projects/)
- [x] setup for testing (https://mathspp.com/blog/how-to-create-a-python-package-in-2022)
- [x] github actions that build the package run the testsuite (https://mathspp.com/blog/how-to-create-a-python-package-in-2022)
  - [x] for Linux
  - [x] for Windows
  - [x] for macOS
- [x] setup for C extensions (see below)
- [ ] Rename the package to just `eaf`
- [x] Move all `*.c` and `*.h` files to `libeaf/` so that the C code is separated from the Python code.
- [ ] Setup for building and publishing the package in pip: https://mathspp.com/blog/how-to-create-a-python-package-in-2022#publish-the-real-package
- [ ] Other nice things to have (but lower priority):
  - [x] Coverage: https://github.com/codecov/example-python (see also: https://mathspp.com/blog/how-to-create-a-python-package-in-2022#running-coverage-py-with-tox)
  - [ ] Setup pre-commit to run by github actions: https://pre-commit.ci/ . Look at this example:
<https://github.com/poliastro/poliastro/commit/0bbcf95e50e3d740db659f651cbb56cc28219cf1>
  - [x] Documentation: (short intro: https://docs.python-guide.org/writing/documentation/) Longer: https://py-pkgs.org/06-documentation.html
    - [ ] API documentation: https://www.sphinx-doc.org/en/master/tutorial/automatic-doc-generation.html (I would suggest to use NumPy style annotations: https://www.sphinx-doc.org/en/master/usage/extensions/napoleon.html)
    - [x] Jupyter notebooks generated from python files: https://sphinx-gallery.github.io/stable/getting_started.html#create-simple-gallery (editing jupyter notebooks in github is not easy and they take a lot of space, it is better if they are generated from .py files that have markdown in comments)
  - [ ] Tutorial showing how to use the package: add it to the documentation or as a jupyter notebook.
  - [ ] Source of good ideas: https://github.com/anyoptimization/pymoo/tree/main

## TODO Setup for C extensions

We want to do the following in Python:

```python
import eaf
x = eaf.read_datasets("input1.dat")
# x is now a Numpy matrix with 3 columns and 100 rows. The first three rows are:
# 8.0755965 2.4070255   1
# 8.6609445 3.6405014   1
# 0.2081643 4.6227547   1
...
# The last three rows are:
# 1.2223439 5.6895031  10
# 7.9946696 2.8112254  10
# 2.1270029 2.4311417  10
print(x)
```
  - [x] The function `read_datasets` will call the C function `read_double_data()` either directly or via another C function (see next point) and setup everything that it needs to return a NumPy matrix.
  - [x] You may need to add additional C code to interface between Python and C. This is OK (see how I did it in R: https://github.com/MLopez-Ibanez/eaf/blob/5be4108dc02c10f48ea5ebedbeaaccf504531791/src/Reaf.c#L329)
  - [x] Investigate options available (ctype, CFFI or something else): What are the positives and negatives of each option?
  - [x] Add a few tests to make sure it is working as expected.
  - [x] Setup github actions / package build for Windows, macOS and Linux.
  - [x] Next function is `fpli_hv()` in `hv.h`

      ```python
       import eaf
       x = eaf.read_datasets("tests/test_data/input1.dat")
       # x is now a Numpy matrix with 3 columns and 100 rows. The first three rows are:
       # 8.0755965 2.4070255   1
       # 8.6609445 3.6405014   1
       # 0.2081643 4.6227547   1
       ...
       # The last three rows are:
       # 1.2223439 5.6895031  10
       # 7.9946696 2.8112254  10
       # 2.1270029 2.4311417  10
       print(x)
       z = hv(X[X[:,2] == 1, :2], ref = np.array([10, 10]))
       print(z)
       # It should be a single number: 90.4627276475589
      ```

  - [ ] More C functions: https://github.com/multi-objective/moocore/issues/4

## TODO Plotting

Once we are able to read the data into Python, the package should offer utilities for plotting.  What is the best way of plotting the data needs to be studied (it could be a combination of methods: e.g., matplotlib for static plots + plotly for interactive ones):

 - Basic matplotlib (https://matplotlib.org/):
   - Advantages: very flexible, ...
   - Disadvantage: complex to use, static plots, ...
 - Seaborn (https://seaborn.pydata.org/):
   - Advantages: things that are hard in matplotlib are easier in seaborn, ...
   - Disadvantages: less flexible, static plots, ...
 - plotnine (https://plotnine.readthedocs.io/en/stable/):
   - Advantages: things that are hard in matplotlib are easier in seaborn, grammar of graphics (easy to modify a plot already created), ...
   - Disadvantages: less flexible, static plots, ...
 - Plotly (https://plotly.com/python/):
   - Advantages: interactive plots, nice 3D plots, ...
   - Disadvantages: almost as complex as matplotlib, slower, static plots that look nice when printed may be harder, ...
 - anything else???

### Which plots? ###

 - [ ] We want to plot the datasets read as points (scatter-plot, different symbol and/or color per set) or as stair/step lines (https://matplotlib.org/stable/gallery/lines_bars_and_markers/stairs_demo.html#comparison-of-pyplot-step-and-pyplot-stairs)

 - [ ] We want to plot attainment surfaces as stair/step lines or as a collection of filled rectangles (https://matplotlib.org/stable/gallery/statistics/errorbars_and_boxes.html#sphx-glr-gallery-statistics-errorbars-and-boxes-py) or as a filled polygon with axis-aligned sides (https://matplotlib.org/stable/gallery/lines_bars_and_markers/fill.html).

     ![lines](https://mlopez-ibanez.github.io/eaf/reference/eafplot-7.png)

     ![areas](https://mlopez-ibanez.github.io/eaf/reference/eafplot-8.png)

### TODO plots
- [x] 2d line plots -> The points should extend out to infinity for the first and last point
- [ ] 3d cube plots -> The cube should extend down from infinity to the point for minimisation, instead of from zero
  - [ ] There is a bug in how I have drawn the triangles where a lot of the triangles sit on top of each other, add an optimisation that fixes all the unneccesary intersections
- [x] EAF plots -> Need to be able to shade areas of the plots with different colors
- [ ] Make EAF plots capable of minimisation
- [ ] Add labels to filled areas
- [ ] Add subplot functionality


## Developer instructions
### Quick start - Use the package
```sh
git clone https://github.com/multi-objective/moocore
cd eafpy
pip install -r requirements.txt
py -m build
pip install .
# You should now be able to import the package
```
### Quick start - Develop the package

```sh
pip install -r requirements_dev.txt
pre-commit install
py -m build
pip install -e .
# Run tests
py -m pytest-cov
```

It's not strictly neccesary to use a virtualenv for the dev requirements (setuptools makes its own virtualenv when building) but it is generally reccomend. I have skipped it for brevity.

### Full instructions
Pip package manager is required. Ensure and upgrade pip:

`python3 -m ensurepip --upgrade`

#### Install required development packages

It is reccomended to create a new virtual environment for development. You can do this using virtual env:

`pip install virtualenv`

Create and activate a new virtual environment (You can do this in the repo root. It will create a `.gitignore`):

```sh
virtualenv -p python3 eaf_env
cd eaf_env/scripts
activate
```

Now you can install the development packages in this fresh environment. You will need to make sure you have the environment activated every time you develop.

```sh
# cd to the repo root
pip install -r requirements_dev.txt
```

#### Install the pre-commit hooks for the repo

Pre-commit is a package that should now be installed, it adds some hooks that will execute when you make a new git commit, such as formatting the code with `black`.The `.pre-commmit-config.yaml` configures which hooks are used. You need to install these hooks locally using:

    pre-commit install

It is a good idea to run the them:

    pre-commit run --all-files

#### Make sure you have a reccomended C compiler

This package uses the CFFI package to compile a C extension module, so if you want to build the project you need have one of the reccomended C compilers installed.
1. Windows: [MSVC - install Visual studio](https://visualstudio.microsoft.com/vs/features/cplusplus/)
2. Linux - get gcc

    ```sh
    sudo apt update
    sudo apt install build-essential
    ```

3. MacOS - get gcc

   ```sh
   brew update
   brew upgrade
   brew install gcc
   ```

If you have more trouble with the compilation you can visit [this CFFI doc](https://cffi.readthedocs.io/en/latest/installation.html#:~:text=Requirements%3A,to%20compile%20C%20extension%20modules.)

#### Build the project
In order to test the package you need to build the project and install it.

```sh
# CD to the repo root

# For windows
py -m build

# For Linux/ Macos
python3 -m build
```

You can now install the package. Use this command:
`pip install -e .`
It is reccomended to reinstall the package every time you have re-compiled the C files. If you have only changed the python files, you don't need to re-build and re-install it every time due to the `-e` argument.
You can now test the installation worked by running the tests

#### Run the tests
If you have installed the package, you can run the tests by simply going to package root and executing:

`pytest --cov`

(Note that `pytest` will not work if the package is not installed, even if the C files are compiled. This is because of the way the imports work)

You can run the test suite that is executed in the github actions by running tox

```sh
cd /repo_root
tox
```

Some of these tests may fail because `tox` is setup to test several different python version that you might not have installed. `tox.ini` is used to configure `tox`.

#### Developing C extension without full package build

You can test the c extension without doing the entire build, this can speed up developing the C extension. (You need to have the development requirements installed)

```sh
cd src/eafpy/
# Compile the C extension with CFFI
py build_c_eaf.py
# Now you can open an interpretor and import the package c_bindings
```

### Building the documentation
#### Build doc in the CI
The CI will automatically build and deploy doc to the github pages when a new commit is added to the master

#### Build doc locally

The documentation uses sphinx, which can be build using the `make` command or with `sphinx-build`. `make` is preffered but requires a linux environment or something like [chocolatey](https://chocolatey.org/)

##### Linux - Make
```
pip install -r requirements_dev.txt
cd doc
make html
# Use this to remove the build
make clean
```
##### Python - sphinx-build
This should work on any operating system
```
pip install -r requirements_dev.txt
cd doc
sphinx-build -b html . _build
```
##### Windows - Make with WSL
I would reccomend [Windows Subsytem for Linux](https://learn.microsoft.com/en-us/windows/wsl/install) if you haven't tried it already
```
# If you haven't already, install wsl
wsl --install
# You might have to install a dist from the windows app store
wsl -d ubuntu
# You should now be in a WSL terminal. You might have to install pip first
pip install -r requirements_dev.txt
cd doc
make clean
make html
```
#### Building the automatic docstring for sphinx
To build the pages that are produced from docstrings, execute this
```
cd doc
sphinx-apidoc -f -o . ../src/
# Re-make the docs to see the changes
make clean
make html
```
This uses the sphinx `sphinxcontrib-napoleon` extension to build numpy docstrings
#### Editing the interactive examples
The examples uses the sphinx `myst-nb` extension to produce interactive plotly graphs inside executable jupyter, from markdown files. Look at this file as a template:

`doc/examples/plot_datasets_examples.md`

See [this](https://myst-nb.readthedocs.io/en/latest/authoring/text-notebooks.html) myst-nb for more information

[py-build-badge]: https://github.com/multi-objective/moocore/workflows/Python/badge.svg
[py-build-link]: https://github.com/multi-objective/moocore/actions/workflows/python.yaml
[py-coverage-badge]: https://codecov.io/gh/multi-objective/moocore/branch/main/graph/badge.svg?flag=python
[py-coverage-link]: https://codecov.io/gh/multi-objective/moocore/tree/main/python
[py-moocore-github]: https://github.com/multi-objective/moocore/tree/main/python#readme
[py-moocore-homepage]: https://multi-objective.github.io/moocore/python
