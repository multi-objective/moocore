.. _unary_quality_metrics:

=====================
Unary quality metrics
=====================

.. currentmodule:: moocore


.. _igd_hausdorf:

Inverted Generational Distance (IGD and IGD+) and Averaged Hausdorff Distance
=============================================================================

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
   d(a,r) = \sqrt{\sum_{k=1}^m (a_k - r_k)^2}

The inverted generational distance (IGD) is calculated as :math:`IGD_p(A,R) = GD_p(R,A)`.

The modified inverted generational distanced (IGD+) was proposed by
:cite:t:`IshMasTanNoj2015igd` to ensure that IGD+ is weakly Pareto compliant,
similarly to :func:`epsilon_additive` or :func:`epsilon_mult`. It modifies the
distance measure as:

.. math::
   d^+(r,a) = \sqrt{\sum_{k=1}^m (\max\{r_k - a_k, 0\})^2}

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
Pareto front, it is not weakly Pareto-compliant and it often contradicts Pareto
optimality.

IGD is still popular due to historical reasons, but we strongly recommend IGD+
instead of IGD, because IGD contradicts Pareto optimality in some cases
(see examples in :func:`igd_plus`) whereas IGD+ is weakly Pareto-compliant.

The average Hausdorff distance (:math:`\Delta_p(A,R)`) is not weakly
Pareto-compliant, as shown in the examples in :func:`igd_plus`.


.. _epsilon_metric:

Epsilon metric
==============

.. autosummary::
   :toctree: generated/

   epsilon_additive
   epsilon_mult

The epsilon metric of a set :math:`A \subset \mathbb{R}^m` with respect to a reference set :math:`R \subset \mathbb{R}^m`
is defined as :cite:p:`ZitThiLauFon2003:tec`

.. math::
   epsilon(A,R) = \max_{r \in R} \min_{a \in A} \max_{1 \leq i \leq m} epsilon(a_i, r_i)

where :math:`a` and :math:`r` are objective vectors of length :math:`m`.

In the case of minimization of objective :math:`i`, :math:`epsilon(a_i,r_i)` is
computed as :math:`a_i/r_i` for the multiplicative variant (respectively,
:math:`a_i - r_i` for the additive variant), whereas in the case of
maximization of objective :math:`i`, :math:`epsilon(a_i,r_i) = r_i/a_i` for the
multiplicative variant (respectively, :math:`r_i - a_i` for the additive
variant).  This allows computing a single value for problems where some
objectives are to be maximized while others are to be minimized. Moreover, a
lower value corresponds to a better approximation set, independently of the
type of problem (minimization, maximization or mixed). However, the meaning of
the value is different for each objective type. For example, imagine that
objective 1 is to be minimized and objective 2 is to be maximized, and the
multiplicative epsilon computed here for :math:`epsilon(A,R) = 3`. This means
that :math:`A` needs to be multiplied by 1/3 for all :math:`a_1` values and by
3 for all :math:`a_2` values in order to weakly dominate :math:`R`.

The multiplicative variant can be computed as :math:`\exp(epsilon_{+}(\log(A),
\log(R)))`, which makes clear that the computation of the multiplicative
version for zero or negative values doesn't make sense. See the examples in
:func:`epsilon_additive`.

The current implementation uses the naive algorithm that requires :math:`O(m \cdot |A| \cdot
|R|)`, where :math:`m` is the number of objectives (dimension of vectors).


.. _hypervolume_metric:

Hypervolume metric
==================

.. autosummary::
   :toctree: generated/

   hypervolume
   Hypervolume
   RelativeHypervolume
   hv_contributions
   total_whv_rect
   whv_rect


The hypervolume of a set of multidimensional points :math:`A \subset
\mathbb{R}^m` with respect to a reference point :math:`\vec{r} \in \mathbb{R}^m`
is the volume of the region dominated by the set and bounded by the reference
point :cite:p:`ZitThi1998ppsn`.  Points in :math:`A` that do not strictly
dominate :math:`\vec{r}` do not contribute to the hypervolume value, thus,
ideally, the reference point must be strictly dominated by all points in the
true Pareto front.

More precisely, the hypervolume is the `Lebesgue measure <https://en.wikipedia.org/wiki/Lebesgue_measure>`_ of the union of
axis-aligned hyperrectangles
(`orthotopes <https://en.wikipedia.org/wiki/Hyperrectangle>`_), where each
hyperrectangle is defined by one point from :math:`\vec{a} \in A` and the
reference point.  The union of axis-aligned hyperrectangles is also called an
*orthogonal polytope*.

The hypervolume is compatible with Pareto-optimality
:cite:p:`KnoCor2002cec,ZitThiLauFon2003:tec`, that is, :math:`\nexists A,B
\subset \mathbb{R}^m`, such that :math:`A` is better than :math:`B` in terms of
Pareto-optimality and :math:`\text{hyp}(A) \leq \text{hyp}(B)`. In other words,
if a set is better than another in terms of Pareto-optimality, the hypervolume
of the former must be strictly larger than the hypervolume of the latter.
Conversely, if the hypervolume of a set is larger than the hypervolume of
another, then we know for sure than the latter set cannot be better than the
former in terms of Pareto-optimality.

Like most measures of unions of high-dimensional geometric objects, computing the hypervolume is #P-hard :cite:p:`BriFri2010approx`.


.. _hv_approximation:

Approximating the hypervolume metric
====================================

.. autosummary::
   :toctree: generated/

   hv_approx
   whv_hype

Computing the hypervolume can be time consuming, thus several approaches have been proposed in the literature to approximate its value via Monte-Carlo or quasi-Monte-Carlo sampling :cite:p:`DenZha2019approxhv`. These methods are implemented in :func:`whv_hype` and :func:`hv_approx`.


.. _r2_indicator:

(Exact) R2 Indicator
=============================================================================

.. autosummary::
   :toctree: generated/

   r2_exact

The unary R2 indicator is a quality indicator for a set :math:`A \subset \mathbb{R}^m`
w.r.t. an ideal or utopian reference point :math:`\vec{r} \in \mathbb{R}^m`.
It was originally proposed by :cite:t:`HanJas1998` and is defined as the
expected Tchebycheff utility under a uniform distribution of weight vectors
(w.l.o.g. assuming minimization):

.. math::
   R2(A) := \int_{w \in W} \min_{a \in A} \left\{ \max_{i=1,\dots,m} w_i (a_i - r_i) \right\} \, dw,

where :math:`W` denotes the uniform distribution across weights:

.. math::
   W = \{w \in \mathbb R^m \mid w_i \geq 0, \sum_{i=1}^m w_i = 1\}.

The R2 indicator is to be minimized and has an optimal value of 0 when
:math:`\vec{r} \in A`.

The exact R2 indicator is strongly Pareto-compliant, i.e., compatible with Pareto-optimality:

.. math::
   \forall A, B \subset \mathbb R^m: A \prec B \Rightarrow R2(A) < R2(B).

Given an ideal or utopian reference ponint, which is available in most scenarios,
all non-dominated solutions always contribute to the value of the exact R2 indicator.
However, it is scale-dependent and care should be taken such that all objectives
contribute approximately equally to the indicator, e.g., by normalizing the
Pareto front to the unit hypercube.

:func:`r2_exact` computes the exact R2 indicator for bi-objective solution
sets in :math:`O(n \log n)` following :cite:t:`SchKer2025r2v2`.

Bibliography
============

.. bibliography::
