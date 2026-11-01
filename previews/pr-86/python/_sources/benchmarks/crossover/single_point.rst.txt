.. _bench-crossover-single-point:

Single-point crossover
======================

:func:`~moocore.single_point_crossover` draws a cut point and exchanges the
tails of the two parents.  `pymoo`_ (``SinglePointCrossover``), `DEAP`_
(``cxOnePoint``) and `DESDEO`_ (``SinglePointBinaryCrossover``) provide the
same operator.  The operator of `DESDEO`_ only accepts binary variables, so all
packages cross over binary parents here: a parent of zeros and a parent of ones
in the KS tests, and random binary parents in the timings.

KS test results
---------------

.. csv-table::
   :file: /_static/bench/crossover_ks-single_point.csv
   :header-rows: 1
   :stub-columns: 1

All implementations produce the same distribution of offspring.

Performance over number of parents
----------------------------------

.. image:: /_static/bench/crossover_bench-single_point-nparents-time.png
   :width: 80%

With 10 and 20 parents, `DEAP`_ is faster than `moocore`_.  From 50 parents
on, `moocore`_ is the fastest.  With 10 000 parents, it takes 0.14 ms, while
`DESDEO`_, `DEAP`_ and `pymoo`_ take 9.8, 18 and 270 times as long.

Performance over number of variables
------------------------------------

.. image:: /_static/bench/crossover_bench-single_point-nvars-time.png
   :width: 80%

`moocore`_ is the fastest for every number of variables.  `DEAP`_ takes 2.5 to
5 times as long, and `pymoo`_ and `DESDEO`_ take 24 to 144 times as long.

.. include:: /benchmarks-links.inc.rst
