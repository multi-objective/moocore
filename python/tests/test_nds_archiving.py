# ruff: noqa: D100, D103, N802
import os

import numpy as np
import pytest
from numpy.testing import (
    assert_array_equal,
)

import moocore


def no_print(x):
    return


RANK_QUERY_STRATEGIES = moocore._nds_archive.BaseNDSForest.RANK_QUERY_STRATEGIES


def rank_query_strategies():
    if os.environ.get("MOOCORE_SKIP_SLOW_TESTS") == "1":
        return RANK_QUERY_STRATEGIES[:1]
    return RANK_QUERY_STRATEGIES


@pytest.fixture(scope="module", params=[(1000, 2), (1000, 3), (1000, 10)])
def generate_points(request):
    n, d = request.param
    seed = np.random.default_rng().integers(2**32 - 2)
    print(f"seed={seed}")
    rng = np.random.default_rng(seed)
    array = rng.integers(0, 100, (n, d))
    array.flags.writeable = False
    return array


@pytest.fixture(scope="module")
def pareto_rank_front_sizes(generate_points):
    points = generate_points
    ranks = moocore.pareto_rank(points)
    # FIXME: Use fronts to test exact equality of results.
    # FIXME: Is there a faster way to generate the front sizes?
    ## fronts = [points.compress((g == ranks), axis=0) for g in np.unique(ranks)]
    _, counts = np.unique_counts(ranks)
    return counts


@pytest.mark.parametrize("strategy", rank_query_strategies())
def test_NDSortedArchive_random(
    strategy, generate_points, pareto_rank_front_sizes
):
    points = generate_points
    expected = pareto_rank_front_sizes
    nds_archive = moocore.NDSortedArchive(
        dim=points.shape[1], query_strategy=strategy, has_x_values=True
    )
    for i, p in enumerate(points):
        no_print(f"p = {p}")
        nds_archive.add(p, x=i)

    nds_sizes = nds_archive.fronts_total_sizes()
    assert_array_equal(expected, nds_sizes)


@pytest.mark.parametrize("strategy", rank_query_strategies())
def test_NDSortedArchive(strategy):
    arch = moocore.NDSortedArchive(dim=2, query_strategy=strategy)

    def assert_add(arch, z, rank, total_sizes):
        assert arch.add(z) == rank
        assert_array_equal(arch.fronts_total_sizes(), total_sizes)

    assert_add(arch, [1, 1], 0, [1])
    assert_add(arch, [2, 2], 1, [1, 1])
    assert_add(arch, [0, 3], 0, [2, 1])
    assert_add(arch, [3, 0], 0, [3, 1])
    assert_add(arch, [9, 1], 1, [3, 2])
    assert_add(arch, [1, 9], 1, [3, 3])
    assert_add(arch, [2, 0], 0, [3, 3, 1])
    assert_add(arch, [0, 2], 0, [3, 3, 2])
    assert_add(arch, [0, 0], 0, [1, 3, 3, 2])
    assert_add(arch, [0, 0], 0, [2, 3, 3, 2])
    assert_array_equal(arch.fronts_unique_sizes(), [1, 3, 3, 2])


@pytest.mark.parametrize("dim", [2, 3])
def test_invalid_iterator(dim):
    archive = moocore.NDSortedArchive(dim)
    rng = np.random.default_rng()
    n = 5
    z = rng.uniform(size=(n, dim))
    archive.add_many(z)
    with pytest.raises(RuntimeError, match=r"invalid 'BaseNDSForest' iterator"):
        for r, z, x in archive:
            archive.add(np.full(dim, 1))

    assert len(archive) == n + 1

    # Test that the iterator keeps the archive alive.
    iterator = iter(archive)
    del archive
    r, z, x = next(iterator)
    assert r < n + 1
    assert len(z) == dim
    assert x is None
