MOOCORE: Core Mathematical Functions for Multi-Objective Optimization
==============================================
<!-- badges: start -->
[![C build
status](https://github.com/multi-objective/moocore/workflows/C/badge.svg)](https://github.com/multi-objective/moocore/actions/workflows/C.yaml)
[![Python build
status](https://github.com/multi-objective/moocore/workflows/Python/badge.svg)](https://github.com/multi-objective/moocore/actions/workflows/python.yaml)
[![R build
status](https://github.com/multi-objective/moocore/workflows/R/badge.svg)](https://github.com/multi-objective/moocore/actions/workflows/R.yaml)
<!-- badges: end -->

[ [**GitHub**](https://multi-objective.github.io/moocore) ] [ [**R package**](https://multi-objective.github.io/moocore/r/) ] [ [**Python package**](https://multi-objective.github.io/moocore/python/) ]

**Contributors:**
    [Manuel López-Ibáñez](https://lopez-ibanez.eu),

Summary
-------

This is the C library and associated command-line tools of the [`moocore`](https://github.com/multi-objective/moocore** projects. This README describes just this component.

**TODO: complete the rest of this README**

nondominated
------------

Obtain information and perform some operations on the nondominated
sets given as input.


  Building `nondominated'
  -----------------------

The program has been tested on GNU/Linux using bash as shell and a
recent version of GCC (>= 4.2). If you have success or problems using
other systems, please let me know.

I recommend that you compile specifically for your architecture
using GCC option -march=. The default compilation is done with:

  make nondominated

This uses the option "-march=native". If your GCC version does not
support "native", you can give an explicit architecture:

  make nondominated march=i686

See the GCC manual for the names of the architectures supported by
your version of GCC.


  Using `nondominated'
  --------------------

See the output of

  nondominated --help


  License
  -------

See LICENSE file or contact the author.


  Contact
  -------

Manuel Lopez-Ibanez <manuel.lopez-ibanez@ulb.ac.be>
