# ruff: noqa: D100, D103
import math

import numpy as np
import pytest
from numpy.testing import assert_allclose, assert_array_equal

import moocore

LOWER, UPPER = 0.0, 10.0

TWO_CHILDREN = {
    "uniform": moocore.uniform_crossover,
    "single_point": moocore.single_point_crossover,
    "single_arithmetic": moocore.single_arithmetic_crossover,
    "local": moocore.local_crossover,
    "bounded_exponential": moocore.bounded_exponential_crossover,
}
PAIR_OPERATORS = {**TWO_CHILDREN, "blend_alpha": moocore.blend_alpha_crossover}
POPULATION_OPERATORS = {
    "de_rand_1_bin": moocore.de_rand_1_bin_crossover,
    "de_current_1": moocore.de_current_1_crossover,
    "parent_centric": moocore.parent_centric_crossover,
}
ALL_OPERATORS = {**PAIR_OPERATORS, **POPULATION_OPERATORS}
BOUNDED = {
    "blend_alpha",
    "bounded_exponential",
    *POPULATION_OPERATORS,
}


def random_matrix(n, nvar=4, seed=0):
    return np.random.default_rng(seed).uniform(LOWER, UPPER, size=(n, nvar))


def pairs(x):
    return x[0::2], x[1::2]


@pytest.mark.parametrize("name", TWO_CHILDREN)
@pytest.mark.parametrize("n", [2, 3, 4, 5, 10])
def test_two_children_per_pair_keep_row_count(name, n):
    x = random_matrix(n)
    assert TWO_CHILDREN[name](x, seed=1).shape == x.shape


@pytest.mark.parametrize("n", [2, 3, 4, 5, 10])
def test_blend_alpha_crossover_one_child_per_pair(n):
    x = random_matrix(n)
    out = moocore.blend_alpha_crossover(x, seed=1)
    assert out.shape == (math.ceil(n / 2), x.shape[1])


@pytest.mark.parametrize("name", POPULATION_OPERATORS)
def test_population_operators_one_child_per_target(name):
    fun = POPULATION_OPERATORS[name]
    x = random_matrix(6)
    assert fun(x, seed=1).shape == x.shape
    assert fun(x, targets=[4, 0, 4], seed=1).shape == (3, x.shape[1])
    assert fun(x, targets=2, seed=1).shape == (1, x.shape[1])
    assert fun(x, targets=[], seed=1).shape == (0, x.shape[1])


@pytest.mark.parametrize("n", [3, 5, 7])
def test_odd_number_of_parents_pairs_last_with_first(n):
    x = random_matrix(n)
    assert_array_equal(moocore.uniform_crossover(x, swap_prob=0, seed=1), x)
    # Swapping every variable gives each child the other parent.
    out = moocore.uniform_crossover(x, swap_prob=1, seed=1)
    assert_array_equal(out[0], x[1])
    assert_array_equal(out[-1], x[0])

    out = moocore.blend_alpha_crossover(x, alpha=0, seed=1)
    assert np.all(out[-1] >= np.minimum(x[-1], x[0]))
    assert np.all(out[-1] <= np.maximum(x[-1], x[0]))


@pytest.mark.parametrize("name", PAIR_OPERATORS)
@pytest.mark.parametrize("n", [3, 4, 5])
def test_identical_parents_reproduce_themselves(name, n):
    # The row has distinct values, so a transposed output would not match.
    row = np.arange(1.0, 6.0)
    x = np.tile(row, (n, 1))
    out = PAIR_OPERATORS[name](x, seed=1)
    assert_allclose(out, np.tile(row, (len(out), 1)))


@pytest.mark.parametrize("name", ALL_OPERATORS)
def test_seed(name):
    fun = ALL_OPERATORS[name]
    x = random_matrix(8)
    assert_array_equal(fun(x, seed=42), fun(x, seed=42))
    assert_array_equal(
        fun(x, seed=np.random.default_rng(42)),
        fun(x, seed=np.random.default_rng(42)),
    )
    assert not np.array_equal(fun(x, seed=1), fun(x, seed=2))
    # The same Generator gives new children on each call.
    rng = np.random.default_rng(42)
    assert not np.array_equal(fun(x, seed=rng), fun(x, seed=rng))
    # seed=None still works.
    assert fun(x).shape[1] == x.shape[1]


@pytest.mark.parametrize("name", ALL_OPERATORS)
def test_inputs_are_not_modified(name, immutable_call):
    x = random_matrix(6)
    kwargs = {}
    if name in BOUNDED:
        kwargs = {"lower": np.full(4, LOWER), "upper": np.full(4, UPPER)}
    if name in POPULATION_OPERATORS:
        kwargs["targets"] = np.array([0, 2])
    immutable_call(ALL_OPERATORS[name], x, seed=3, **kwargs)


def test_uniform_crossover_only_copies_parent_values():
    x = random_matrix(200, nvar=6)
    p1, p2 = pairs(x)
    c1, c2 = pairs(moocore.uniform_crossover(x, seed=1))
    from_first = c1 == p1
    assert np.all(from_first | (c1 == p2))
    assert_array_equal(c2, np.where(from_first, p2, p1))
    assert_array_equal(moocore.uniform_crossover(x, pair_prob=0, seed=1), x)


@pytest.mark.parametrize("swap_prob", [0.0, 0.25, 0.5, 0.75, 1.0])
def test_uniform_crossover_swap_rate(swap_prob):
    x = np.tile([np.zeros(10), np.ones(10)], (2000, 1))
    out = moocore.uniform_crossover(x, swap_prob=swap_prob, seed=1)
    # The first child gets a 1 only where it was swapped.
    assert abs(out[0::2].mean() - swap_prob) < 0.02


@pytest.mark.parametrize(
    "fun", [moocore.uniform_crossover, moocore.single_point_crossover]
)
@pytest.mark.parametrize("dtype", [int, bool, np.int8, np.float32])
def test_value_copying_operators_keep_dtype(fun, dtype):
    x = np.array([[0, 0, 0, 0], [1, 1, 1, 1], [0, 1, 0, 1]], dtype=dtype)
    out = fun(x, seed=1)
    if np.issubdtype(dtype, np.floating):
        assert out.dtype == np.float64
    else:
        assert out.dtype == x.dtype
    assert np.all((out == 0) | (out == 1))


def test_single_point_crossover_exchanges_tails():
    nvar = 5
    x = np.tile([np.zeros(nvar), np.ones(nvar)], (2000, 1))
    c1, c2 = pairs(moocore.single_point_crossover(x, seed=1))
    cuts = (c1 == 0).sum(axis=1)
    assert_array_equal(c1, (np.arange(nvar) >= cuts[:, None]).astype(float))
    assert_array_equal(c2, 1 - c1)
    assert set(cuts) == set(range(1, nvar))


@pytest.mark.parametrize("alpha", [0.0, 0.5, 1.0])
def test_blend_alpha_crossover_interval(alpha):
    x = random_matrix(400, nvar=3)
    p1, p2 = pairs(x)
    out = moocore.blend_alpha_crossover(
        x, lower=LOWER, upper=UPPER, alpha=alpha, seed=1
    )
    lo, hi = np.minimum(p1, p2), np.maximum(p1, p2)
    span = hi - lo
    assert np.all(out >= np.maximum(lo - alpha * span, LOWER) - 1e-12)
    assert np.all(out <= np.minimum(hi + alpha * span, UPPER) + 1e-12)


def test_blend_alpha_crossover_samples_variables_independently():
    x = np.tile([[2.0, 8.0], [8.0, 2.0]], (5000, 1))
    out = moocore.blend_alpha_crossover(x, alpha=0, seed=1)
    corr = np.corrcoef(out[:, 0], out[:, 1])[0, 1]
    assert abs(corr) < 0.05


def test_single_arithmetic_crossover_changes_one_variable():
    x = random_matrix(100, nvar=5)
    p1, p2 = pairs(x)
    c1, c2 = pairs(moocore.single_arithmetic_crossover(x, seed=1))
    changed = c1 != p1
    assert np.all(changed.sum(axis=1) == 1)
    assert_array_equal(changed, c2 != p2)
    mean = 0.5 * (p1 + p2)
    assert_array_equal(c1[changed], mean[changed])
    assert_array_equal(c2[changed], mean[changed])
    assert_array_equal(
        moocore.single_arithmetic_crossover(x, pair_prob=0, seed=1), x
    )


def test_local_crossover_children_lie_between_parents():
    x = random_matrix(100, nvar=5)
    p1, p2 = pairs(x)
    c1, c2 = pairs(moocore.local_crossover(x, seed=1))
    assert_allclose(c1 + c2, p1 + p2)
    lo, hi = np.minimum(p1, p2) - 1e-12, np.maximum(p1, p2) + 1e-12
    for c in (c1, c2):
        assert np.all((c >= lo) & (c <= hi))


def test_bounded_exponential_crossover_shared_values():
    x = np.tile([[0.0, 5.0, 10.0, 3.0], [0.0, 5.0, 10.0, 7.0]], (50, 1))
    out = moocore.bounded_exponential_crossover(x, lower=0, upper=10, seed=1)
    assert np.all(np.isfinite(out))
    assert_array_equal(out[:, :3], x[:, :3])
    assert np.all((out >= 0) & (out <= 10))


def test_bounded_exponential_crossover_bounds():
    x = random_matrix(1000)
    out = moocore.bounded_exponential_crossover(
        x, lower=LOWER, upper=UPPER, scale=1.0, seed=1
    )
    assert np.all((out >= LOWER) & (out <= UPPER))
    out = moocore.bounded_exponential_crossover(x, scale=1.0, seed=1)
    assert np.all(np.isfinite(out))
    assert np.any((out < LOWER) | (out > UPPER))
    assert_array_equal(
        moocore.bounded_exponential_crossover(x, pair_prob=0, seed=1), x
    )


@pytest.mark.parametrize(
    ("swap_prob", "expected"), [(0.0, 1.0), (0.5, 0.5), (1.0, 0.0)]
)
def test_bounded_exponential_crossover_swap_prob(swap_prob, expected):
    x = np.tile([np.full(10, 2.0), np.full(10, 8.0)], (1000, 1))
    out = moocore.bounded_exponential_crossover(
        x, lower=0, upper=10, swap_prob=swap_prob, seed=1
    )
    # Fraction of the variables of the first child that stay near its parent.
    c1 = out[0::2]
    kept = (np.abs(c1 - 2) < np.abs(c1 - 8)).mean()
    assert abs(kept - expected) < 0.02


@pytest.mark.parametrize("order", [1, -1])
def test_bounded_exponential_crossover_children_near_their_parent(order):
    pair = np.array([[3.0, 3.0], [7.0, 7.0]])[::order]
    x = np.tile(pair, (2000, 1))
    out = moocore.bounded_exponential_crossover(x, lower=0, upper=10, seed=1)
    assert_allclose(np.median(out[0::2], axis=0), pair[0], atol=0.1)
    assert_allclose(np.median(out[1::2], axis=0), pair[1], atol=0.1)


def test_de_rand_1_bin_crossover_forces_one_mutant_variable():
    x = random_matrix(20, nvar=6)
    out = moocore.de_rand_1_bin_crossover(x, crossover_rate=0, seed=1)
    assert np.all((out != x).sum(axis=1) == 1)


def test_de_rand_1_bin_crossover_base_is_another_row():
    x = random_matrix(10, nvar=3)
    out = moocore.de_rand_1_bin_crossover(
        x, scaling_factor=1e-12, crossover_rate=1, seed=1
    )
    for i, child in enumerate(out):
        dist = np.abs(x - child).max(axis=1)
        nearest = np.argmin(dist)
        assert nearest != i
        assert dist[nearest] < 1e-9


def test_de_current_1_crossover_base_is_the_target():
    x = random_matrix(10)
    assert_array_equal(
        moocore.de_current_1_crossover(x, crossover_rate=0, seed=1), x
    )
    out = moocore.de_current_1_crossover(x, scaling_factor=1e-12, seed=1)
    assert_allclose(out, x, atol=1e-9)


def test_de_current_1_crossover_two_rows():
    # The donors are the two rows, in either order.
    x = random_matrix(2)
    step = 0.5 * (x[0] - x[1])
    out = moocore.de_current_1_crossover(x, seed=1)
    for i in range(2):
        assert np.allclose(out[i], x[i] + step) or np.allclose(
            out[i], x[i] - step
        )


def test_parent_centric_crossover_tiny_sigmas_return_index_parent():
    x = random_matrix(10)
    out = moocore.parent_centric_crossover(
        x, sigma_zeta=1e-12, sigma_eta=1e-12, seed=1
    )
    assert_allclose(out, x, atol=1e-9)


def test_parent_centric_crossover_sigma_zeta_scales_step_along_direction():
    x = random_matrix(10)
    step1 = (
        moocore.parent_centric_crossover(
            x, sigma_zeta=0.1, sigma_eta=1e-12, seed=1
        )
        - x
    )
    step2 = (
        moocore.parent_centric_crossover(
            x, sigma_zeta=0.2, sigma_eta=1e-12, seed=1
        )
        - x
    )
    assert_allclose(step2, 2 * step1, atol=1e-9)


@pytest.mark.parametrize("name", POPULATION_OPERATORS)
def test_population_operators_clip_to_bounds(name):
    x = random_matrix(30)
    if name == "parent_centric":
        kwargs = {"sigma_zeta": 5.0, "sigma_eta": 5.0}
    else:
        kwargs = {"scaling_factor": 5.0}
    out = POPULATION_OPERATORS[name](
        x, lower=LOWER, upper=UPPER, seed=1, **kwargs
    )
    assert np.all((out >= LOWER) & (out <= UPPER))
    assert np.any((out == LOWER) | (out == UPPER))


@pytest.mark.parametrize("name", ALL_OPERATORS)
def test_errors_shape(name):
    fun = ALL_OPERATORS[name]
    with pytest.raises(ValueError, match=r"must be a 2D array"):
        fun([1.0, 2.0, 3.0])
    with pytest.raises(ValueError, match=r"must have at least \d rows"):
        fun([[1.0, 2.0]])


@pytest.mark.parametrize("name", sorted(BOUNDED))
def test_errors_bounds(name):
    fun = ALL_OPERATORS[name]
    x = random_matrix(6)
    with pytest.raises(ValueError, match=r"'lower' must not be larger"):
        fun(x, lower=1, upper=0)
    with pytest.raises(ValueError, match=r"'upper' must have length 4"):
        fun(x, upper=[1, 2])


@pytest.mark.parametrize(
    ("fun", "kwargs", "match"),
    [
        (moocore.uniform_crossover, {"swap_prob": 1.5}, "'swap_prob'"),
        (moocore.uniform_crossover, {"pair_prob": -0.1}, "'pair_prob'"),
        (moocore.single_arithmetic_crossover, {"pair_prob": 2}, "'pair_prob'"),
        (moocore.blend_alpha_crossover, {"alpha": -1}, "'alpha'"),
        (moocore.bounded_exponential_crossover, {"scale": 0}, "'scale'"),
        (moocore.bounded_exponential_crossover, {"swap_prob": 2}, "swap_prob"),
        (moocore.de_rand_1_bin_crossover, {"scaling_factor": 0}, "scaling"),
        (moocore.de_rand_1_bin_crossover, {"crossover_rate": 2}, "crossover"),
        (moocore.de_current_1_crossover, {"scaling_factor": -1}, "scaling"),
        (moocore.de_current_1_crossover, {"crossover_rate": -1}, "crossover"),
        (moocore.parent_centric_crossover, {"sigma_zeta": 0}, "'sigma_zeta'"),
        (moocore.parent_centric_crossover, {"sigma_eta": 0}, "'sigma_eta'"),
    ],
)
def test_errors_parameters(fun, kwargs, match):
    with pytest.raises(ValueError, match=match):
        fun(random_matrix(6), **kwargs)


def test_errors_operator_specific():
    with pytest.raises(ValueError, match=r"at least 2 columns"):
        moocore.single_point_crossover([[0], [1]])
    with pytest.raises(ValueError, match=r"at least 4 rows"):
        moocore.de_rand_1_bin_crossover(random_matrix(3))
    with pytest.raises(ValueError, match=r"at least 3 rows"):
        moocore.parent_centric_crossover(random_matrix(2))
    with pytest.raises(ValueError, match=r"within 'lower' and 'upper'"):
        moocore.bounded_exponential_crossover(
            [[0.5, 2.0], [0.2, 0.3]], lower=0, upper=1
        )


@pytest.mark.parametrize("name", POPULATION_OPERATORS)
@pytest.mark.parametrize(
    ("targets", "match"),
    [
        ([6], r"within \[0, 5\]"),
        ([-1], r"within \[0, 5\]"),
        ([[0, 1]], "1D vector"),
        ([0.5], "1D vector"),
    ],
)
def test_errors_targets(name, targets, match):
    with pytest.raises(ValueError, match=match):
        POPULATION_OPERATORS[name](random_matrix(6), targets=targets)
