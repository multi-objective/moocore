.. _bench-crossover-de-current-1:

Differential evolution: DE/current/1
====================================

:func:`~moocore.de_current_1_crossover` adds a scaled difference of two random
members of the population to the target, as in MOEA/D-DE.  `pymoo`_ provides
it as ``DEX`` with ``at_least_once=False``, when the target is given as the
first parent.  `jMetalPy`_ provides it through the order of the parents of
``DifferentialEvolutionCrossover``, as its MOEA/D does.  `jMetalPy`_ always
takes one random variable from the mutant, so it equals this operator only
with :math:`CR = 1`, which is the default.  The version of `DESDEO`_ on PyPI
does not provide this operator.

KS test results
---------------

.. csv-table::
   :file: /_static/bench/crossover_ks-de_current_1.csv
   :header-rows: 1
   :stub-columns: 1

`moocore`_ and `jMetalPy`_ produce the same distribution of offspring.
`pymoo`_ multiplies the scaling factor by a random factor within :math:`1 \pm
5 \cdot 10^{-5}`, so its offspring are within :math:`2 \cdot 10^{-5}` of the
values that the other packages produce, but never equal to them.  The KS test
detects this.  After rounding the offspring to three decimals, the p-values
between `moocore`_ and `pymoo`_ are 0.43, 0.59 and 0.98 for the three
variables.

Performance over number of parents
----------------------------------

.. image:: /_static/bench/crossover_bench-de_current_1-nparents-time.png
   :width: 80%

`moocore`_ is the fastest for every number of parents.  With 10 000 parents,
it takes 1.2 ms, while `pymoo`_ and `jMetalPy`_ take 33 and 230 times as
long.

Performance over number of variables
------------------------------------

.. image:: /_static/bench/crossover_bench-de_current_1-nvars-time.png
   :width: 80%

`moocore`_ is the fastest for every number of variables.  With 1000 variables,
`pymoo`_ and `jMetalPy`_ take 2.7 and 950 times as long.

.. include:: /benchmarks-links.inc.rst
