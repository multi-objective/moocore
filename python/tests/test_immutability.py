# ruff: noqa: D100, D103
import numpy as np
import pytest

import moocore


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
        y[0] = 99  # SHOULD be caught
        return x.sum() + y.sum()  # nocov

    def good(x, y):
        x[0] = 88  # allowed
        return x.sum() + y.sum()

    x = np.array([1, 2, 3])
    y = np.array([5, 6, 7])

    with pytest.raises(AssertionError):
        immutable_call(bad, x, y, immutable=[1])  # x must remain immutable

    immutable_call(good, x, y, immutable=[1])  # x must remain immutable


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
        immutable_call(bad, a=a, b=b, immutable=["b"])

    immutable_call(good, a=a, b=b, immutable=["b"])


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


def _maximise(maximise, nobj):
    if maximise == "mixed":
        return np.arange(nobj) % 2 == 0
    return maximise


def _points(nobj, n=20, seed=42):
    return np.random.default_rng(seed).random((n, nobj))


def _sets(n=20, nsets=4):
    return np.repeat(np.arange(1, nsets + 1), n // nsets)


def _points_sets(nobj, n=20, nsets=4, seed=42):
    """Return points with the set number in the last column."""
    return np.column_stack((_points(nobj, n, seed), _sets(n, nsets)))


def _ref(nobj, maximise=False):
    """Return a reference point worse than all points in (0,1)^nobj."""
    maximise = np.broadcast_to(_maximise(maximise, nobj), nobj)
    return np.where(maximise, -0.1, 1.1)


def _ideal(nobj, maximise=False):
    """Return an ideal point better than all points in (0,1)^nobj."""
    maximise = np.broadcast_to(_maximise(maximise, nobj), nobj)
    return np.where(maximise, 1.1, -0.1)


def _rectangles():
    return np.array(
        [
            [0.1, 0.6, 0.4, np.inf, 1],
            [0.4, 0.7, 0.5, np.inf, 2],
            [0.4, 0.6, 0.6, 0.7, 3],
        ]
    )


def _hypervolume_class(points, ref, maximise):
    return moocore.Hypervolume(ref, maximise=maximise)(points)


def _relative_hypervolume_class(points, ref, ref_set, maximise):
    return moocore.RelativeHypervolume(ref, ref_set, maximise=maximise)(points)


def _vorob_dev(points, sets, ref):
    ve = moocore.vorob_t(points, sets, ref=ref).ve
    return moocore.vorob_dev(points, sets, ref=ref, ve=ve)


# Each case maps a value of maximise to (function, args, kwargs).  The arrays
# are created anew for every call because immutable_call() makes them
# read-only.
_CASES_MAXIMISE = {
    "hypervolume": lambda m: (
        moocore.hypervolume,
        (_points(3),),
        {"ref": _ref(3, m), "maximise": _maximise(m, 3)},
    ),
    "Hypervolume": lambda m: (
        _hypervolume_class,
        (_points(3),),
        {"ref": _ref(3, m), "maximise": _maximise(m, 3)},
    ),
    "RelativeHypervolume": lambda m: (
        _relative_hypervolume_class,
        (_points(3),),
        {
            "ref": _ref(3, m),
            "ref_set": _points(3, seed=1),
            "maximise": _maximise(m, 3),
        },
    ),
    **{
        f"hv_contributions-ignore_dominated={ignore}": (
            lambda m, ignore=ignore: (
                moocore.hv_contributions,
                (_points(3),),
                {
                    "ref": _ref(3, m),
                    "maximise": _maximise(m, 3),
                    "ignore_dominated": ignore,
                },
            )
        )
        for ignore in (False, True)
    },
    **{
        f"hv_approx-{method}": (
            lambda m, method=method: (
                moocore.hv_approx,
                (_points(3),),
                {
                    "ref": _ref(3, m),
                    "maximise": _maximise(m, 3),
                    "nsamples": 100,
                    "seed": 42,
                    "method": method,
                },
            )
        )
        for method in ("DZ2019-HW", "DZ2019-MC", "Rphi-FWE+")
    },
    **{
        fn.__name__: (
            lambda m, fn=fn: (
                fn,
                (_points(3),),
                {"ref": _points(3, seed=1), "maximise": _maximise(m, 3)},
            )
        )
        for fn in (
            moocore.igd,
            moocore.igd_plus,
            moocore.avg_hausdorff_dist,
            moocore.epsilon_additive,
            moocore.epsilon_mult,
        )
    },
    "r2_exact": lambda m: (
        moocore.r2_exact,
        (_points(2),),
        {"ref": _ideal(2, m), "maximise": _maximise(m, 2)},
    ),
    **{
        f"{fn.__name__}-keep_weakly={keep_weakly}": (
            lambda m, fn=fn, keep_weakly=keep_weakly: (
                fn,
                (_points(3),),
                {"maximise": _maximise(m, 3), "keep_weakly": keep_weakly},
            )
        )
        for fn in (
            moocore.is_nondominated,
            moocore.any_dominated,
            moocore.filter_dominated,
        )
        for keep_weakly in (False, True)
    },
    "is_nondominated_within_sets": lambda m: (
        moocore.is_nondominated_within_sets,
        (_points(3), _sets()),
        {"maximise": _maximise(m, 3)},
    ),
    "filter_dominated_within_sets": lambda m: (
        moocore.filter_dominated_within_sets,
        (_points_sets(3),),
        {"maximise": _maximise(m, 3)},
    ),
    "pareto_rank": lambda m: (
        moocore.pareto_rank,
        (_points(3),),
        {"maximise": _maximise(m, 3)},
    ),
    "normalise": lambda m: (
        moocore.normalise,
        (_points(3),),
        {
            "to_range": np.array([1.0, 2.0]),
            "lower": np.zeros(3),
            "upper": np.ones(3),
            "maximise": _maximise(m, 3),
        },
    ),
    **{
        f"eafdiff-rectangles={rectangles}": (
            lambda m, rectangles=rectangles: (
                moocore.eafdiff,
                (_points_sets(2), _points_sets(2, seed=1)),
                {
                    "intervals": 3,
                    "maximise": _maximise(m, 2),
                    "rectangles": rectangles,
                },
            )
        )
        for rectangles in (False, True)
    },
    **{
        f"whv_hype-{dist}": (
            lambda m, dist=dist, mu=mu: (
                moocore.whv_hype,
                (_points(2),),
                {
                    "ref": _ref(2, m),
                    "ideal": _ideal(2, m),
                    "maximise": _maximise(m, 2),
                    "nsamples": 100,
                    "seed": 42,
                    "dist": dist,
                    "mu": mu() if callable(mu) else mu,
                },
            )
        )
        for dist, mu in (
            ("uniform", None),
            ("point", lambda: np.array([0.5, 0.5])),
            ("exponential", 0.2),
        )
    },
}

_CASES_NO_MAXIMISE = {
    # These functions only support minimisation.
    "largest_eafdiff": lambda: (
        moocore.largest_eafdiff,
        ([_points_sets(2, seed=seed) for seed in range(3)],),
        {"ref": _ref(2), "intervals": 3, "ideal": _ideal(2)},
    ),
    "whv_rect": lambda: (
        moocore.whv_rect,
        (_points(2), _rectangles()),
        {"ref": _ref(2)},
    ),
    "total_whv_rect": lambda: (
        moocore.total_whv_rect,
        (_points(2), _rectangles()),
        {"ref": _ref(2), "ideal": _ideal(2)},
    ),
    "eaf": lambda: (
        moocore.eaf,
        (_points(2), _sets()),
        {"percentiles": np.array([0.0, 50.0, 100.0])},
    ),
    "vorob_t": lambda: (
        moocore.vorob_t,
        (_points(2), _sets()),
        {"ref": _ref(2)},
    ),
    "vorob_dev": lambda: (
        _vorob_dev,
        (_points(2), _sets()),
        {"ref": _ref(2)},
    ),
    "apply_within_sets": lambda: (
        moocore.apply_within_sets,
        (_points(3), _sets(), moocore.is_nondominated),
        {},
    ),
}


@pytest.mark.parametrize("maximise", [False, True, "mixed"])
@pytest.mark.parametrize("case", _CASES_MAXIMISE.keys())
def test_public_api_immutable_maximise(immutable_call, case, maximise):
    """Check that public functions do not modify their array arguments."""
    fn, args, kwargs = _CASES_MAXIMISE[case](maximise)
    immutable_call(fn, *args, **kwargs)


@pytest.mark.parametrize("case", _CASES_NO_MAXIMISE.keys())
def test_public_api_immutable(immutable_call, case):
    """Check that public functions do not modify their array arguments."""
    fn, args, kwargs = _CASES_NO_MAXIMISE[case]()
    immutable_call(fn, *args, **kwargs)
