# ruff: noqa: D100, D103
# These tests require the relaxed `ASSUME(1 <= dim)` in c/igd.h (optional
# patch): single-column IGD-family metrics are mathematically well-defined and
# are called with 1 column by released downstream code (pymoo 0.6.2 uses
# moocore.igd on design-space data of single-variable problems).
import numpy as np
import pytest
from numpy.testing import assert_allclose

import moocore


@pytest.mark.parametrize("seed", [1, 2, 3])
def test_igd_family_single_column(seed):
    rng = np.random.default_rng(seed)
    a = rng.random((6, 1))
    r = rng.random((4, 1))

    igd_ref = np.mean([np.min(np.abs(a - x)) for x in r.ravel()])
    igdp_ref = np.mean([np.min(np.maximum(a - x, 0)) for x in r.ravel()])
    gd_ref = np.mean([np.min(np.abs(r - x)) for x in a.ravel()])

    assert_allclose(moocore.igd(a, ref=r), igd_ref)
    assert_allclose(moocore.igd_plus(a, ref=r), igdp_ref)
    assert_allclose(moocore.avg_hausdorff_dist(a, ref=r), max(igd_ref, gd_ref))


def test_igd_single_column_identical_is_zero():
    x = np.arange(5.0).reshape(-1, 1)
    assert moocore.igd(x, ref=x) == 0.0
