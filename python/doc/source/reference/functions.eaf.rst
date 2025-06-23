===================================
Empirical Attainment Function (EAF)
===================================

.. currentmodule:: moocore

.. _eaf_computation:

EAF Computation
===============
.. autosummary::
   :toctree: generated/

   eaf
   eafdiff
   largest_eafdiff

Given a set :math:`A \subset \mathbb{R}^d`, the attainment function of
:math:`A`, denoted by :math:`\alpha_{A}\colon \mathbb{R}^d\to \{0,1\}`,
specifies which points in the objective space are weakly dominated by
:math:`A`, where :math:`\alpha_A(\vec{z}) = 1` if :math:`\exists \vec{a} \in A,
\vec{a} \leq \vec{z}`, and :math:`\alpha_A(\vec{z}) = 0`, otherwise.

Let :math:`\mathcal{A} = \{A_1, \dots, A_n\}` be a multi-set of :math:`n` sets
:math:`A_i \subset \mathbb{R}^d`, the EAF
:footcite:p:`Grunert01,GruFon2009:emaa` is the function
:math:`\hat{\alpha}_{\mathcal{A}}\colon \mathbb{R}^d\to [0,1]`, such that:

.. math::
   \hat{\alpha}_{\mathcal{A}}(\vec{z}) = \frac{1}{n}\sum_{i=1}^n \alpha_{A_i}(\vec{z})

The EAF is a coordinate-wise non-decreasing step function, similar to the
empirical cumulative distribution function (ECDF)
:footcite:p:`LopVerDreDoe2025`.  Thus, a finite representation of the EAF can
be computed as the set of minima, in terms of Pareto optimality, with a value
of the EAF not smaller than a given :math:`t/n`, where :math:`t=1,\dots,n`
:footcite:p:`FonGueLopPaq2011emo`. Formally, the EAF can be represented by the
sequence :math:`(L_1, L_2, \dots, L_n)`, where:

.. math::
   L_t = \min \{\vec{z} \in \mathbb{R}^d : \hat{\alpha}_{\mathcal{A}}(\vec{z}) \geq t/n\}

It is also common to refer to the :math:`k\% \in [0,100]` percentile. For
example, the *median* (or 50\%) attainment surface corresponds to
:math:`L_{\lfloor n/2\rfloor}` and it is the lower boundary of the vector space
attained by at least 50\% of the input sets :math:`A_i`. Similarly, :math:`L_1`
is called the *best* attainment surface (:math:`\frac{1}{n}`\%) and represents
the lower boundary of the space attained by at least one input set, whereas
:math:`L_{100}` is called the *worst* attainment surface (100\%) and represents
the lower boundary of the space attained by all input sets.



.. _vorobev_expectation_and_deviation:

Vorob'ev Expectation and Deviation
==================================
.. autosummary::
   :toctree: generated/

   vorob_t
   vorob_dev


Let :math:`\mathcal{A} = \{A_1, \dots, A_n\}` be a multi-set of :math:`n` sets
:math:`A_i \subset \mathbb{R}^d` of mutually nondominated vectors, with finite
(but not necessarily equal) cardinality.  If bounded by a reference point
:math:`\vec{r}` that is strictly dominated by any point in any set, then these
sets can be seen a samples from a random closed set
:footcite:p:`Molchanov2005theory`.

Let the :math:`\beta`-quantile be the subset of the empirical attainment
function :math:`\mathcal{Q}_\beta = \{\vec{z}\in \mathbb{R}^d :
\hat{\alpha}_{\mathcal{A}}(\vec{z}) \geq \beta\}`.

The Vorob'ev *expectation* is the :math:`\beta^{*}`-quantile set
:math:`\mathcal{Q}_{\beta^{*}}` such that the mean value hypervolume of the
sets is equal (or as close as possible) to the hypervolume of
:math:`\mathcal{Q}_{\beta^{*}}`, that is, :math:`\text{hyp}(\mathcal{Q}_\beta)
\leq \mathbb{E}[\text{hyp}(\mathcal{A})] \leq
\text{hyp}(\mathcal{Q}_{\beta^{*}})`, :math:`\forall \beta > \beta^{*}`. Thus,
the Vorob'ev expectation provides a definition of the notion of *mean*
nondominated set.

The value :math:`\beta^{*} \in [0,1]` is called the Vorob'ev *threshold*. Large
differences from the median quantile (0.5) indicate a skewed distribution of
:math:`\mathcal{A}`.

The Vorob'ev *deviation* is the mean hypervolume of the symmetric difference
between the Vorob'ev expectation and any set in :math:`\mathcal{A}`, that is,
:math:`\mathbb{E}[\text{hyp}(\mathcal{Q}_{\beta^{*}} \ominus \mathcal{A})]`,
where the symmetric difference is defined as :math:`A \ominus B = (A
\setminus B) \cup (B \setminus A)`.  Low deviation values indicate that the
sets are very similar, in terms of the location of the weakly dominated
space, to the Vorob'ev expectation.

For more background, see
:footcite:t:`BinGinRou2015gaupar,Molchanov2005theory,CheGinBecMol2013moda`.


Bibliography
============

.. footbibliography::
