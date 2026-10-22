.. _variation_operators:

===================
Variation operators
===================

.. currentmodule:: moocore

Crossover operators combine decision vectors, called parents, to produce new
decision vectors, called children.  Every function takes a 2D array with one
decision vector per row and accepts a ``seed`` for reproducible results.  The
:ref:`crossover example <sphx_glr_auto_examples_plot_crossover.py>` shows where
each operator places its children.

Crossover
=========
.. autosummary::
   :toctree: generated/

   uniform_crossover
   single_point_crossover
   blend_alpha_crossover
   single_arithmetic_crossover
   local_crossover
   bounded_exponential_crossover
   de_rand_1_bin_crossover
   de_current_1_crossover
   parent_centric_crossover

The operators differ in how many parents one crossover operation uses and how
many children it produces:

.. list-table::
   :header-rows: 1

   * - Function
     - Children per operation
     - Parents per operation
   * - :func:`uniform_crossover`
     - 2
     - 2
   * - :func:`single_point_crossover`
     - 2
     - 2
   * - :func:`blend_alpha_crossover`
     - 1
     - 2
   * - :func:`single_arithmetic_crossover`
     - 2
     - 2
   * - :func:`local_crossover`
     - 2
     - 2
   * - :func:`bounded_exponential_crossover`
     - 2
     - 2
   * - :func:`de_rand_1_bin_crossover`
     - 1
     - 1, plus 3 other parents chosen at random
   * - :func:`de_current_1_crossover`
     - 1
     - 1, plus 2 other parents chosen at random
   * - :func:`parent_centric_crossover`
     - 1
     - 1, plus 2 other parents chosen at random

The functions that use 2 parents per operation take a matrix ``parents`` and
mate rows ``2i`` and ``2i + 1``, so the order of the rows decides which parents
mate, for example after a selection step.  If the number of rows is odd, the
last row mates with the first one.  When an operation produces 2 children, the
output has as many rows as ``parents``, and one child is dropped when the
number of rows is odd.  :func:`blend_alpha_crossover` returns one row per pair.

The functions that use 1 parent per operation take a matrix ``population`` and
the row indices ``targets`` of the parents, and return one child per target.
They choose the other parents at random from ``population``.

Differential evolution (DE) comes in two variants that are common in
multi-objective optimization.  :func:`de_rand_1_bin_crossover` is the
DE/rand/1/bin scheme of :footcite:t:`StoPri1997:de`, which most Pareto-based
multi-objective DE algorithms use :footcite:p:`BezLopStu2015emode`.
:func:`de_current_1_crossover` is the operator of MOEA/D-DE
:footcite:p:`LiZha2009:moead-de`.  It starts from the target instead of a
random row and does not force any variable to change.  Its 2 other parents may
include the target itself.


Bibliography
============

.. footbibliography::
