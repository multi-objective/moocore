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


def np2d_to_double_array(x):
    x = np.ascontiguousarray(x)
    nrows = ffi.cast("int", x.shape[0])
    ncols = ffi.cast("int", x.shape[1])
    x = ffi.from_buffer("double []", x)
    return x, nrows, ncols


def np1d_to_double_array(x):
    x = np.ascontiguousarray(x)
    size = ffi.cast("int", x.shape[0])
    x = ffi.from_buffer("double []", x)
    return x, size


def np1d_to_int_array(x):
    x = np.ascontiguousarray(x, dtype=np.intc())
    size = ffi.cast("int", x.shape[0])
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
