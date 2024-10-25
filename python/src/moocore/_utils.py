import numpy as np
from ._libmoocore import ffi


def get1_and_is_copied(x, x_):
    x_copied = id(x) != id(x_)
    return x, x_copied


def asarray_maybe_copy(x, dtype=float):
    """Convert to numpy array of dtype=float and detect copies."""
    return get1_and_is_copied(np.asarray(x, dtype=dtype), x)


def unique_nosort(array, **kwargs):
    uniq, index = np.unique(array, return_index=True, **kwargs)
    return uniq[index.argsort()]


def groupby(x, groups, /, *, axis: int = 0):
    """Split an array into groups.

    See https://github.com/numpy/numpy/issues/7265

    Parameters
    ----------
    x : ndarray
        Array to be divided into sub-arrays.
    groups : 1-D array
        A ndarray of length equal to the selected `axis`. The values are used as-is to determine the groups and do not need to be sorted.
    axis :
        The axis along which to split, default is 0.

    Yields
    ------
    sub-array : ndarray
        Sub-arrays of `x`.

    """
    index = unique_nosort(groups)
    for g in index:
        yield x.compress(g == groups, axis=axis)


def np2d_to_double_array(x):
    nrows = ffi.cast("int", x.shape[0])
    ncols = ffi.cast("int", x.shape[1])
    # FIXME: This may cause an unexpected copy. Make this an assert and force
    # the caller to enforce it if needed.
    x = np.ascontiguousarray(x)
    x = ffi.from_buffer("double []", x)
    return x, nrows, ncols


def np1d_to_double_array(x):
    size = ffi.cast("int", x.shape[0])
    x = np.ascontiguousarray(x)
    x = ffi.from_buffer("double []", x)
    return x, size


def np1d_to_int_array(x):
    size = ffi.cast("int", x.shape[0])
    x = np.ascontiguousarray(x, dtype=np.intc())
    x = ffi.from_buffer("int []", x)
    return x, size


def atleast_1d_of_length_n(x, n):
    x = np.atleast_1d(x)
    if len(x) == 1:
        return np.full((n), x[0])
    if x.shape[0] == n:
        return x
    raise ValueError(
        f"1D array must have length {n} but it has length {x.shape[0]}"
    )


def is_integer_value(n):
    if isinstance(n, int):
        return True
    if n is None:
        return False
    # FIXME: When we bump to Python 3.12, we can use float().is_integer()
    try:
        return int(n) == n
    except ValueError:
        return False
    except TypeError:
        return False
