.. _variation_operators:

===================
Variation operators
===================

.. currentmodule:: moocore

Variation operators produce new decision vectors, called offspring, from
existing ones, called parents.
This usually happens in two steps: first, a crossover operator combines the
parents to produce one or more offspring, and then a mutation operator randomly
changes some of the offspring's decision variables. We currently provide a
number of crossover operators.
Every operator takes a 2D array with one
decision vector per row and accepts a ``seed`` for reproducible results.  The
:ref:`crossover example <sphx_glr_auto_examples_plot_crossover.py>` shows
the behaviour of the crossover operators.

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
many offspring it produces:

.. list-table::
   :header-rows: 1

   * - Function
     - Offspring per operation
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
cross over rows ``2i`` and ``2i + 1``, so the order of the rows decides which
parents are crossed over, for example after a selection step.  If the number
of rows is odd, the last row is crossed over with the first one.
When an
operation produces 2 offspring, the output has as many rows as ``parents``, and
one offspring is dropped when the number of rows is odd.
:func:`blend_alpha_crossover` returns one row per pair.

The functions that use 1 parent per operation take a matrix ``population`` and
the row indices ``targets`` of the parents, and return one offspring per target.
They choose the other parents at random from ``population``.

Differential evolution (DE) comes in two variants that are common in
multi-objective optimization.  :func:`de_rand_1_bin_crossover` is the
DE/rand/1/bin scheme of :footcite:t:`StoPri1997:de`, which most Pareto-based
multi-objective DE algorithms use :footcite:p:`BezLopStu2015emode`.
:func:`de_current_1_crossover` is the operator of MOEA/D-DE
:footcite:p:`LiZha2009:moead-de`.  It starts from the target instead of a
random row and does not force any variable to change.  Its 2 other parents may
include the target itself.  `Tanabe and Ishibuchi (2019)
<https://doi.org/10.1007/s00500-019-03842-6>`__ call this mutation the current/1
strategy, a term introduced by `Gong et al. (2017)
<https://doi.org/10.1109/TEVC.2017.2670779>`__.

Certain crossover operators tend to produce offspring that are close to one
or the other parent.
Operators that produce 2 offspring from 2 parents or 1 offspring from 1 parent
preserve the order, so that the index of an offspring corresponds to the index
of the parent it is closest to, assuming a parent-centric crossover operation.
For the operators that take ``population``, the parent of offspring ``i`` is
row ``targets[i]``.


Random seeds
============

Every operator takes a ``seed``.  With an integer seed, the operator draws the
same random numbers on every call.  This makes a single call reproducible, but
in an evolutionary algorithm every generation would reuse the same random
numbers.  Instead, create one :class:`numpy.random.Generator` at the start of a
run and pass it as ``seed`` to every call.  Each call then draws a new seed
from the generator, and the seed of the generator still reproduces the whole
run::

    rng = np.random.default_rng(42)
    for generation in range(n_generations):
        parents = ...  # Select and order the parents.
        offspring = moocore.uniform_crossover(parents, seed=rng)
        ...

With ``seed=None``, every call uses a new random seed, so the results cannot be
reproduced.


Bibliography
============

.. footbibliography::
