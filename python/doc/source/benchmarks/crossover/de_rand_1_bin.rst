.. _bench-crossover-de-rand-1-bin:

Differential evolution: DE/rand/1/bin
=====================================

:func:`~moocore.de_rand_1_bin_crossover` adds a scaled difference of two random
members of the population to a third one, and takes each variable of the
offspring from this mutant or from the target.  One random variable always
comes from the mutant.  `pymoo`_ (``VariantDE``), `jMetalPy`_
(``DifferentialEvolutionCrossover``) and `DESDEO`_
(``DifferentialEvolutionCrossover``) provide this operator.  `pymoo`_ forces a
variable to come from the mutant only when no variable was chosen, instead of
forcing a random variable in every offspring.

KS test results
---------------

.. csv-table::
   :file: /_static/bench/crossover_ks-de_rand_1_bin.csv
   :header-rows: 1
   :stub-columns: 1

`moocore`_, `jMetalPy`_ and `DESDEO`_ produce the same distribution of
offspring.  `pymoo`_ differs from all three because of its rule for the
forced variable.  With 3 variables and :math:`CR = 0.9`, it keeps the value of
the target in 13% to 14% of the variables, against 9% to 11% in the other
packages.

Performance over number of parents
----------------------------------

.. image:: /_static/bench/crossover_bench-de_rand_1_bin-nparents-time.png
   :width: 80%

`moocore`_ is the fastest for every number of parents.  With 10 000 parents,
it takes 1.9 ms, while `DESDEO`_, `pymoo`_ and `jMetalPy`_ take 2.2, 17 and 140
times as long.

Performance over number of variables
------------------------------------

.. image:: /_static/bench/crossover_bench-de_rand_1_bin-nvars-time.png
   :width: 80%

`moocore`_ is the fastest for every number of variables.  With 1000 variables,
`pymoo`_, `DESDEO`_ and `jMetalPy`_ take 2.6, 6.5 and 550 times as long.

.. include:: /benchmarks-links.inc.rst
