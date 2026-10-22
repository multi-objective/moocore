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

Two-parent crossover
====================
.. autosummary::
   :toctree: generated/

   uniform_crossover
   single_point_crossover
   blend_alpha_crossover
   single_arithmetic_crossover
   local_crossover
   bounded_exponential_crossover

These functions mate rows ``2i`` and ``2i + 1`` of ``parents``, so the order of
the rows decides which parents mate, for example after a selection step.  If
the number of rows is odd, the last row mates with the first one.  Most of
these operators produce two children per pair and return as many rows as
``parents``, dropping one child when the number of rows is odd.
:func:`blend_alpha_crossover` produces one child per pair.

Population-based crossover
==========================
.. autosummary::
   :toctree: generated/

   de_rand_1_bin_crossover
   de_current_1_crossover
   parent_centric_crossover

These functions produce one child for each target row of ``population`` and
draw the other parents at random from ``population``.

Differential evolution (DE) comes in two variants that are common in
multi-objective optimization.  :func:`de_rand_1_bin_crossover` is the
DE/rand/1/bin scheme of :footcite:t:`StoPri1997:de`, which most Pareto-based
multi-objective DE algorithms use :footcite:p:`BezLopStu2015emode`.
:func:`de_current_1_crossover` is the operator of MOEA/D-DE
:footcite:p:`LiZha2009:moead-de`.  It starts from the target instead of a
random row and does not force any variable to change.


Bibliography
============

.. footbibliography::
