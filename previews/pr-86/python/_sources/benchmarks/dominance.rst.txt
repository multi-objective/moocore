Nondominated points and sorting
===============================

.. _bench-ndom:

Identifying nondominated points
-------------------------------

The following plots compare the speed of finding nondominated solutions, equivalent to :func:`moocore.is_nondominated`. We test both ``keep_weakly=False`` (the default) and ``keep_weakly=True``. The former is only supported by `BoTorch`_ and `paretoset`_. Thus, for the purpose of benchmarking the other packages with ``keep_weakly=False``, we remove duplicated points from the data and exclude this time from the results. This makes all packages, except `moocore`_, `BoTorch`_ and `paretoset`_, seem faster than they actually are, because the user is forced to remove duplicated points when using them.

For 2D and 3D, the plots show that `moocore`_ is 10 times faster than `DESDEO`_ and `paretoset`_ and 100-1000 times faster than the other packages. `fast-pareto`_ claims to implement a :math:`O(n\log n)` algorithm for 3D, but the benchmarks below indicate a :math:`O(n^2)` complexity, similar to other packages and significantly slower than `moocore`_.

|ndom_bench-test2D-200k| |ndom_bench-ran3d-40k|

.. |ndom_bench-test2D-200k| image:: /_static/bench/ndom_bench-test2D-200k-time.png
   :width: 49%

.. |ndom_bench-ran3d-40k| image:: /_static/bench/ndom_bench-ran3d-40k-time.png
   :width: 49%

|wndom_bench-test2D-200k| |wndom_bench-ran3d-40k|

.. |wndom_bench-test2D-200k| image:: /_static/bench/wndom_bench-test2D-200k-time.png
   :width: 49%

.. |wndom_bench-ran3d-40k| image:: /_static/bench/wndom_bench-ran3d-40k-time.png
   :width: 49%


For dimensions larger than 3, :func:`moocore.is_nondominated` uses the
best-known :math:`O(n\log^{m-2} n)` algorithm.  `fast-pareto`_ claims to be
using a :math:`O(n\log^{m-2} n)` algorithm, but the benchmarks suggest that it
uses a :math:`O(m n^2)` algorithm, although slower than most other packages and
almost 100 times slower than `moocore`_.  The following plots with
``keep_weakly=True`` show that `moocore`_ is at least 10 times faster than the
other packages, even for 10D.

|wndom_bench-sphere-4d| |wndom_bench-convex-4d|

|wndom_bench-sphere-5d| |wndom_bench-rmnk-10d|

.. |wndom_bench-sphere-4d| image:: /_static/bench/wndom_bench-sphere-4d-time.png
   :width: 49%

.. |wndom_bench-convex-4d| image:: /_static/bench/wndom_bench-convex-4d-time.png
   :width: 49%

.. |wndom_bench-sphere-5d| image:: /_static/bench/wndom_bench-sphere-5d-time.png
   :width: 49%

.. |wndom_bench-rmnk-10d| image:: /_static/bench/wndom_bench-rmnk-10d-time.png
   :width: 49%


Similar results are obtained with ``keep_weakly=False``, however, this setting
is only supported by `moocore`_, `BoTorch`_ and `paretoset`_, thus the other
packages are actually slower to use than reported here, because the user is
forced to remove duplicated points when using them.

|ndom_bench-sphere-4d| |ndom_bench-convex-4d|

|ndom_bench-sphere-5d| |ndom_bench-rmnk-10d|

.. |ndom_bench-sphere-4d| image:: /_static/bench/ndom_bench-sphere-4d-time.png
   :width: 49%

.. |ndom_bench-convex-4d| image:: /_static/bench/ndom_bench-convex-4d-time.png
   :width: 49%

.. |ndom_bench-sphere-5d| image:: /_static/bench/ndom_bench-sphere-5d-time.png
   :width: 49%

.. |ndom_bench-rmnk-10d| image:: /_static/bench/ndom_bench-rmnk-10d-time.png
   :width: 49%


.. _bench-ndsort:

Nondominated Sorting (Pareto ranking)
-------------------------------------

Nondominated sorting (or Pareto ranking) assigns a rank to each point according
to Pareto-optimality. It can be used to split a set of points into layers of
nondominated fronts.  For more details, see :func:`moocore.pareto_rank`.  This
function in `moocore`_ uses a :math:`O(n \log n)` algorithm for 2 dimensions
and a :math:`O(n^2\log n)` algorithm for 3 dimensions.  As the plots show, this
makes `moocore`_ significantly faster than `paretoset`_, `pymoo`_ and
`DESDEO`_.  With higher dimensions, `moocore`_ uses a :math:`O(n^2\log^{m-2}n)`
algorithm instead of the :math:`O(n^3)` algorithm in `paretoset`_ and `DESDEO`_
(latter is compiled with `Numba`_).  In these benchmarks points are sampled
uniformly at random within the hypercube.

|ndsort_bench-ran-2d| |ndsort_bench-ran-3d|

|ndsort_bench-ran-4d| |ndsort_bench-ran-5d|

|ndsort_bench-ran-9d| |ndsort_bench-ran-10d|

.. |ndsort_bench-ran-2d| image:: /_static/bench/ndsort_bench-ran-2d-time.png
   :width: 49%

.. |ndsort_bench-ran-3d| image:: /_static/bench/ndsort_bench-ran-3d-time.png
   :width: 49%

.. |ndsort_bench-ran-4d| image:: /_static/bench/ndsort_bench-ran-4d-time.png
   :width: 49%

.. |ndsort_bench-ran-5d| image:: /_static/bench/ndsort_bench-ran-5d-time.png
   :width: 49%

.. |ndsort_bench-ran-9d| image:: /_static/bench/ndsort_bench-ran-9d-time.png
   :width: 49%

.. |ndsort_bench-ran-10d| image:: /_static/bench/ndsort_bench-ran-10d-time.png
   :width: 49%

.. include:: /benchmarks-links.inc.rst
