.. _whatsnew:

**********
What's new
**********

Version 0.1.5 (development version)
-----------------------------------

- New default method in :func:`~moocore.hv_approx()`. Computation is now done in C, so it is much faster.


Version 0.1.4 (30/10/2024)
--------------------------

- Improved example :ref:`sphx_glr_auto_examples_plot_pandas.py` to work in Pandas version >= 2.
- Changed behavior of :func:`~moocore.apply_within_sets`. The previous behavior could lead to subtle bugs.


Version 0.1.3 (28/10/2024)
--------------------------

- New: :class:`~moocore.Hypervolume`: Object-oriented API for hypervolume indicator.
- New: :func:`~moocore.apply_within_sets()`: Utility function to apply operations to individual datasets.
- New: :func:`~moocore.is_nondominated_within_sets()`: Utility function to identify nondominated points within sets.
- New example using :class:`pandas.DataFrame` in :ref:`sphx_glr_auto_examples_plot_pandas.py`.
- Fix bug in :func:`~moocore.normalise` when the input is :class:`pandas.DataFrame` or some other non-contiguous array.


Version 0.1.2 (18/09/2024)
--------------------------

- New: :func:`~moocore.hv_approx()`
- Documentation improvements.
- New gallery examples.
