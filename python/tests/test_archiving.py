# ruff: noqa: D100, D103, N802
import numpy as np
import pytest
from numpy.testing import (
    assert_array_equal,
)

import moocore


def no_print(x):
    return


def extend_point(z, dim):
    z = np.asarray(z)
    z_ext = np.zeros(dim)
    z_ext[-len(z) :] = z
    return z_ext


def test_UnboundedArchive2D_add():
    archive = moocore.UnboundedArchive(dim=2, allow_duplicates=True)
    assert archive.add([1, 10])
    assert len(archive) == 1
    assert archive.add([10, 1])
    assert len(archive) == 2
    assert archive.add([5, 5])
    assert len(archive) == 3
    assert archive.unique_len() == 3
    # Duplicates increase len()
    assert archive.add([5, 5])
    assert len(archive) == 4
    assert archive.unique_len() == 3
    assert archive.add([4, 4])
    assert len(archive) == 3
    assert not archive.add([5, 5])
    assert len(archive) == 3
    assert archive.add([1, 1])
    assert len(archive) == 1

    # Test mixing up x = None and x not None.
    with pytest.raises(ValueError, match=r"'x' must be None"):
        archive.add([1, 1], x=2)

    with pytest.raises(ValueError, match=r"'x' must be None"):
        archive.add([0, 0], x=2)

    archive = moocore.UnboundedArchive(dim=2)
    archive.add([1, 10], x="a")
    with pytest.raises(ValueError, match=r"'x' cannot be None"):
        archive.add([10, 1])


def test_UnboundedArchive2D_add_nodups():
    archive = moocore.UnboundedArchive(dim=2, allow_duplicates=False)
    assert archive.add([1, 10])
    assert len(archive) == 1
    assert archive.add([10, 1])
    assert len(archive) == 2
    assert archive.add([5, 5])
    assert len(archive) == 3
    assert archive.unique_len() == 3
    assert not archive.add([5, 5])
    assert len(archive) == 3
    assert archive.unique_len() == 3


def test_UnboundedArchive2D_dominance():
    archive = moocore.UnboundedArchive(dim=2)
    assert_array_equal(
        archive.add_many([[1, 10], [5, 5], [10, 1]]), [True, True, True]
    )
    # Test __contains__
    assert [5, 5] in archive
    assert [4, 4] not in archive

    assert not archive.dominated_by([4, 4])
    assert not archive.dominated_by([5, 5])
    assert not archive.dominated_by([6, 6])
    assert archive.dominated_by([1, 1])

    assert archive.dominates([6, 6])  # One point in the archive dominates.
    assert archive.dominates([10, 10])  # Whole archive dominates.
    assert not archive.dominates(
        [4, 4]
    )  # One point in the archive is dominated.
    assert not archive.dominates(
        [5, 5]
    )  # One point in the archive is weakly dominated.
    assert not archive.dominates([6, 4])  # Mutually non-dominated
    assert not archive.dominates([1, 1])

    assert archive.add([1, 1])
    assert len(archive) == 1
    assert not archive.dominated_by(
        [1, 1]
    )  # Duplicated do not count as dominance.
    assert not archive.dominated_by([2, 1])
    assert not archive.dominated_by([1, 2])
    assert archive.dominated_by([0, 1])
    assert archive.dominated_by([1, 0])

    assert not archive.dominates(
        [1, 1]
    )  # Duplicated do not count as dominance.
    assert not archive.dominates([1, 0])
    assert not archive.dominates([0, 1])
    assert archive.dominates([2, 1])
    assert archive.dominates([1, 2])


def test_UnboundedArchive2D_displaced(count):
    dim = 2
    pf = moocore.UnboundedArchive(dim, allow_duplicates=True)

    accepted, displaced = pf._add_with_displaced_raw(
        [1, 10], x=count(), check=False, discard=True
    )
    assert accepted and displaced is None
    accepted, displaced = pf._add_with_displaced_raw(
        [10, 1], x=count(), check=False, discard=True
    )
    assert accepted and displaced is None

    assert pf.dominated_by([1, 1])
    assert not pf.dominated_by([2, 9])

    accepted, displaced = pf._add_with_displaced_raw(
        [2, 9], x=count(), check=True, discard=False
    )
    assert accepted and displaced is None

    assert not pf.dominated_by([2, 9])
    assert not pf.dominated_by([9, 2])
    assert not pf.dominates([2, 9])
    assert not pf.dominates([9, 2])
    assert pf.dominates([3, 10])

    accepted, displaced = pf._add_with_displaced_raw(
        [3, 8], x=count(), check=True, discard=False
    )
    assert accepted and displaced is None
    accepted, displaced = pf._add_with_displaced_raw(
        [3, 8], x=count(), check=True, discard=False
    )
    assert accepted and displaced is None
    accepted, displaced = pf._add_with_displaced_raw(
        [2, 10], x=count(), check=True, discard=False
    )
    assert not accepted and displaced is None
    accepted, displaced_1 = pf._add_with_displaced_raw(
        [0, 10], x=count(), check=True, discard=False
    )
    assert accepted and displaced_1 is not None
    accepted, displaced_2 = pf._add_with_displaced_raw(
        [2, 8], x=count(), check=True, discard=False
    )
    assert accepted and displaced_2 is not None

    z, x = pf.get_contents()
    no_print(z)
    no_print(x)

    # We have to use allow_duplicates = True in both pf and pf2, otherwise it
    # will crash.
    pf2 = moocore.UnboundedArchive(dim, allow_duplicates=True)
    accepted, displaced = pf2._add_with_displaced_raw(
        [0, 11], x=count(), check=False, discard=False
    )
    assert accepted and displaced is None
    accepted, displaced = pf2._add_with_displaced_raw(
        [11, 1], x=count(), check=False, discard=False
    )
    assert accepted and displaced is None
    accepted, displaced = pf2._add_with_displaced_raw(
        [10, 3], x=count(), check=True, discard=False
    )
    assert accepted and displaced is None
    accepted, displaced = pf2._add_with_displaced_raw(
        [4, 9], x=count(), check=True, discard=False
    )
    assert accepted and displaced is None
    z, x = pf2.get_contents()
    no_print(z)
    no_print(x)
    displaced = pf2._add_displaced_raw(displaced_1)
    assert displaced is None
    displaced = pf2._add_displaced_raw(displaced_2)
    assert displaced is not None
    no_print(displaced)
    z, x = pf2.get_contents()
    no_print(z)
    no_print(x)


@pytest.mark.parametrize("dim", [3, 4, 5, 10])
def test_UnboundedArchive_add(dim):

    archive = moocore.UnboundedArchive(dim=dim, allow_duplicates=True)

    def archive_add_extended(z, **kwargs):
        z_dim = extend_point(z, dim)
        return archive.add(z_dim, **kwargs)

    def assert_add(z, expect_res, expect_len):
        assert archive_add_extended(z) == expect_res
        assert len(archive) == expect_len

    assert_add([1, 10, 1], True, 1)
    assert_add([10, 1, 10], True, 2)
    assert_add([5, 5, 5], True, 3)
    assert archive.unique_len() == 3
    # Duplicates increase len()
    assert_add([5, 5, 5], True, 4)
    assert archive.unique_len() == 3
    assert_add([4, 4, 4], True, 3)
    assert_add([5, 5, 5], False, 3)
    assert_add([1, 1, 1], True, 1)

    # Test mixing up x = None and x not None.
    with pytest.raises(ValueError, match=r"'x' must be None"):
        archive_add_extended([2, 0, 1], x=2)

    with pytest.raises(ValueError, match=r"'x' must be None"):
        archive_add_extended([1, 1, 1], x=2)

    with pytest.raises(ValueError, match=r"'x' must be None"):
        archive_add_extended([0, 0, 0], x=2)

    archive = moocore.UnboundedArchive(dim=dim)
    archive_add_extended([1, 10, 1], x="a")
    with pytest.raises(ValueError, match=r"'x' cannot be None"):
        archive_add_extended([10, 1, 10])


# FIXME: How to generalize this test to any number of dimensions?
def test_UnboundedArchive3D_dominance():

    archive = moocore.UnboundedArchive(dim=3, allow_duplicates=True)
    assert_array_equal(
        archive.add_many([[1, 10, 1], [5, 5, 5], [10, 1, 10]]),
        [True, True, True],
    )
    # Test __contains__
    assert [5, 5, 5] in archive
    assert [4, 4, 4] not in archive

    assert not archive.dominated_by([4, 4, 4])
    assert not archive.dominated_by([5, 5, 5])
    assert not archive.dominated_by([6, 6, 6])
    assert archive.dominated_by([1, 1, 1])

    assert archive.dominates([6, 6, 6])  # One point in the archive dominates.
    assert archive.dominates([10, 10, 10])  # Whole archive dominates.
    assert not archive.dominates(
        [4, 4, 4]
    )  # One point in the archive is dominated.
    assert not archive.dominates(
        [5, 5, 5]
    )  # One point in the archive is weakly dominated.
    assert not archive.dominates([11, 1, 1])  # Mutually non-dominated
    assert not archive.dominates([1, 1, 1])

    assert archive.add([1, 1, 1])
    assert len(archive) == 1
    assert not archive.dominated_by(
        [1, 1, 1]
    )  # Duplicated do not count as dominance.
    assert not archive.dominated_by([2, 1, 1])
    assert not archive.dominated_by([1, 2, 1])
    assert archive.dominated_by([0, 1, 0])
    assert archive.dominated_by([1, 0, 0])

    assert not archive.dominates(
        [1, 1, 1]
    )  # Duplicated do not count as dominance.
    assert not archive.dominates([1, 0, 0])
    assert not archive.dominates([0, 1, 0])
    assert archive.dominates([2, 1, 20])
    assert archive.dominates([1, 2, 4])


# FIXME: How to generalize this test to any number of dimensions?
def test_UnboundedArchive3D_displaced(count):
    dim = 3
    pf = moocore.UnboundedArchive(dim, allow_duplicates=True)

    accepted, displaced = pf._add_with_displaced_raw(
        [1, 11, 10], x=count(), check=False, discard=True
    )
    assert accepted and displaced is None
    accepted, displaced = pf._add_with_displaced_raw(
        [10, 1, 10], x=count(), check=False, discard=True
    )
    assert accepted and displaced is None

    assert pf.dominated_by([1, 1, 1])
    assert not pf.dominated_by([2, 9, 1])

    accepted, displaced = pf._add_with_displaced_raw(
        [2, 9, 1], x=count(), check=True, discard=False
    )
    assert accepted and displaced is None

    assert not pf.dominated_by([2, 9, 1])
    assert not pf.dominated_by([9, 2, 1])
    assert not pf.dominates([2, 9, 1])
    assert not pf.dominates([9, 2, 1])
    assert pf.dominates([3, 10, 10])

    accepted, displaced = pf._add_with_displaced_raw(
        [3, 8, 10], x=count(), check=True, discard=False
    )
    assert accepted and displaced is None
    accepted, displaced = pf._add_with_displaced_raw(
        [3, 8, 10], x=count(), check=True, discard=False
    )
    assert accepted and displaced is None
    accepted, displaced = pf._add_with_displaced_raw(
        [2, 10, 10], x=count(), check=True, discard=False
    )
    assert not accepted and displaced is None
    accepted, displaced_1 = pf._add_with_displaced_raw(
        [0, 10, 9], x=count(), check=True, discard=False
    )
    assert accepted and displaced_1 is not None
    accepted, displaced_2 = pf._add_with_displaced_raw(
        [2, 8, 1], x=count(), check=True, discard=False
    )
    assert accepted and displaced_2 is not None

    z, x = pf.get_contents()
    no_print(z)
    no_print(x)

    pf2 = moocore.UnboundedArchive(dim, allow_duplicates=True)
    accepted, displaced = pf2._add_with_displaced_raw(
        [
            1,
            10,
            11,
        ],
        x=count(),
        check=False,
        discard=False,
    )
    assert accepted and displaced is None
    accepted, displaced = pf2._add_with_displaced_raw(
        [10, 1, 11], x=count(), check=False, discard=False
    )
    assert accepted and displaced is None
    accepted, displaced = pf2._add_with_displaced_raw(
        [11, 3, 9], x=count(), check=True, discard=False
    )
    assert accepted and displaced is None
    accepted, displaced = pf2._add_with_displaced_raw(
        [4, 11, 8], x=count(), check=True, discard=False
    )
    assert accepted and displaced is None
    z, x = pf2.get_contents()
    no_print(z)
    no_print(x)
    displaced = pf2._add_displaced_raw(displaced_1)
    assert displaced is None
    displaced = pf2._add_displaced_raw(displaced_2)
    assert displaced is not None
    no_print(displaced)
    z, x = pf2.get_contents()
    no_print(z)
    no_print(x)


@pytest.mark.parametrize("dim", [2, 3, 5, 10])
@pytest.mark.parametrize("allow_duplicates", [True, False])
def test_UnboundedArchive_random(dim, allow_duplicates, count):
    """Randomized tests for any number of dimensions."""
    seed = np.random.default_rng().integers(2**32 - 2)
    print(f"seed={seed}")
    rng = np.random.default_rng(seed)
    points = rng.integers(0, 100, (1000, dim))
    archive = moocore.UnboundedArchive(dim, allow_duplicates=allow_duplicates)
    for p in points:
        archive.add(p, x=count())

    z, x = archive.get_contents()
    points = moocore.filter_dominated(points, keep_weakly=allow_duplicates)
    z = z.astype(int)
    assert_array_equal(points, z[np.argsort(x), :])


@pytest.mark.parametrize("dim", [2, 3])
def test_invalid_iterator(dim):
    archive = moocore.UnboundedArchive(dim)
    rng = np.random.default_rng()
    z = rng.uniform(size=(3, dim))
    archive.add_many(z)

    with pytest.raises(
        RuntimeError, match=r"invalid 'BaseUnboundedArchive' iterator"
    ):
        for z, x in archive:
            archive.add(np.full(dim, 0))

    # archive.add() succeeded.
    assert len(archive) == 1

    # Test that the iterator keeps the archive alive.
    iterator = iter(archive)
    del archive
    z, x = next(iterator)
    assert len(z) == dim
    assert x is None


@pytest.mark.parametrize("dim", [2, 3])
def test_duplicate_add_invalidates_iterator(dim):
    archive = moocore.UnboundedArchive(dim, allow_duplicates=True)
    z = np.arange(dim, dtype=float)
    archive.add(z)

    iterator = iter(archive)
    next(iterator)
    assert archive.add(z)

    with pytest.raises(
        RuntimeError, match=r"invalid 'BaseUnboundedArchive' iterator"
    ):
        next(iterator)


def test_ndtree_internal_split_and_pruning():
    archive = moocore._archive.NDTreeArchive(
        dim=3, max_children=4, max_bucket_size=4
    )
    points = [[i, 10 - i, 0] for i in range(10)]
    exceptional = [100, 100, -1]

    for point in [*points, exceptional]:
        assert archive.add(point)

    assert archive.max_bucket_size == 4
    assert {tuple(z) for z, _ in archive} == {
        tuple(point) for point in [*points, exceptional]
    }
    assert archive.dominates([11, 11, 0])

    # This point removes several ND-tree subtrees but leaves the exceptional
    # point, which is not dominated in the third objective.
    assert archive.add([-1, -1, 0])
    assert {tuple(z) for z, _ in archive} == {
        tuple(exceptional),
        (-1.0, -1.0, 0.0),
    }

    # Removing the remaining subtree resets the root leaf, which remains usable.
    assert archive.add([-2, -2, -2])
    assert {tuple(z) for z, _ in archive} == {(-2.0, -2.0, -2.0)}
    assert archive.add([-3, -3, -1])


def test_ndtree_displaced_duplicate_payloads(count):
    source = moocore._archive.NDTreeArchive(
        dim=3,
        max_children=4,
        max_bucket_size=4,
        allow_duplicates=True,
    )
    points = [[i, 10 - i, 0] for i in range(8)]
    x_values = [count() for _ in points]
    source.add_many(points, x=x_values)
    duplicate_x = count()
    assert source.add(points[3], x=duplicate_x)

    accepted, displaced = source._add_with_displaced_raw(
        [-1, -1, -1], x=count(), check=True, discard=False
    )
    assert accepted and displaced is not None
    assert len(source) == 1

    destination = moocore._archive.NDTreeArchive(
        dim=3,
        max_children=4,
        max_bucket_size=4,
        allow_duplicates=True,
    )
    displaced = destination._add_displaced_raw(displaced)
    assert displaced is None

    contents, contents_x = destination.get_contents()
    assert {tuple(z) for z in contents} == {tuple(z) for z in points}
    assert len(contents_x) == len(points) + 1
    assert destination.get_x_values_of_vector(points[3]) == [
        x_values[3],
        duplicate_x,
    ]


def test_ndtree_displaced_duplicate_counts_without_payloads():
    source = moocore._archive.NDTreeArchive(
        dim=3,
        max_children=4,
        max_bucket_size=4,
        allow_duplicates=True,
    )
    points = [[i, 10 - i, 0] for i in range(8)]
    source.add_many(points)
    assert source.add(points[3])

    accepted, displaced = source._add_with_displaced_raw(
        [-1, -1, -1], x=None, check=True, discard=False
    )
    assert accepted and displaced is not None

    destination = moocore._archive.NDTreeArchive(
        dim=3,
        max_children=4,
        max_bucket_size=4,
        allow_duplicates=True,
    )
    assert destination._add_displaced_raw(displaced) is None

    contents, x_values = destination.get_contents()
    assert len(contents) == len(points) + 1
    assert x_values == []
    assert destination.unique_len() == len(points)
    assert np.count_nonzero(np.all(contents == points[3], axis=1)) == 2


def test_treap_displaced_frontier_union(count):
    source = moocore.UnboundedArchive(dim=2, allow_duplicates=True)
    original = [[1, 10], [2, 9], [5, 8], [6, 7], [7, 6], [10, 1]]
    original_x = [count() for _ in original]
    source.add_many(original, x=original_x)

    accepted, displaced = source._add_with_displaced_raw(
        [4, 6], x=count(), check=True, discard=False
    )
    assert accepted and displaced is not None
    assert len(source) == 4

    destination = moocore.UnboundedArchive(dim=2, allow_duplicates=True)
    assert destination.add([0, 11], x=count())
    assert destination.add([10, 0], x=count())
    assert destination._add_displaced_raw(displaced) is None

    contents, contents_x = destination.get_contents()
    contents_by_point = {
        tuple(z): x for z, x in zip(contents, contents_x, strict=True)
    }
    assert contents_by_point == {
        (0.0, 11.0): 7,
        (5.0, 8.0): 2,
        (6.0, 7.0): 3,
        (7.0, 6.0): 4,
        (10.0, 0.0): 8,
    }
    assert set(contents_by_point) == {
        (0.0, 11.0),
        (5.0, 8.0),
        (6.0, 7.0),
        (7.0, 6.0),
        (10.0, 0.0),
    }
    assert len(contents_x) == len(contents)


@pytest.mark.parametrize(
    ("config", "message"),
    [
        ({"max_children": 255}, "'max_children' must be smaller than 255"),
        ({"max_bucket_size": 255}, "'max_bucket_size' must be smaller than 255"),
    ],
)
def test_ndtree_rejects_out_of_range_configuration(config, message):
    with pytest.raises(ValueError, match=message):
        moocore._archive.NDTreeArchive(dim=3, **config)


def test_unbounded_archive_input_validation():
    archive = moocore.UnboundedArchive(dim=3)

    with pytest.raises(ValueError, match="'z' must be a 1D objective vector"):
        archive.add([[1, 2, 3]])

    with pytest.raises(ValueError, match="'f' must have 3 columns"):
        archive.add_many([[1, 2], [3, 4]])

    with pytest.raises(ValueError, match="'x' must be either None"):
        archive.add_many([[1, 2, 3]], x=[])


@pytest.mark.parametrize("dim", [2, 3])
def test_maximise(dim):
    maximise = np.array([True, False, True])
    n = 10
    z = np.random.uniform(size=(n, dim))
    z_orig = z.copy()

    arch = moocore.UnboundedArchive(dim=dim, maximise=maximise[:dim])
    arch.add_many(z, x=np.arange(len(z)))
    for z, x in arch:
        assert np.array_equal(z, z_orig[x, :])
    for z, x in arch:
        assert np.array_equal(z, z_orig[x, :])

    z, x = arch.get_contents()
    assert np.array_equal(z, z_orig[np.array(x), :])

    z, x = arch.get_contents()
    assert np.array_equal(z, z_orig[np.array(x), :])


@pytest.mark.parametrize("dim", [2, 3])
def test_find_exact(dim):
    z = moocore.generate_ndset(1000, dim, method="simplex", integer=True)
    arch = moocore.UnboundedArchive(dim=dim)
    arch.add_many(z, x=np.arange(len(z)))
    for i in [1, 500, 990]:
        res = arch.get_x_values_of_vector(z[i, :])
        assert res[0] == i

    y = z.max(axis=0)
    res = arch.get_x_values_of_vector(y)
    assert len(res) == 0


@pytest.mark.parametrize("dim", [2, 3])
def test_empty_archive(dim):
    """Test that operations on an empty archive works as expected."""
    arch = moocore.UnboundedArchive(dim=dim)
    z = np.full(dim, 1)

    assert len(arch) == 0
    assert arch.unique_len() == 0
    assert not arch.has_x_values()
    assert z not in arch
    assert not arch.dominates(z)
    assert not arch.dominated_by(z)
    x = arch.get_x_values_of_vector(z)
    assert len(x) == 0
    for z, _ in arch:
        assert False
    z, x = arch.get_contents()
    assert len(z) == 0
    assert len(x) == 0
    z = arch.get_unique_vectors()
    assert isinstance(z, np.ndarray)
    assert z.shape == (0, dim)
    x = arch.get_x_values()
    assert len(x) == 0


def test_ndtree_dominated_by():
    dim = 3
    arch = moocore.UnboundedArchive(dim=dim)
    n = 1000
    z = np.random.uniform(size=(n, dim))
    # We cannot use add_many() because it may filter before insertion.
    for z_i in z:
        arch.add(z_i)
    nondom = moocore.filter_dominated(z)
    ideal = nondom.min(axis=0)
    assert arch.dominated_by(ideal)
