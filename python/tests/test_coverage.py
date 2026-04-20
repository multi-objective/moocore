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
