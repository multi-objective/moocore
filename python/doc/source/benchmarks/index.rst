.. _benchmarks:

Benchmarks
==========

The following pages compare the performance of `moocore`_, `pymoo`_, `BoTorch`_, `DEAP`_, `DESDEO`_, `paretoset`_, `Nevergrad`_, `jMetalPy`_, `fast-pareto`_, `Optuna`_, `moarchiving`_, and `patatune`_.

Other Python packages are not included in the comparison because they are based
on these packages for the functionality benchmarked, so they are **at least as
slow** as them. For example `Xopt`_ and `BoFire`_ use `BoTorch`_, `pysamoo`_ is
an extension of `pymoo`_.  We do not include functions from the Bayesian
optimization toolbox `trieste`_, because it is much slower than `BoTorch`_ and
too slow to run the benchmarks in a reasonable time
(https://github.com/secondmind-labs/trieste/issues/917).  Similarly, we do not
include `paretobench`_ because it is always slower than `BoTorch`_ for
identifying nondominated points and more than 2000 times slower than `moocore`_
for computing the hypervolume in 3D
(https://github.com/electronsandstuff/ParetoBench/issues/57), which is too slow
to include in the benchmarks.

Not all packages provide the same functionality. For example, `pymoo`_ does not
provide the :ref:`epsilon indicator <epsilon_metric>`. `BoTorch`_ and
`paretobench`_ only provide the hypervolume. `paretoset`_ and `fast-pareto`_
only identify nondominated points.  `DESDEO`_, `DEAP`_, and `jMetalPy`_ use
`moocore`_ for computing the hypervolume, but other functionality, such as
filtering dominated points, is still slower than `moocore`_.  Recent versions
of `seqme`_ and `pymoo`_ (≥0.6.2) already use `moocore`_ for most functionality
benchmarked here.

We would like to benchmark `pygmo`_, however, it is currently impossible to install using ``pip`` (See https://github.com/esa/pygmo2/issues/152).

We would also like to benchmark `platypus`_, but its API is too different from
other packages. For example, it only supports filtering dominated points by
creating a new list of points, but it cannot identify their position in the
input. It cannot calculate hypervolume without normalization.  It doesn't
support :obj:`~numpy.typing.ArrayLike` inputs.

The source code for the benchmarks below can be found at https://github.com/multi-objective/moocore/tree/main/python/benchmarks .

.. toctree::
   :maxdepth: 2

   dominance
   indicators
   crossover/index

.. include:: /benchmarks-links.inc.rst
