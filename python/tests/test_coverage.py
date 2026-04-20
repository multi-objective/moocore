# ruff: noqa: D100, D103
"""Tests targeting uncovered code paths to improve coverage."""

import pytest
import numpy as np
from numpy.testing import assert_allclose

import moocore


# ---------------------------------------------------------------------------
# generate_ndset additional methods (exercises match/case branches)
# ---------------------------------------------------------------------------
def test_generate_ndset_inverted_simplex():
    pts = moocore.generate_ndset(10, 3, "inverted-simplex", seed=42)
    assert pts.shape == (10, 3)


def test_generate_ndset_inverted_linear():
    pts = moocore.generate_ndset(10, 3, "inverted-linear", seed=42)
    assert pts.shape == (10, 3)


def test_generate_ndset_concave_simplex():
    pts = moocore.generate_ndset(10, 3, "concave-simplex", seed=42)
    assert pts.shape == (10, 3)


def test_generate_ndset_unknown_method():
    with pytest.raises(ValueError, match="unknown method"):
        moocore.generate_ndset(10, 3, "unknown-method")


def test_generate_ndset_integer():
    pts = moocore.generate_ndset(5, 3, "simplex", seed=42, integer=True)
    assert pts.dtype == int or np.issubdtype(pts.dtype, np.integer)
    assert pts.shape == (5, 3)


# ---------------------------------------------------------------------------
# is_nondominated / any_dominated edge cases
# ---------------------------------------------------------------------------
def test_is_nondominated_not_matrix():
    with pytest.raises(ValueError, match="must be a matrix"):
        moocore.is_nondominated([1, 2, 3])


def test_is_nondominated_empty():
    from numpy.testing import assert_array_equal

    result = moocore.is_nondominated(np.empty((0, 2)))
    assert_array_equal(result, np.array([], dtype=bool))


def test_any_dominated_not_matrix():
    with pytest.raises(ValueError, match="must be a matrix"):
        moocore.any_dominated([1, 2, 3])


def test_any_dominated_empty():
    with pytest.raises(ValueError, match="no points"):
        moocore.any_dominated(np.empty((0, 2)))


def test_any_dominated_single_point():
    assert moocore.any_dominated([[1, 2]]) is False


def test_any_dominated_single_objective():
    assert moocore.any_dominated([[1], [2], [1]]) is True


# ---------------------------------------------------------------------------
# Hypervolume class edge cases
# ---------------------------------------------------------------------------
def test_hypervolume_class_broadcast_ref():
    """Test Hypervolume with scalar ref and multi-objective maximise."""
    hv = moocore.Hypervolume(ref=[10], maximise=[True, False])
    result = hv(np.array([[5, 5], [4, 6]]))
    assert result >= 0


def test_hypervolume_class_ref_maximise_mismatch():
    with pytest.raises(ValueError, match="same length"):
        moocore.Hypervolume(ref=[10, 20], maximise=[True, False, True])


def test_hypervolume_class_data_nobj_mismatch():
    hv = moocore.Hypervolume(ref=[10, 10])
    with pytest.raises(ValueError, match="same number of objectives"):
        hv(np.array([[1, 2, 3]]))


def test_hypervolume_class_maximise_copy():
    """Exercise the copy-and-transform path when maximise is set."""
    hv = moocore.Hypervolume(ref=[10, 10], maximise=[True, False])
    data = np.array([[5.0, 5.0], [4.0, 6.0]])
    result = hv(data)
    assert result >= 0
    # Verify original data is not modified
    assert_allclose(data, [[5.0, 5.0], [4.0, 6.0]])


# ---------------------------------------------------------------------------
# RelativeHypervolume edge cases
# ---------------------------------------------------------------------------
def test_relative_hypervolume_zero_refset_hv():
    with pytest.raises(ValueError, match="zero"):
        moocore.RelativeHypervolume(
            ref=[10, 10], ref_set=[[10, 10]], maximise=False
        )


# ---------------------------------------------------------------------------
# hv_contributions edge cases
# ---------------------------------------------------------------------------
def test_hv_contributions_ref_mismatch():
    with pytest.raises(ValueError, match="must have length"):
        moocore.hv_contributions(
            np.array([[1, 2], [3, 4]]), ref=np.array([10, 10, 10])
        )


def test_hv_contributions_maximise_copy():
    """Exercise the copy path with maximise in hv_contributions."""
    x = np.array([[1.0, 2.0], [3.0, 1.0]])
    ref = np.array([10.0, 10.0])
    result = moocore.hv_contributions(x, ref=ref, maximise=[True, False])
    assert len(result) == 2
    # Verify original data unchanged
    assert_allclose(x, [[1.0, 2.0], [3.0, 1.0]])


# ---------------------------------------------------------------------------
# pareto_rank with maximise (copy path)
# ---------------------------------------------------------------------------
def test_pareto_rank_maximise_copy():
    data = np.array([[1.0, 2.0], [3.0, 1.0], [2.0, 2.0]])
    ranks = moocore.pareto_rank(data, maximise=[True, False])
    assert len(ranks) == 3
    # Verify original data unchanged
    assert_allclose(data, [[1.0, 2.0], [3.0, 1.0], [2.0, 2.0]])
