.. _whatsnew:

**********
What's new
**********

Version 0.3.0 (development)
---------------------------

- `moarchiving <https://cma-es.github.io/moarchiving/>`_ is now included in the benchmarks.


Version 0.2.0 (10/01/2026)
---------------------------

- Requires ``numpy>=1.24``.
- `Optuna <https://optuna.org>`_ is now included in the benchmarks.
- New :ref:`bench-ndsort` benchmarks.
- :func:`~moocore.pareto_rank` is faster in 3D.
- HV3D+ is slightly faster with repeated coordinates (:issue:`41`).
- :func:`~moocore.igd`, :func:`~moocore.igd_plus`, :func:`~moocore.avg_hausdorff_dist` are faster.
- Fix wrong values returned by :func:`~moocore.epsilon_mult` when mixing minimization and maximization.
- Fix documentation of :ref:`epsilon_metric` (:user:`leandrolanzieri`)
- :func:`~moocore.hypervolume` uses the inclusion-exclusion algorithm for small inputs of up to 15 points, which is significantly faster.


Version 0.1.10 (24/11/2025)
---------------------------

- :func:`~moocore.is_nondominated`: Fix wrong assert (:issue:`38`).


Version 0.1.9 (31/10/2025)
--------------------------

- :func:`~moocore.hv_contributions` ignores dominated points by default.
  Set ``ignore_dominated=False`` to restore the previous behavior.
  The 3D case uses the HVC3D algorithm.
- New function :func:`~moocore.any_dominated` to quickly detect if a set is
  nondominated.
- New function :func:`~moocore.generate_ndset` to generate random nondominated
  sets with different shapes.
- New example :ref:`sphx_glr_auto_examples_plot_generate.py`.
- :func:`~moocore.is_nondominated`, :func:`~moocore.any_dominated`,
  :func:`~moocore.pareto_rank` now handle single-objective inputs correctly (:issue:`27`, :issue:`29`).
- Ranks returned by :func:`~moocore.pareto_rank` are 0-based.
- :func:`~moocore.is_nondominated` and :func:`~moocore.filter_dominated` are faster for dimensions larger than 3.
- ``moocore`` wheels are now built for ``aarch64`` (ARM64) in Linux and Windows.
  See the `installation instructions <https://github.com/multi-objective/moocore/tree/main/python#install>`_.
- :func:`~moocore.is_nondominated` and :func:`~moocore.filter_dominated` are now stable in 2D and 3D with ``!keep_weakly``, that is, only the first of duplicated points is marked as nondominated.


Version 0.1.8 (15/07/2025)
--------------------------

- Correct license to LGPL v2.1 or later.
- Bump dependencies to ``cffi>=1.17.1`` and ``setuptools>=77.0.3``.
- :func:`~moocore.eaf`, :func:`~moocore.vorob_t` and :func:`~moocore.vorob_dev`
  take the set indices as a separate argument ``sets`` following the API of the
  R package.
- New example :ref:`sphx_glr_auto_examples_plot_eaf.py`.
- Document EAF and Vorob'ev expectation and deviation in more detail.
- New online dataset: ``DTLZLinearShape.8d.front.60pts.10`` (see :func:`~moocore.get_dataset`).
- New default method in :func:`~moocore.hv_approx`. Computation is now done in C, so it is much faster.
- :func:`~moocore.hv_contributions` is much faster for 2D inputs.

Version 0.1.7 (04/06/2025)
--------------------------

- :func:`~moocore.hypervolume` now uses the HV3D\ :sup:`+` algorithm for the 3D case and the HV4D\ :sup:`+` algorithm for the 4D case.
  For dimensions larger than 4, the recursive algorithm uses HV4D\ :sup:`+` as the base case, which is significantly faster.
- :func:`~moocore.read_datasets` is significantly faster for large files.
- :func:`~moocore.is_nondominated` and :func:`~moocore.filter_dominated` are
  faster for 3D inputs.
- New function: :func:`~moocore.hv_contributions`.
- New online datasets: ``test2D-200k.inp.xz`` and ``ran.1000pts.3d.10`` (see
  :func:`~moocore.get_dataset`).

Version 0.1.6 (14/05/2025)
--------------------------

- New function: :func:`~moocore.largest_eafdiff`.
- New class: :class:`~moocore.RelativeHypervolume`.
- New dataset ``tpls50x20_1_MWT.csv``.
- Extended example :ref:`sphx_glr_auto_examples_plot_metrics.py`.
- ``vorobT()`` and ``vorobDev()`` were renamed to :func:`~moocore.vorob_t` and
  :func:`~moocore.vorob_dev` to follow Python convention.
- :func:`~moocore.get_dataset_path` and :func:`~moocore.get_dataset` can download large datasets from a remote repository.

Version 0.1.4 (30/10/2024)
--------------------------

- Improved example :ref:`sphx_glr_auto_examples_plot_pandas.py` to work in Pandas version >= 2.
- Changed behavior of :func:`~moocore.apply_within_sets`. The previous behavior could lead to subtle bugs.


Version 0.1.3 (28/10/2024)
--------------------------

- New: :class:`~moocore.Hypervolume`: Object-oriented API for hypervolume indicator.
- New: :func:`~moocore.apply_within_sets`: Utility function to apply operations to individual datasets.
- New: :func:`~moocore.is_nondominated_within_sets`: Utility function to identify nondominated points within sets.
- New example using :class:`pandas.DataFrame` in :ref:`sphx_glr_auto_examples_plot_pandas.py`.
- Fix bug in :func:`~moocore.normalise` when the input is :class:`pandas.DataFrame` or some other non-contiguous array.


Version 0.1.2 (18/09/2024)
--------------------------

- New: :func:`~moocore.hv_approx`
- Documentation improvements.
- New gallery examples.
