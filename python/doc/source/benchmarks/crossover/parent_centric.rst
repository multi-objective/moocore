.. _bench-crossover-parent-centric:

Parent-centric crossover (PCX)
==============================

:func:`~moocore.parent_centric_crossover` creates an offspring around each
target, from two more parents chosen at random.  `pymoo`_ (``PCX``) and
`DESDEO`_ (``ParentCentricCrossover``) provide the same operator.  `pymoo`_
replaces values outside the bounds by random values, while `moocore`_ and
`DESDEO`_ clip them.  The KS tests use bounds that never bind.  The PCX of
`platypus`_ keeps the offspring in the plane of the three parents and has half
the spread, so it is a different operator.

KS test results
---------------

.. csv-table::
   :file: /_static/bench/crossover_ks-parent_centric.csv
   :header-rows: 1
   :stub-columns: 1

All implementations produce the same distribution of offspring.

Performance over number of parents
----------------------------------

.. image:: /_static/bench/crossover_bench-parent_centric-nparents-time.png
   :width: 80%

`moocore`_ is the fastest for every number of parents.  With 10 000 parents,
it takes 4.0 ms, while `DESDEO`_ and `pymoo`_ take 3.7 and 12 times as long.

Performance over number of variables
------------------------------------

.. image:: /_static/bench/crossover_bench-parent_centric-nvars-time.png
   :width: 80%

`moocore`_ is the fastest for every number of variables.  With 1000 variables,
`pymoo`_ and `DESDEO`_ take 2.1 and 3.8 times as long.

.. include:: /benchmarks-links.inc.rst
