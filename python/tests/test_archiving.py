# ruff: noqa: D100, D103, N802
import numpy as np
import pytest
from numpy.testing import (
    assert_array_equal,
)

import moocore


def extend_point(z, dim):
    z = np.asarray(z)
    z_ext = np.zeros(dim)
    z_ext[-len(z) :] = z
    return z_ext


@pytest.mark.parametrize("dim", [2, 3])
@pytest.mark.parametrize("keep_duplicates", [True, False])
def test_UnboundedArchive_add(dim, keep_duplicates):
    archive = moocore.UnboundedArchive(dim=dim, keep_duplicates=keep_duplicates)

    def add(z):
        return archive.add(extend_point(z, dim))

    assert add([1, 10])
    assert len(archive) == 1
    assert add([10, 1])
    assert len(archive) == 2
    assert add([5, 5])
    assert len(archive) == 3
    assert archive.unique_len() == 3

    # Duplicate handling differs only in the total length.
    assert add([5, 5]) is keep_duplicates
    assert len(archive) == (4 if keep_duplicates else 3)
    assert archive.unique_len() == 3

    assert add([4, 4])
    assert len(archive) == 3
    assert not add([5, 5])
    assert len(archive) == 3
    assert add([1, 1])
    assert len(archive) == 1


@pytest.mark.parametrize("dim", [2, 3])
def test_UnboundedArchive_add_error(dim):

    archive_without_x = moocore.UnboundedArchive(dim=dim, keep_duplicates=True)
    archive_without_x.add(extend_point([1, 10], dim))
    with pytest.raises(ValueError, match=r"'x' must be None"):
        archive_without_x.add(extend_point([1, 1], dim), x=2)

    archive_with_x = moocore.UnboundedArchive(dim=dim, has_x_values=True)
    archive_with_x.add(extend_point([1, 10], dim), x="a")
    with pytest.raises(ValueError, match=r"'x' cannot be None"):
        archive_with_x.add(extend_point([10, 1], dim))


@pytest.mark.parametrize("dim", [2, 3])
def test_UnboundedArchive_accessors(dim, monkeypatch):
    points = [extend_point([1, 10], dim), extend_point([10, 1], dim)]
    without_x = moocore.UnboundedArchive(dim=dim)

    with pytest.raises(ValueError, match="'z' must be a 1D objective vector"):
        without_x.add(points)
    with pytest.raises(ValueError, match=f"'f' must have {dim} columns"):
        without_x.add_many(np.array(points)[:, : dim - 1])
    assert without_x.add(points[0])
    monkeypatch.setattr(
        without_x, "get_contents", lambda: (np.asarray(points[0]), None)
    )
    assert repr(without_x) == str(np.asarray(points[0]))

    archive = moocore.UnboundedArchive(dim=dim, has_x_values=True)
    assert archive.dim == dim
    assert archive.add_many(points, x=["a", "b"]) == [True, True]
    vectors, values = archive.get_contents()
    assert_array_equal(vectors, points)
    assert values == ["a", "b"]
    assert_array_equal(archive.get_unique_vectors(), points)
    assert archive.get_x_values() == values
    assert archive.get_x_values_of_vector(points[0]) == ["a"]
    assert "x = ['a', 'b']" in repr(archive)

    with pytest.raises(ValueError, match="'x' must be a list with"):
        archive.add_many(points, x=["only one"])


def test_UnboundedArchive_validation(monkeypatch):
    with pytest.raises(ValueError, match="'dim' cannot be smaller than 2"):
        moocore.UnboundedArchive(dim=1)
    with pytest.raises(NotImplementedError, match="type of archive"):
        moocore.UnboundedArchive(dim=float("nan"))

    with pytest.raises(ValueError, match="max_children"):
        moocore.UnboundedArchive(dim=3, max_children=255)
    with pytest.raises(ValueError, match="max_bucket_size"):
        moocore.UnboundedArchive(dim=3, max_bucket_size=255)

    archive = moocore.UnboundedArchive(dim=3, max_children=4, max_bucket_size=5)
    assert (archive.max_children, archive.max_bucket_size) == (4, 5)

    with monkeypatch.context() as patch:
        patch.setattr(
            moocore._archive.NDTreeArchive,
            "_get_default_config",
            classmethod(lambda cls, dim, children, bucket: (4, 20)),
        )
        with pytest.raises(ValueError, match="larger than"):
            moocore.UnboundedArchive(
                dim=moocore._archive.lib.MOOCORE_DIMENSION_MAX + 1
            )

    with monkeypatch.context() as patch:
        patch.setattr(
            moocore._archive.NDTreeArchive,
            "_setup_c_archive",
            lambda self, allow_duplicated, has_x_values: (
                moocore._archive.ffi.NULL
            ),
        )
        with pytest.raises(MemoryError):
            moocore._archive.NDTreeArchive(dim=3)


@pytest.mark.parametrize(
    ("status", "error", "message"),
    [
        (
            moocore._archive.lib.ARCHIVE_INSERT_X_MUST_BE_NOT_NULL,
            ValueError,
            "'x' cannot be None",
        ),
        (
            moocore._archive.lib.ARCHIVE_INSERT_X_MUST_BE_NULL,
            ValueError,
            "'x' must be None",
        ),
        (moocore._archive.lib.ARCHIVE_INSERT_MEMORY_ERROR, MemoryError, None),
        (-999, NotImplementedError, "unknown error"),
    ],
)
@pytest.mark.parametrize("dim", [2, 3])
def test_UnboundedArchive_add_errors_has_x(
    monkeypatch, dim, status, error, message
):
    archive = moocore.UnboundedArchive(dim=dim, has_x_values=True)
    monkeypatch.setattr(
        archive, "_lib_archive_add_and_get_displaced", lambda *args: status
    )
    with pytest.raises(error, match=message):
        archive.add(np.arange(dim), x="a")


@pytest.mark.parametrize(
    ("status", "error", "message"),
    [
        (
            moocore._archive.lib.ARCHIVE_INSERT_X_MUST_BE_NOT_NULL,
            ValueError,
            "'x' cannot be None",
        ),
        (moocore._archive.lib.ARCHIVE_INSERT_MEMORY_ERROR, MemoryError, None),
        (-999, NotImplementedError, "unknown error"),
    ],
)
@pytest.mark.parametrize("dim", [2, 3])
def test_UnboundedArchive_add_errors_no_x(
    monkeypatch, dim, status, error, message
):
    archive = moocore.UnboundedArchive(dim=dim, has_x_values=False)
    monkeypatch.setattr(
        archive, "_lib_archive_add_and_discard", lambda *args: status
    )
    with pytest.raises(error, match=message):
        archive.add(np.arange(dim))


@pytest.mark.parametrize("dim", [2, 3])
def test_UnboundedArchive_iterator_allocation_error(monkeypatch, dim):
    archive = moocore.UnboundedArchive(dim=dim)
    archive.add(np.arange(dim))
    monkeypatch.setattr(
        archive, "_lib_archive_iter_new", lambda *_: moocore._archive.ffi.NULL
    )
    with pytest.raises(MemoryError, match="failed to create iterator"):
        list(archive)


@pytest.mark.parametrize("dim", [2, 3])
def test_UnboundedArchive_dominance(dim):
    def full(v):
        return [v] * dim

    def pt(*head, fill=0):
        # First coordinates given explicitly, remaining ones set to fill.
        return [*head, *([fill] * (dim - len(head)))]

    low_high = [1, 10] * dim
    p_a = low_high[:dim]
    p_c = [11 - v for v in p_a]
    archive = moocore.UnboundedArchive(dim=dim, keep_duplicates=True)
    assert_array_equal(
        archive.add_many([p_a, full(5), p_c]),
        [True, True, True],
    )
    # Test __contains__
    assert full(5) in archive
    assert full(4) not in archive

    assert not archive.dominated_by(full(4))
    assert not archive.dominated_by(full(5))
    assert not archive.dominated_by(full(6))
    assert archive.dominated_by(full(1))

    assert archive.dominates(full(6))  # One point in the archive dominates.
    assert archive.dominates(full(10))  # Whole archive dominates.
    # One point in the archive is dominated.
    assert not archive.dominates(full(4))
    # One point in the archive is weakly dominated.
    assert not archive.dominates(full(5))
    # Mutually non-dominated
    assert not archive.dominates(pt(11))
    assert not archive.dominates(pt(6, 4))
    assert not archive.dominates(full(1))

    assert archive.add(full(1))
    assert len(archive) == 1
    # Duplicated do not count as dominance.
    assert not archive.dominated_by(full(1))
    assert not archive.dominated_by(pt(2, fill=1))
    assert not archive.dominated_by([1, 2, *full(1)[2:]])
    assert archive.dominated_by(pt(0, 1, fill=0))
    assert archive.dominated_by(pt(1, fill=0))

    # Duplicated do not count as dominance.
    assert not archive.dominates(full(1))
    assert not archive.dominates(pt(1, fill=0))
    assert not archive.dominates(pt(0, 1, fill=0))
    assert archive.dominates([2, *full(1)[2:], 20])
    assert archive.dominates([1, 2, *full(4)[2:]])


@pytest.mark.parametrize("dim", [2, 3])
@pytest.mark.parametrize("keep_duplicates", [True, False])
def test_UnboundedArchive_random(dim, keep_duplicates, count):
    """Compare insertion and domination against the reference filter."""
    seed = np.random.default_rng().integers(2**32 - 2)
    print(f"seed={seed}")
    rng = np.random.default_rng(seed)
    points = rng.integers(0, 100, (1000, dim))
    archive = moocore.UnboundedArchive(
        dim, keep_duplicates=keep_duplicates, has_x_values=True
    )
    for p in points:
        archive.add(p, x=count())

    z, x = archive.get_contents()
    points = moocore.filter_dominated(points, keep_weakly=keep_duplicates)
    z = z.astype(int)
    assert_array_equal(points, z[np.argsort(x), :])
    assert archive.dominated_by(points.min(axis=0) - 1)


@pytest.mark.parametrize("dim", [2, 3])
def test_invalid_iterator(dim):
    archive = moocore.UnboundedArchive(dim)
    rng = np.random.default_rng()
    z = rng.uniform(size=(3, dim))
    archive.add_many(z)

    with pytest.raises(
        RuntimeError, match=r"invalid 'BaseUnboundedArchive' iterator"
    ):
        for _ in archive:
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
    archive = moocore.UnboundedArchive(dim, keep_duplicates=True)
    z = np.arange(dim, dtype=float)
    archive.add(z)

    iterator = iter(archive)
    next(iterator)
    assert archive.add(z)

    with pytest.raises(
        RuntimeError, match=r"invalid 'BaseUnboundedArchive' iterator"
    ):
        next(iterator)


@pytest.mark.parametrize("dim", [2, 3])
def test_maximise(dim):
    maximise = np.array([True, False, True])
    n = 20
    first = n // 2
    z_all = np.random.uniform(size=(n, dim))
    z = z_all[:first, :]
    z_orig = z.copy()

    arch = moocore.UnboundedArchive(
        dim=dim, maximise=maximise[:dim], has_x_values=True
    )
    res = arch.add_many(z, x=np.arange(len(z)))
    assert_array_equal(res, moocore.is_nondominated(z, maximise=maximise[:dim]))
    for z, x in arch:
        assert_array_equal(z, z_orig[x, :])
    z, x = arch.get_contents()
    assert_array_equal(z, z_orig[np.array(x), :])

    z = z_all[first:, :]
    res = arch.add_many(z, x=np.arange(len(z)))
    nondom = moocore.is_nondominated(z_all, maximise=maximise[:dim])
    assert_array_equal(res, nondom[first:])


@pytest.mark.parametrize("dim", [2, 3])
def test_find_exact(dim):
    z = moocore.generate_ndset(1000, dim, method="simplex", integer=True)
    arch = moocore.UnboundedArchive(dim=dim, has_x_values=True)
    arch.add_many(z, x=np.arange(len(z)))
    for i in [1, 500, 990]:
        res = arch.get_x_values_of_vector(z[i, :])
        assert res[0] == i

    y = z.max(axis=0)
    res = arch.get_x_values_of_vector(y)
    assert len(res) == 0


@pytest.mark.parametrize("dim", [2, 3])
@pytest.mark.parametrize("keep_duplicates", [True, False])
def test_empty_archive(dim, keep_duplicates):
    """Test that operations on an empty archive work as expected."""
    arch = moocore.UnboundedArchive(dim=dim, keep_duplicates=keep_duplicates)
    z = np.full(dim, 1)

    assert len(arch) == 0
    assert arch.unique_len() == 0
    assert not arch.has_x_values()
    assert z not in arch
    assert not arch.dominates(z)
    assert not arch.dominated_by(z)
    with pytest.raises(ValueError, match="does not store x-values"):
        x = arch.get_x_values_of_vector(z)
    assert list(arch) == []
    z, x = arch.get_contents()
    assert len(z) == 0
    assert x is None
    z = arch.get_unique_vectors()
    assert len(z) == 0
    with pytest.raises(ValueError, match="does not store x-values"):
        x = arch.get_x_values()


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


@pytest.mark.parametrize(
    "config", [{"max_children": 255}, {"max_bucket_size": 255}]
)
def test_ndtree_rejects_out_of_range_configuration(config):
    param = next(iter(config))
    value = config[param]
    with pytest.raises(
        ValueError, match=f"'{param}' must be smaller than {value}"
    ):
        moocore._archive.NDTreeArchive(dim=3, **config)
