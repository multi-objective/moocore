# ruff: noqa: D100, D103
import numpy as np
import pytest
from numpy.testing import assert_allclose

import moocore


# --- reference implementations (independent, pure numpy) --------------------


def igd_bruteforce(data, ref):
    # mean over ref points of the min Euclidean distance to data
    d = np.sqrt(((ref[:, None, :] - data[None, :, :]) ** 2).sum(axis=2))
    return float(d.min(axis=1).mean())


def igd_plus_bruteforce(data, ref):
    # like IGD but distances only count where data is worse than ref (minimise)
    diff = np.maximum(data[None, :, :] - ref[:, None, :], 0.0)
    d = np.sqrt((diff**2).sum(axis=2))
    return float(d.min(axis=1).mean())


def epsilon_additive_bruteforce(data, ref):
    # min factor eps s.t. some data point + eps dominates each ref point
    return float(
        max(min(max(a - r) for a in data) for r in ref)
    )


def ranks_from_fronts_reference(F):
    # naive O(N^2 M) non-dominated ranking, correct at any dimension
    n = F.shape[0]
    ranks = np.full(n, -1)
    remaining = set(range(n))
    r = 0
    while remaining:
        idx = list(remaining)
        front = []
        for i in idx:
            dominated = False
            for j in idx:
                if j == i:
                    continue
                if np.all(F[j] <= F[i]) and np.any(F[j] < F[i]):
                    dominated = True
                    break
            if not dominated:
                front.append(i)
        for i in front:
            ranks[i] = r
            remaining.discard(i)
        r += 1
    return ranks


# --- large-dimension correctness (beyond 31, up to the 255 limit) ----------

DIMS = [32, 64, 128, 255]


@pytest.mark.parametrize("dim", DIMS)
def test_igd_large_dim_matches_reference(dim):
    rng = np.random.default_rng(dim)
    data = rng.random((6, dim))
    ref = rng.random((8, dim))
    assert_allclose(moocore.igd(data, ref=ref), igd_bruteforce(data, ref))


@pytest.mark.parametrize("dim", DIMS)
def test_igd_plus_large_dim_matches_reference(dim):
    rng = np.random.default_rng(dim + 1)
    data = rng.random((6, dim))
    ref = rng.random((8, dim))
    assert_allclose(
        moocore.igd_plus(data, ref=ref), igd_plus_bruteforce(data, ref)
    )


@pytest.mark.parametrize("dim", DIMS)
def test_epsilon_additive_large_dim_matches_reference(dim):
    rng = np.random.default_rng(dim + 2)
    data = rng.random((6, dim))
    ref = rng.random((8, dim))
    assert_allclose(
        moocore.epsilon_additive(data, ref=ref),
        epsilon_additive_bruteforce(data, ref),
    )


@pytest.mark.parametrize("dim", DIMS)
def test_igd_identical_sets_is_zero_large_dim(dim):
    # analytic case: IGD of a set against itself is exactly 0
    rng = np.random.default_rng(dim + 3)
    x = rng.random((5, dim))
    assert moocore.igd(x, ref=x) == 0.0


@pytest.mark.parametrize("dim", DIMS)
def test_pareto_rank_large_dim_chain(dim):
    # dominance chain: row i dominates row j for i < j -> ranks 0,1,2,...
    F = (np.arange(10)[:, None] * np.ones((1, dim))).astype(float)
    assert_allclose(moocore.pareto_rank(F), np.arange(10))


@pytest.mark.parametrize("dim", DIMS)
def test_pareto_rank_large_dim_matches_reference(dim):
    # real structure in the first 3 columns, remaining columns constant
    rng = np.random.default_rng(dim + 4)
    base = rng.random((40, 3))
    F = np.hstack([base, np.zeros((40, dim - 3))])
    assert_allclose(
        moocore.pareto_rank(F), ranks_from_fronts_reference(F)
    )


@pytest.mark.parametrize("dim", DIMS)
def test_is_nondominated_large_dim_matches_reference(dim):
    rng = np.random.default_rng(dim + 5)
    base = rng.random((40, 3))
    F = np.hstack([base, np.zeros((40, dim - 3))])
    expected = ranks_from_fronts_reference(F) == 0
    assert_allclose(moocore.is_nondominated(F), expected)


# --- errors are raised at the documented limits -----------------------------

GENERAL_255 = [
    ("igd", lambda x: moocore.igd(x, ref=x)),
    ("igd_plus", lambda x: moocore.igd_plus(x, ref=x)),
    ("avg_hausdorff_dist", lambda x: moocore.avg_hausdorff_dist(x, ref=x)),
    ("epsilon_additive", lambda x: moocore.epsilon_additive(x, ref=x)),
    ("epsilon_mult", lambda x: moocore.epsilon_mult(x, ref=x)),
    ("pareto_rank", lambda x: moocore.pareto_rank(x)),
    ("is_nondominated", lambda x: moocore.is_nondominated(x)),
    ("any_dominated", lambda x: moocore.any_dominated(x)),
]

HV_31 = [
    ("hypervolume", lambda x: moocore.hypervolume(x, ref=np.ones(x.shape[1]))),
    (
        "hv_contributions",
        lambda x: moocore.hv_contributions(x, ref=np.ones(x.shape[1])),
    ),
    ("hv_approx", lambda x: moocore.hv_approx(x, ref=np.ones(x.shape[1]))),
]


@pytest.mark.parametrize("name,fn", GENERAL_255)
def test_general_functions_raise_above_255(name, fn):
    rng = np.random.default_rng(0)
    # at the limit: must not raise
    fn(rng.random((4, 255)))
    # above the limit: must raise ValueError (not crash)
    with pytest.raises(ValueError, match="at most 255"):
        fn(rng.random((4, 256)))


@pytest.mark.parametrize("name,fn", HV_31)
def test_hv_functions_raise_above_31(name, fn):
    rng = np.random.default_rng(0)
    # at the limit: must not raise
    fn(rng.random((4, 31)))
    # above the limit: must raise ValueError (not crash / silently wrong)
    with pytest.raises(ValueError, match="at most 31"):
        fn(rng.random((4, 32)))


def test_epsilon_raises_on_single_column():
    # the C epsilon kernel is unrolled for the first two dimensions, so a
    # 1-column input used to read out of bounds and return garbage
    x = np.ones((3, 1))
    for fn in (moocore.epsilon_additive, moocore.epsilon_mult):
        with pytest.raises(ValueError, match="at least 2"):
            fn(x, ref=x)
