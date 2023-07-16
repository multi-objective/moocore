import numpy as np
from ._libmoocore import ffi


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
        x = np.full((n), x[0])
    elif x.shape[0] != n:
        raise ValueError(
            f"array must have same number of elements as data columns {x.shape[0]} != {n}"
        )
    return x
