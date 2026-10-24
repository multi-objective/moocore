.. _bench-crossover-blend-alpha:

Blend crossover (BLX-α)
=======================

:func:`~moocore.blend_alpha_crossover` samples each variable of the offspring
uniformly from the interval between the parents, widened on each side by
``alpha`` times its length.  `DEAP`_ (``cxBlend``), `jMetalPy`_
(``BLXAlphaCrossover``) and `DESDEO`_ (``BlendAlphaCrossover``) provide it.

`moocore`_ produces one offspring per pair of parents, and the other packages
produce two.  The two offspring of `DEAP`_ mirror each other, so the KS tests
use the first offspring of each pair from `DEAP`_ and `jMetalPy`_, and every
offspring from `DESDEO`_, whose offspring are independent.  The timings call
`moocore`_ twice, so that every package produces the same number of offspring.

The packages handle bounds differently.  `moocore`_ and `DESDEO`_ clip the
sampling interval to the bounds, `jMetalPy`_ clips each sampled value, and
`DEAP`_ ignores the bounds.  The KS tests use bounds that never bind.

KS test results
---------------

.. csv-table::
   :file: /_static/bench/crossover_ks-blend_alpha.csv
   :header-rows: 1
   :stub-columns: 1

All implementations produce the same distribution of offspring.  The p-value of
0.014 between `jMetalPy`_ and `DESDEO`_ is one of the low values expected by
chance: with two other random seeds, it was 0.07 and 0.31.

Performance over number of parents
----------------------------------

.. image:: /_static/bench/crossover_bench-blend_alpha-nparents-time.png
   :width: 80%

With 10 parents, `DEAP`_ is faster than the two calls to `moocore`_.  From 20
parents on, `moocore`_ is the fastest.  With 10 000 parents, it takes 2.1 ms,
while `DEAP`_, `DESDEO`_ and `jMetalPy`_ take 7.8, 15 and 51 times as long.

Performance over number of variables
------------------------------------

.. image:: /_static/bench/crossover_bench-blend_alpha-nvars-time.png
   :width: 80%

With 2 variables, `DEAP`_ is faster than `moocore`_.  With more variables,
`moocore`_ is the fastest.  With 1000 variables, `DEAP`_, `DESDEO`_ and
`jMetalPy`_ take 6.8, 8.1 and 170 times as long.  The time of `jMetalPy`_ grows
much faster than linearly with the number of variables, because its solutions
copy their list of variables on every access.

.. include:: /benchmarks-links.inc.rst
