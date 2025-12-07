# ruff: noqa: D100, D103
import pytest
import numpy as np


def test_immutable_call(immutable_call):
    """Check that immutable_call works."""

    def bad(x):
        x[0] = 42  # Accidental mutation
        return x.sum()  # nocov

    x = np.array([1, 2, 3])
    with pytest.raises(AssertionError):
        immutable_call(bad, x)


def test_partial_immutability_positional(immutable_call):
    """Check partial immutable_call works."""

    def bad(x, y):
        x[0] = 99  # SHOULD be caught
        return x.sum() + y.sum()  # nocov

    def good(x, y):
        y[0] = 88  # allowed
        return x.sum() + y.sum()

    x = np.array([1, 2, 3])
    y = np.array([5, 6, 7])

    with pytest.raises(AssertionError):
        immutable_call(bad, x, y, immutable=[0])  # x must remain immutable

    immutable_call(good, x, y, immutable=[0])  # x must remain immutable


def test_partial_immutability_keyword(immutable_call):
    """Check partial immutable_call works."""

    def bad(a, b):
        b[0] = 111  # not allowed
        return a + b  # nocov

    def good(a, b):
        a[0] = 111  # allowed
        return a + b

    a = np.array([1, 2])
    b = np.array([3, 4])

    with pytest.raises(AssertionError):
        immutable_call(bad, a, b=b, immutable=["b"])

    immutable_call(good, a, b=b, immutable=["b"])


def test_nested_structures(immutable_call):
    data = {"x": [1, 2, {"w": np.array([5, 6])}]}

    def f1(d):
        d["x"][2]["w"][0] = 99  # forbidden

    with pytest.raises(AssertionError):
        immutable_call(f1, data)

    def f3(d):
        d["x"][2]["w"] = np.array([5.0, 6.0])  # forbidden

    with pytest.raises(AssertionError):
        immutable_call(f3, data)

    def f4(d):
        d["x"] = (1, 2, {"w": np.array([5, 6])})  # forbidden

    with pytest.raises(AssertionError):
        immutable_call(f4, data)

    def f5(d):
        d["x"] = [1, 2]  # forbidden

    with pytest.raises(AssertionError):
        immutable_call(f5, data)

    def f6(d):
        d["y"] = [1, 2, {"w": np.array([5, 6])}]  # forbidden

    with pytest.raises(AssertionError):
        immutable_call(f6, data)

    data = [1, 2]

    def f7(d):
        d.append(3)

    with pytest.raises(AssertionError):
        immutable_call(f7, data)


@pytest.mark.xfail
def test_identity(immutable_call):
    """We should forbid replacing an element with a new object that looks the same."""
    data = {"x": [1, 2, {"w": np.array([5, 6])}]}

    # FIXME: This doesn't work!
    def f2(d):
        d["x"][2]["w"] = np.array([5, 6])  # forbidden

    with pytest.raises(AssertionError):
        immutable_call(f2, data)
