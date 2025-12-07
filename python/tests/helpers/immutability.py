# ruff: noqa: D100
import copy
from collections.abc import Mapping
import numpy as np
from numpy.testing import assert_array_equal


def deep_copy(obj):
    """Deep copy with NumPy awareness and nested structure support."""
    if isinstance(obj, np.ndarray):
        return obj.copy()

    if isinstance(obj, Mapping):  # dict-like
        return {k: deep_copy(v) for k, v in obj.items()}

    if isinstance(obj, (list, tuple)):
        copied = [deep_copy(x) for x in obj]
        return type(obj)(copied)

    return copy.deepcopy(obj)


def deep_equal(a, b):
    """Deep equality that handles nested containers and NumPy arrays."""
    if type(a) is not type(b):
        return False

    if isinstance(a, np.ndarray):
        try:
            assert_array_equal(a, b, strict=True)
        except Exception as e:
            print(e)
            return False
        else:
            return True

    if isinstance(a, Mapping):
        if a.keys() != b.keys():
            return False
        return all(deep_equal(a[k], b[k]) for k in a)

    if isinstance(a, (list, tuple)):
        if len(a) != len(b):
            return False
        return all(deep_equal(x, y) for x, y in zip(a, b))

    return a == b


def freeze_numpy(obj):
    """Set NumPy arrays inside nested structures to writeable=False."""
    if isinstance(obj, np.ndarray):
        obj.setflags(write=False)
        return

    if isinstance(obj, Mapping):
        for v in obj.values():
            freeze_numpy(v)

    if isinstance(obj, (list, tuple)):
        for x in obj:
            freeze_numpy(x)
