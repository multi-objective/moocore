# ruff: noqa: D100, D103
from numpy.testing import (
    assert_array_equal,
)
import moocore


def test_unbounded():
    arch = moocore.UnboundedArchive(m=3)
    assert len(arch) == 0
    arch.add([0, 1, 1])
    assert len(arch) == 1
    arch.add([[1, 0, 1], [0, 1, 1]])
    assert len(arch) == 2
    assert len(arch.delete(arch.indexof([1, 0, 1]))) > 0
    assert len(arch) == 1
    arch.add([[2, 1, 1], [3, 0, 1]])
    assert len(arch) == 2
    assert len(arch.delete(arch.indexof([2, 1, 1]))) == 0
    assert [0, 1, 1] in arch
    assert [0, 0, 1] not in arch
    left, right = arch.merge_nondominated([[0, 1, 1], [2, 0, 1], [1, 0, 2]])
    assert_array_equal(left, [True, False])
    assert_array_equal(right, [False, True, True])


def test_unbounded_hv():
    arch = moocore.UnboundedArchive(
        m=3, online_metrics="hypervolume", ref_point=10
    )
    assert len(arch) == 0
    arch.add([0, 1, 1])
    arch.hypervolume()
    assert len(arch) == 1
    arch.add([[1, 0, 1], [0, 1, 1]])
    assert len(arch) == 2
    assert len(arch.delete(arch.indexof([1, 0, 1]))) > 0
    assert len(arch) == 1
    arch.add([[2, 1, 1], [3, 0, 1]])
    assert len(arch) == 2
    assert len(arch.delete(arch.indexof([2, 1, 1]))) == 0
    assert [0, 1, 1] in arch
    assert [0, 0, 1] not in arch
    left, right = arch.merge_nondominated([[0, 1, 1], [2, 0, 1], [1, 0, 2]])
    assert_array_equal(left, [True, False])
    assert_array_equal(right, [False, True, True])


def test_unbounded_xval():
    arch = moocore.UnboundedArchive(m=3)
    assert len(arch) == 0
    arch.add([0, 1, 1], x="a")
    assert len(arch) == 1
    arch.add([[1, 0, 1], [0, 1, 1]], x=["b", "c"])
    assert len(arch) == 2
    assert arch.delete(arch.indexof([1, 0, 1])) == ["b"]
    assert len(arch) == 1
    arch.add([[2, 1, 1], [3, 0, 1]], x=[4, 5])
    assert len(arch) == 2
    assert len(arch.delete(arch.indexof([2, 1, 1]))) == 0
    assert [0, 1, 1] in arch
    assert [0, 0, 1] not in arch
    left, right = arch.merge_nondominated([[0, 1, 1], [2, 0, 1], [1, 0, 2]])
    assert_array_equal(left, [True, False])
    assert_array_equal(right, [False, True, True])
