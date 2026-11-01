.. _bench-crossover-single-arithmetic:

Single arithmetic crossover
===========================

:func:`~moocore.single_arithmetic_crossover` sets one random variable of both
offspring to the average of the parents and copies the other variables.  Only
`DESDEO`_ (``SingleArithmeticCrossover``) provides this operator.

KS test results
---------------

.. csv-table::
   :file: /_static/bench/crossover_ks-single_arithmetic.csv
   :header-rows: 1
   :stub-columns: 1

Both implementations produce the same distribution of offspring.

Performance over number of parents
----------------------------------

.. image:: /_static/bench/crossover_bench-single_arithmetic-nparents-time.png
   :width: 80%

`DESDEO`_ takes 16 to 69 times as long as `moocore`_.

Performance over number of variables
------------------------------------

.. image:: /_static/bench/crossover_bench-single_arithmetic-nvars-time.png
   :width: 80%

`DESDEO`_ takes 31 to 164 times as long as `moocore`_.

.. include:: /benchmarks-links.inc.rst
