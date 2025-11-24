# ruff: noqa: D100
import numpy as np
from numpy.testing import (
    assert_array_equal,
)
import moocore


def test_bug27():
    """Invalid output of is_nondominated() with single-objective inputs."""
    x = np.array([[0.5], [0.6], [0.3], [0.1], [0.0], [0.9], [0.0]])

    assert_array_equal(
        moocore.is_nondominated(x),
        [False, False, False, False, True, False, False],
    )
    assert_array_equal(
        moocore.is_nondominated(x.tolist()),
        [False, False, False, False, True, False, False],
    )
    assert_array_equal(
        moocore.is_nondominated(x, keep_weakly=True),
        [False, False, False, False, True, False, True],
    )
    assert_array_equal(
        moocore.is_nondominated(x.tolist(), keep_weakly=True),
        [False, False, False, False, True, False, True],
    )

    assert_array_equal(
        moocore.is_nondominated(x, maximise=True),
        [False, False, False, False, False, True, False],
    )
    assert_array_equal(
        moocore.is_nondominated(x.tolist(), maximise=True),
        [False, False, False, False, False, True, False],
    )
    assert_array_equal(
        moocore.is_nondominated(x, keep_weakly=True, maximise=True),
        [False, False, False, False, False, True, False],
    )
    assert_array_equal(
        moocore.is_nondominated(x.tolist(), keep_weakly=True, maximise=True),
        [False, False, False, False, False, True, False],
    )


def test_bug29():
    """Invalid output of pareto_rank() with single-objective inputs."""
    x = [[0.2], [0.1], [0.2], [0.5], [0.3]]
    assert_array_equal(moocore.pareto_rank(x), [1, 0, 1, 3, 2])
    assert_array_equal(moocore.pareto_rank(x, maximise=True), [2, 3, 2, 0, 1])


def test_bug38():
    """Unexpected behavior of is_nondominated with 4+ objectives."""
    x = np.array([[0, 0, 0, 0], [1, 1, 1, 1]])
    assert_array_equal(
        moocore.is_nondominated(x, maximise=[False, True, True, True]),
        [True, True],
    )
    assert_array_equal(
        moocore.is_nondominated(x, maximise=[True, False, True, True]),
        [True, True],
    )
    assert_array_equal(
        moocore.is_nondominated(x, maximise=[True, True, False, True]),
        [True, True],
    )
    assert_array_equal(
        moocore.is_nondominated(x, maximise=[True, True, True, False]),
        [True, True],
    )
    assert_array_equal(
        moocore.is_nondominated(x, maximise=False), [True, False]
    )
    assert_array_equal(moocore.is_nondominated(x, maximise=True), [False, True])
