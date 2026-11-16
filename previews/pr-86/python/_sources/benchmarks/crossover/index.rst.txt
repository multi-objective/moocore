.. _bench-crossover:

Crossover operators
===================

These pages compare the :ref:`crossover operators <variation_operators>` of
`moocore`_ with the equivalent operators of `pymoo`_, `DEAP`_, `jMetalPy`_ and
`DESDEO`_, when they exist.  Each page compares the offspring that the packages
produce and the time they take.

The KS test results show whether two packages produce offspring with the same
distribution.  Each package crosses over the same parents 100 000 times, with
bounds wide enough that they never bind, so the results do not depend on how
each package handles bounds.  For every pair of packages and every decision
variable, a two-sample Kolmogorov-Smirnov test compares their offspring.  Each
table shows the smallest p-value over the variables.  A p-value below 0.001
means that the distributions differ.

The performance plots time each operator for an increasing number of parents
with 30 variables, and for an increasing number of variables with 100 parents.
Each package receives the parents in its own data structure, which is created
before the timing starts.  When a package expects the caller to choose the
other parents of DE or PCX, choosing them is part of the time.

.. toctree::
   :maxdepth: 1

   uniform
   single_point
   blend_alpha
   single_arithmetic
   local
   bounded_exponential
   de_rand_1_bin
   de_current_1
   parent_centric

.. include:: /benchmarks-links.inc.rst
