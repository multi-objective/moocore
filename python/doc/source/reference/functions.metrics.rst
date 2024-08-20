Unary quality metrics
=====================

.. currentmodule:: moocore


.. _igd_hausdorf:

Inverted Generational Distance (IGD and IGD+) and Averaged Hausdorff Distance
-----------------------------------------------------------------------------

.. autosummary::
   :toctree: generated/

   igd
   igd_plus
   avg_hausdorff_dist

Functions to compute the inverted generational distance (IGD and IGD+) and the
averaged Hausdorff distance between nondominated sets of points.

The generational distance (GD) of a set :math:`A` is defined as the distance
between each point :math:`a \in A` and the closest point :math:`r` in a reference
set :math:`R`, averaged over the size of :math:`A`. Formally,

.. math::
   GD_p(A,R) = \left(\frac{1}{|A|}\sum_{a\in A}\min_{r\in R} d(a,r)^p\right)^{\frac{1}{p}}

where the distance in our implementation is the Euclidean distance:

.. math::
   d(a,r) = \sqrt{\sum_{k=1}^M (a_k - r_k)^2}

The inverted generational distance (IGD) is calculated as :math:`IGD_p(A,R) = GD_p(R,A)`.

The modified inverted generational distanced (IGD+) was proposed by
:cite:t:`IshMasTanNoj2015igd` to ensure that IGD+ is weakly Pareto compliant,
similarly to :func:`epsilon_additive` or :func:`epsilon_mult`. It modifies the
distance measure as:

.. math::
   d^+(r,a) = \sqrt{\sum_{k=1}^M (\max\{r_k - a_k, 0\})^2}

The average Hausdorff distance (:math:`\Delta_p`) was proposed by
:cite:t:`SchEsqLarCoe2012tec` and it is calculated as:

.. math::

   \Delta_p(A,R) = \max\{ IGD_p(A,R), IGD_p(R,A) \}

IGDX :cite:p:`ZhoZhaJin2009igdx` is the application of IGD to decision vectors
instead of objective vectors to measure closeness and diversity in decision
space. One can use the functions :func:`igd` or :func:`igd_plus` (recommended)
directly, just passing the decision vectors as ``data``.

There are different formulations of the GD and IGD metrics in the literature
that differ on the value of :math:`p`, on the distance metric used and on
whether the term :math:`|A|^{-1}` is inside (as above) or outside the exponent
:math:`1/p`.  GD was first proposed by :cite:t:`VelLam1998gp` with :math:`p=2`
and the term :math:`|A|^{-1}` outside the exponent. IGD seems to have been
mentioned first by :cite:t:`CoeSie2004igd`, however, some people also used the
name D-metric for the same concept with :math:`p=1` and later papers have often
used IGD/GD with :math:`p=1`. :cite:t:`SchEsqLarCoe2012tec` proposed to place
the term :math:`|A|^{-1}` inside the exponent, as in the formulation shown
above.  This has a significant effect for GD and less so for IGD given a
constant reference set. IGD+ also follows this formulation.  We refer to
:cite:t:`IshMasTanNoj2015igd` and :cite:t:`BezLopStu2017emo` for a more
detailed historical perspective and a comparison of the various variants.

Following :cite:t:`IshMasTanNoj2015igd`, we always use :math:`p=1` in our
implementation of IGD and IGD+ because (1) it is the setting most used in
recent works; (2) it makes irrelevant whether the term :math:`|A|^{-1}` is
inside or outside the exponent :math:`1/p`; and (3) the meaning of IGD becomes
the average Euclidean distance from each reference point to its nearest
objective vector. It is also slightly faster to compute.

GD should never be used directly to compare the quality of approximations to a
Pareto front, as it often contradicts Pareto optimality (it is not weakly
Pareto-compliant). We recommend IGD+ instead of IGD, since the latter
contradicts Pareto optimality in some cases (see examples in :func:`igd_plus`) whereas IGD+
is weakly Pareto-compliant, but we implement IGD here because it is still
popular due to historical reasons.

The average Hausdorff distance (:math:`\Delta_p(A,R)`) is also not weakly
Pareto-compliant, as shown in the examples in :func:`igd_plus`.




.. _epsilon_metric:

Epsilon metric
--------------

.. autosummary::
   :toctree: generated/

   epsilon_additive
   epsilon_mult

The epsilon metric of a set :math:`A` with respect to a reference set :math:`R`
is defined as :cite:p:`ZitThiLauFon2003:tec`

.. math::
   epsilon(A,R) = \max_{r \in R} \min_{a \in A} \max_{1 \leq i \leq n} epsilon(a_i, r_i)

where :math:`a` and :math:`b` are objective vectors and, in the case of
minimization of objective :math:`i`, :math:`epsilon(a_i,b_i)` is computed as
:math:`a_i/b_i` for the multiplicative variant (respectively, :math:`a_i - b_i`
for the additive variant), whereas in the case of maximization of objective
:math:`i`, :math:`epsilon(a_i,b_i) = b_i/a_i` for the multiplicative variant
(respectively, :math:`b_i - a_i` for the additive variant). This allows
computing a single value for problems where some objectives are to be
maximized while others are to be minimized. Moreover, a lower value
corresponds to a better approximation set, independently of the type of
problem (minimization, maximization or mixed). However, the meaning of the
value is different for each objective type. For example, imagine that
objective 1 is to be minimized and objective 2 is to be maximized, and the
multiplicative epsilon computed here for :math:`epsilon(A,R) = 3`. This means
that :math:`A` needs to be multiplied by 1/3 for all :math:`a_1` values and by 3
for all :math:`a_2` values in order to weakly dominate :math:`R`. The
computation of the multiplicative version for negative values doesn't make
sense.

Computation of the epsilon indicator requires :math:`O(n \cdot |A| \cdot
|R|)`, where :math:`n` is the number of objectives (dimension of vectors).



Hypervolume metric
------------------

.. autosummary::
   :toctree: generated/

   hypervolume
   whv_hype
   total_whv_rect
   whv_rect


The hypervolume of a set of multidimensional points :math:`A` with respect to a reference point :math:`\vec{r}` is the volume of the region dominated by the set and bounded by the reference point :cite:p:`ZitThi1998ppsn`.


Bibliography
------------

.. bibliography::


