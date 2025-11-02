import numpy as np
from ._libmoocore import ffi


def get1_and_is_copied(x, x_):
    x_copied = id(x) != id(x_)
    return x, x_copied


def asarray_maybe_copy(x, dtype=float):
    """Convert to numpy array of dtype=float and detect copies."""
    return get1_and_is_copied(np.asarray(x, dtype=dtype), x)


def unique_nosort(array, **kwargs):
    """Return unique values without sorting them.

    See https://github.com/numpy/numpy/issues/7265

    """
    uniq, index = np.unique(array, return_index=True, **kwargs)
    return uniq[index.argsort()]


def np2d_to_double_array(x, ctype_shape=("int", "int")):
    nrows = ffi.cast(ctype_shape[0], x.shape[0])
    ncols = ffi.cast(ctype_shape[1], x.shape[1])
    # FIXME: This may cause an unexpected copy. Make this an assert and force
    # the caller to enforce it if needed.
    x = np.ascontiguousarray(x)
    x = ffi.from_buffer("double []", x)
    return x, nrows, ncols


def np1d_to_c_array(x, ctype_data, ctype_size):
    size = ffi.cast(ctype_size, x.shape[0])
    ctype_dtype = np.intc() if ctype_data == "int" else None
    x = np.ascontiguousarray(x, dtype=ctype_dtype)
    x = ffi.from_buffer(ctype_data + "[]", x)
    return x, size


def np1d_to_double_array(x, ctype_size="int"):
    return np1d_to_c_array(x, ctype_data="double", ctype_size=ctype_size)


def np1d_to_int_array(x, ctype_size="int"):
    return np1d_to_c_array(x, ctype_data="int", ctype_size=ctype_size)


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


def _get_seed_for_c(seed):
    if not is_integer_value(seed):
        seed = np.random.default_rng(seed).integers(2**32 - 2, dtype=np.uint32)
    return ffi.cast("uint32_t", seed)
