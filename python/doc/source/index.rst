.. _moocore_docs_mainpage:

#########################################################
moocore: Core Algorithms for Multi-Objective Optimization
#########################################################

.. toctree::
   :maxdepth: 1
   :hidden:

   API reference <reference/index>
   Examples <examples/index>

**Version**: |version|

**Date** |today|

**Useful links**:
`Source Repository <https://github.com/multi-objective/moocore>`_ |
`Issue Tracker <https://github.com/multi-objective/moocore/issues>`_

The goal of **moocore** is to collect fast implementations of core mathematical functions and algorithms for multi-objective optimization. These functions include:

* Identifying and filtering dominated vectors.
* Quality metrics such as (weighted) hypervolume, epsilon, IGD, etc.
* Computation of the Empirical Attainment Function. The empirical attainment
  function (EAF) describes the probabilistic distribution of the outcomes
  obtained by a stochastic algorithm in the objective space.

**Keywords**: empirical attainment function, summary attainment surfaces, EAF
differences, multi-objective optimization, bi-objective optimization,
performance measures, performance assessment

.. grid:: 2
    :gutter: 4
    :padding: 2 2 0 0
    :class-container: sd-text-center


    .. grid-item-card:: API Reference
        :img-top: _static/index_user_guide.svg
        :class-card: intro-card
        :shadow: md
        :link: reference
        :link-type: ref

        The reference guide contains a detailed description of the functions,
        modules, and objects.

    .. grid-item-card:: Examples
        :img-top: _static/index_getting_started.svg
        :class-card: intro-card
        :shadow: md
        :link: examples
        :link-type: ref

        Detailed examples and tutorials.

.. This is not really the index page, that is found in
   _templates/indexcontent.html The toctree content here will be added to the
   top of the template header
