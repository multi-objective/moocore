.. _bench-crossover-local:

Local crossover
===============

:func:`~moocore.local_crossover` draws a random weight for each variable and
sets the two offspring to the two weighted averages of the parents.  Only
`DESDEO`_ (``LocalCrossover``) provides this operator.  The
``ArithmeticCrossover`` of `jMetalPy`_ draws one weight for all the variables,
so it is a different operator.

KS test results
---------------

.. csv-table::
   :file: /_static/bench/crossover_ks-local.csv
   :header-rows: 1
   :stub-columns: 1

Both implementations produce the same distribution of offspring.

Performance over number of parents
----------------------------------

.. image:: /_static/bench/crossover_bench-local-nparents-time.png
   :width: 80%

`DESDEO`_ takes 39 to 70 times as long as `moocore`_.

Performance over number of variables
------------------------------------

.. image:: /_static/bench/crossover_bench-local-nvars-time.png
   :width: 80%

`DESDEO`_ takes 28 to 71 times as long as `moocore`_.

.. include:: /benchmarks-links.inc.rst
