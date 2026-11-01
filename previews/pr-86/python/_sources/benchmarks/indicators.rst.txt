Quality indicators
==================

.. _bench-hv:

Exact computation of hypervolume
--------------------------------

The following plots compare the speed of computing the :ref:`hypervolume indicator <hypervolume_metric>` in 3D, 4D, 5D and 6D. As the plots show, `moocore`_ is 100 times faster than the other packages and 1000 times faster than `BoTorch`_ and, by extension, `Xopt`_ and `BoFire`_.

For 3D, `Optuna`_ uses a :math:`O(n^2)` algorithm, while `moocore`_ uses the best-known :math:`O(n\log n)` one, so the gap between the two will get significantly larger with a larger number of points than those tested here. `moarchiving`_ uses the same algorithms as `moocore`_ for 3D and 4D, but a pure Python implementation, so it is still 10-100 times slower than `moocore`_'s C implementation.  `BoTorch`_ and `fast-pareto`_ are not included for more than 3 objectives because **they are tens of thousands of times slower** than `moocore`_.

Recent versions of `DESDEO`_, `DEAP`_ (≥1.4.4), `pymoo`_ (≥0.6.1.6) and `jMetalPy`_ (≥1.9.0) already use `moocore`_ for hypervolume computation.
The implementation in `patatune`_ appears to be wrong (https://github.com/cms-patatrack/patatune/issues/52).

|hv_bench-DTLZLinearShape-3d| |hv_bench-DTLZLinearShape-4d|

|hv_bench-DTLZLinearShape-5d| |hv_bench-DTLZLinearShape-6d|

.. |hv_bench-DTLZLinearShape-3d| image:: /_static/bench/hv_bench-DTLZLinearShape.3d-time.png
   :width: 49%

.. |hv_bench-DTLZLinearShape-4d| image:: /_static/bench/hv_bench-DTLZLinearShape.4d-time.png
   :width: 49%

.. |hv_bench-DTLZLinearShape-5d| image:: /_static/bench/hv_bench-DTLZLinearShape.5d-time.png
   :width: 49%

.. |hv_bench-DTLZLinearShape-6d| image:: /_static/bench/hv_bench-DTLZLinearShape.6d-time.png
   :width: 49%

.. _bench-hvapprox:

Approximation of the hypervolume
--------------------------------

The following plots compare the accuracy and speed of approximating the
hypervolume with the various methods provided by :func:`moocore.hv_approx`. The
plots show that there is no clear winner, in terms of approximation error,
between methods ``Rphi-FWE+`` (default) and ``DZ2019-HW``, but both produce
consistently lower approximation errors than method ``DZ2019-MC``.  However, ``Rphi-FWE+`` is as fast as ``DZ2019-MC`` and both are consistently faster than ``DZ2019-HW``, in particular with higher number of objectives.

The exact computation of the hypervolume for less than 5D takes milliseconds
for thousands of points, whereas approximating the hypervolume is significantly
slower and, thus, not worth doing.

Approximating the hypervolume becomes more useful for dimensions higher than 5,
where the exact computation becomes noticeably slower with hundreds of points.
For such problems, method ``DZ2019-HW`` becomes significantly slower than
``Rphi-FWE+``.

|hvapprox_bench-DTLZLinearShape-6d-values| |hvapprox_bench-DTLZLinearShape-6d-time|

|hvapprox_bench-DTLZSphereShape-6d-values| |hvapprox_bench-DTLZSphereShape-6d-time|

|hvapprox_bench-ran-6d-values| |hvapprox_bench-ran-6d-time|

|hvapprox_bench-DTLZLinearShape-9d-values| |hvapprox_bench-DTLZLinearShape-9d-time|

|hvapprox_bench-ran-9d-values| |hvapprox_bench-ran-9d-time|

|hvapprox_bench-DTLZSphereShape-10d-values| |hvapprox_bench-DTLZSphereShape-10d-time|

.. |hvapprox_bench-DTLZSphereShape-6d-values| image:: /_static/bench/hvapprox_bench-DTLZSphereShape.6d-values.png
   :width: 49%

.. |hvapprox_bench-DTLZSphereShape-6d-time| image:: /_static/bench/hvapprox_bench-DTLZSphereShape.6d-time.png
   :width: 49%

.. |hvapprox_bench-DTLZLinearShape-6d-values| image:: /_static/bench/hvapprox_bench-DTLZLinearShape.6d-values.png
   :width: 49%

.. |hvapprox_bench-DTLZLinearShape-6d-time| image:: /_static/bench/hvapprox_bench-DTLZLinearShape.6d-time.png
   :width: 49%

.. |hvapprox_bench-ran-6d-values| image:: /_static/bench/hvapprox_bench-ran.6d-values.png
   :width: 49%

.. |hvapprox_bench-ran-6d-time| image:: /_static/bench/hvapprox_bench-ran.6d-time.png
   :width: 49%

.. |hvapprox_bench-DTLZLinearShape-9d-values| image:: /_static/bench/hvapprox_bench-DTLZLinearShape.9d-values.png
   :width: 49%

.. |hvapprox_bench-DTLZLinearShape-9d-time| image:: /_static/bench/hvapprox_bench-DTLZLinearShape.9d-time.png
   :width: 49%

.. |hvapprox_bench-ran-9d-values| image:: /_static/bench/hvapprox_bench-ran.9d-values.png
   :width: 49%

.. |hvapprox_bench-ran-9d-time| image:: /_static/bench/hvapprox_bench-ran.9d-time.png
   :width: 49%

.. |hvapprox_bench-DTLZSphereShape-10d-values| image:: /_static/bench/hvapprox_bench-DTLZSphereShape.10d-values.png
   :width: 49%

.. |hvapprox_bench-DTLZSphereShape-10d-time| image:: /_static/bench/hvapprox_bench-DTLZSphereShape.10d-time.png
   :width: 49%


Epsilon and IGD+ indicators
---------------------------

The following plots benchmark the speed of computing the :ref:`epsilon indicator  <epsilon_metric>` metric and :ref:`IGD+ indicator <igd_hausdorf>`. Although  the algorithms for computing these metrics are relatively simple and easy to vectorize in Python, the `moocore`_ implementation is still 10 to 100 times faster.

|fig_eps_bench| |fig_igd_plus_bench|

.. |fig_eps_bench| image:: /_static/bench/eps_bench-rmnk_10D_random_search-time.png
   :width: 48%

.. |fig_igd_plus_bench| image:: /_static/bench/igd_plus_bench-ran.1000pts.3d-time.png
   :width: 48%

.. include:: /benchmarks-links.inc.rst
