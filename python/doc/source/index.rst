.. _moocore_docs_mainpage:

#########################################################
moocore: Core Algorithms for Multi-Objective Optimization
#########################################################

.. toctree::
   :maxdepth: 1
   :hidden:

   API reference <reference/index>
   Examples <auto_examples/index>
   whatsnew/index
   contribute/index

**Version**: |version| (:ref:`whatsnew`)

**Date** |today|

**Useful links**:
`Install <https://github.com/multi-objective/moocore/tree/main/python#install>`_ |
`Source Repository <https://github.com/multi-objective/moocore>`_ |
`Issue Tracker <https://github.com/multi-objective/moocore/issues>`_

This webpage documents the ``moocore`` Python package. There is also a  `moocore R package <https://multi-objective.github.io/moocore/r>`_.

The goal of the **moocore** project (https://github.com/multi-objective/moocore/) is to collect fast implementations of core mathematical functions and algorithms for multi-objective optimization and make them available to different programming languages via similar interfaces. These functions include:

* Identifying and filtering dominated vectors.
* Quality metrics such as (weighted) hypervolume, epsilon, IGD, etc.
* Computation of the Empirical Attainment Function. The empirical attainment function (EAF) describes the probabilistic distribution of the outcomes obtained by a stochastic algorithm in the objective space.

Most critical functionality is implemented in C, with the R and Python packages providing convenient interfaces to the C code.

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
        :link: api_reference
        :link-type: ref

        The reference guide contains a detailed description of the functions,
        modules, and objects.

    .. grid-item-card:: Examples
        :img-top: _static/index_getting_started.svg
        :class-card: intro-card
        :shadow: md
        :link: auto_examples
        :link-type: ref

        Detailed examples and tutorials.


Benchmarks
----------

The following plots compare the performance of `moocore`_, `pymoo`_, `BoTorch`_, `jMetalPy`_, and `DEAP-er`_. Other optimization packages are not included in the comparison because they are based on these packages so they are **at least as slow** as them. For example `Xopt`_ uses `BoTorch`_, `pysamoo`_ is an extension of `pymoo`_, and `DESDEO`_ uses `pymoo`_ internally.

Not all packages provide the same functionality. For example, `pymoo`_ does not provide the :ref:`epsilon indicator <epsilon_metric>` whereas `jMetalPy`_ does not provide the :ref:`IGD+ indicator <igd_hausdorf>`. `BoTorch`_ provides neither of them.

The following plots compare the speed of computing the :ref:`hypervolume indicator <hypervolume_metric>` in 3D and 4D. As the plots show, `moocore`_ is nearly 100 times faster than the other packages and 1000 times faster than `BoTorch`_ and, by extension, `Xopt`_, which is an order of magnitude slower than the second slowest.

|pic1| |pic2|

.. |pic1| image:: _static/hv_bench-DTLZLinearShape.3d-time.png
   :width: 49%

.. |pic2| image:: _static/hv_bench-DTLZLinearShape.4d-time.png
   :width: 49%


The following plots compare the speed of finding nondominated solutions, equivalent to :func:`moocore.is_nondominated`, in 2D and 3D. As the plots show, `moocore`_ is nearly 100 times faster in 2D and 1000 times faster in 3D than the other packages. Here, `pymoo`_ quickly becomes even slower than `BoTorch`_.

|pic3| |pic4|

.. |pic3| image:: _static/ndom_bench-test2D-200k-time.png
   :width: 49%

.. |pic4| image:: _static/ndom_bench-ran3d-10k-time.png
   :width: 49%


The following plot compares the speed of computing the :ref:`epsilon indicator  <epsilon_metric>` metric and :ref:`IGD+ indicator <igd_hausdorf>`. Despite the algorithms for computing these metrics are relatively simple and easy to vectorize in Python, the `moocore`_ implementation is still 10 to 100 times faster.

|pic5| |pic6|

.. |pic5| image:: _static/eps_bench-rmnk_10D_random_search-time.png
   :width: 48%

.. |pic6| image:: _static/igd_plus_bench-ran.40000pts.3d-time.png
   :width: 48%


The source code for the benchmarks above can be found at https://github.com/multi-objective/moocore/tree/main/python/benchmarks .


.. _moocore: https://multi-objective.github.io/moocore/python/
.. _pymoo: https://pymoo.org/
.. _BoTorch: https://botorch.org/
.. _jMetalPy: https://jmetal.github.io/jMetalPy/index.html
.. _DEAP-er: https://deap-er.readthedocs.io/en/latest/
.. _Xopt: https://xopt.xopt.org/index.html
.. _pysamoo: https://anyoptimization.com/projects/pysamoo/
.. _DESDEO: https://desdeo.readthedocs.io/en/latest/

.. This is not really the index page, that is found in
   _templates/indexcontent.html The toctree content here will be added to the
   top of the template header
