.. _whatsnew:

**********
What's new
**********

Version 0.1.7 (dev)
-------------------

- :func:`~moocore.hypervolume` now uses the HV3D\ :sup:`+` algorithm for the 3D case and the HV4D\ :sup:`+` algorithm for the 4D case.
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
