.. _bench-crossover-bounded-exponential:

Bounded exponential crossover (BEX)
===================================

:func:`~moocore.bounded_exponential_crossover` moves each offspring away from
its parent by a random step that follows an exponential distribution truncated
at the bounds.

KS test results
---------------

.. csv-table::
   :file: /_static/bench/crossover_ks-bounded_exponential.csv
   :header-rows: 1
   :stub-columns: 1

Both implementations produce the same distribution of offspring.

Performance over number of parents
----------------------------------

.. image:: /_static/bench/crossover_bench-bounded_exponential-nparents-time.png
   :width: 80%

`DESDEO`_ takes 20 times as long as `moocore`_ with 10 parents, and 1.2 times
as long with 10 000 parents.

Performance over number of variables
------------------------------------

.. image:: /_static/bench/crossover_bench-bounded_exponential-nvars-time.png
   :width: 80%

`DESDEO`_ takes 3.7 to 15 times as long as `moocore`_.

.. include:: /benchmarks-links.inc.rst
