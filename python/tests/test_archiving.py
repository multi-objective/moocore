# ruff: noqa: D100, D103, N802
import pytest
import numpy as np
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
    archive = moocore.UnboundedArchive(dim=2)
    assert archive.add([1, 10])
    assert len(archive) == 1
    assert archive.add([10, 1])
    assert len(archive) == 2
    assert archive.add([5, 5])
    assert len(archive) == 3
    assert archive.unique_len() == 3
    # Duplicates increases len()
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
    pf = moocore.UnboundedArchive(dim)

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

    pf2 = moocore.UnboundedArchive(dim)
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

    archive = moocore.UnboundedArchive(dim=dim)

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

    archive = moocore.UnboundedArchive(dim=3)
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
    pf = moocore.UnboundedArchive(dim)

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

    pf2 = moocore.UnboundedArchive(dim)
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
def test_UnboundedArchive_random(dim, count):
    """Randomized tests for any number of dimensions."""
    seed = np.random.default_rng().integers(2**32 - 2)
    print(f"seed={seed}")
    rng = np.random.default_rng(seed)
    points = rng.integers(0, 100, (1000, dim))
    archive = moocore.UnboundedArchive(dim)
    for p in points:
        archive.add(p, x=count())

    z, x = archive.get_contents()
    points = moocore.filter_dominated(points, keep_weakly=True)
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

    for z, x in archive:
        del archive
