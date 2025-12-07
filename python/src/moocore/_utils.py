from numpy.typing import ArrayLike  # For type hints
from typing import Any

import numpy as np
from ._libmoocore import ffi


def get1_and_is_copied(x: Any, x_: Any) -> tuple[Any, bool]:
    x_copied = id(x) != id(x_)
    return x, x_copied


def asarray_maybe_copy(
    x: ArrayLike, dtype: type = float
) -> tuple[np.ndarray, bool]:
    """Convert to numpy array of dtype=float and detect copies."""
    return get1_and_is_copied(np.asarray(x, dtype=dtype), x)


def unique_nosort(array: ArrayLike, **kwargs: Any) -> np.ndarray:
    """Return unique values without sorting them.

    See https://github.com/numpy/numpy/issues/7265

    """
    uniq, index = np.unique(array, return_index=True, **kwargs)
    return uniq[index.argsort()]


def np2d_to_double_array(
    x: ArrayLike, ctype_shape: tuple[str, str] = ("int", "int")
) -> tuple[ffi.CData, ffi.CData, ffi.CData]:
    nrows = ffi.cast(ctype_shape[0], x.shape[0])
    ncols = ffi.cast(ctype_shape[1], x.shape[1])
    # FIXME: This may cause an unexpected copy. Make this an assert and force
    # the caller to enforce it if needed.
    x = np.ascontiguousarray(x)
    x = ffi.from_buffer("double []", x)
    return x, nrows, ncols


def np1d_to_c_array(
    x: ArrayLike, ctype_data: str, ctype_size: str
) -> tuple[ffi.CData, ffi.CData]:
    size = ffi.cast(ctype_size, x.shape[0])
    ctype_dtype = np.intc() if ctype_data == "int" else None
    x = np.ascontiguousarray(x, dtype=ctype_dtype)
    x = ffi.from_buffer(ctype_data + "[]", x)
    return x, size


def np1d_to_double_array(
    x: ArrayLike, ctype_size: str = "int"
) -> tuple[ffi.CData, ffi.CData]:
    return np1d_to_c_array(x, ctype_data="double", ctype_size=ctype_size)


def np1d_to_int_array(
    x: ArrayLike, ctype_size: str = "int"
) -> tuple[ffi.CData, ffi.CData]:
    return np1d_to_c_array(x, ctype_data="int", ctype_size=ctype_size)


def array_1d_of_length_n(x: ArrayLike, n: int) -> np.ndarray:
    x = np.ravel(x)
    if len(x) == 1:
        return np.full((n), x[0])
    if x.shape[0] == n:
        return x
    raise ValueError(
        f"1D array must have length {n} but it has length {x.shape[0]}"
    )


def is_integer_value(n: Any) -> bool:
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


def _get_seed_for_c(seed: Any) -> ffi.CData:
    if not is_integer_value(seed):
        seed = np.random.default_rng(seed).integers(2**32 - 2, dtype=np.uint32)
    return ffi.cast("uint32_t", seed)
