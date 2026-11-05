.. _bench-crossover-uniform:

Uniform crossover
=================

:func:`~moocore.uniform_crossover` swaps each variable between the two
offspring with probability ``swap_prob``, which is 0.5 by default.  `pymoo`_
(``UniformCrossover``), `DEAP`_ (``cxUniform``) and `DESDEO`_
(``UniformCrossover``) provide the same operator.

KS test results
---------------

.. csv-table::
   :file: /_static/bench/crossover_ks-uniform.csv
   :header-rows: 1
   :stub-columns: 1

All implementations produce the same distribution of offspring.

Performance over number of parents
----------------------------------

.. image:: /_static/bench/crossover_bench-uniform-nparents-time.png
   :width: 80%

With 10 parents, the Python loop of `DEAP`_ over five pairs takes less time
than the fixed cost of a call to `moocore`_.  From 20 parents on, `moocore`_ is
the fastest.  With 10 000 parents, it takes 1.4 ms, while `DESDEO`_, `DEAP`_
and `pymoo`_ take 1.7, 4.4 and 20 times as long.

Performance over number of variables
------------------------------------

.. image:: /_static/bench/crossover_bench-uniform-nvars-time.png
   :width: 80%

`moocore`_ is the fastest for every number of variables.  With 1000 variables,
`pymoo`_, `DEAP`_ and `DESDEO`_ take 2.0, 3.9 and 7.4 times as long.

.. include:: /benchmarks-links.inc.rst
