from typing import Any

import numpy as np
from numpy.typing import ArrayLike  # For type hints

from ._libmoocore import ffi  # ty: ignore[import-not-found]


def np_asarray_c(x: Any, dtype: type, copy: bool | None = None) -> np.ndarray:
    x = np.asarray(x, dtype=dtype, order="C", copy=copy)
    if not x.flags.aligned:
        # With copy=None, the result may still be unaligned for a double pointer.
        return x.copy(order="C")
    return x


def np_asarray_c_int(x: Any, copy: bool | None = None) -> np.ndarray:
    return np_asarray_c(x, dtype=np.intc, copy=copy)


def np_asarray_c_double(x: Any, copy: bool | None = None) -> np.ndarray:
    return np_asarray_c(x, dtype=float, copy=copy)


def get1_and_is_copied(x: Any, x_: Any) -> tuple[Any, bool]:
    x_copied = id(x) != id(x_)
    return x, x_copied


def asarray_c_double_maybe_copy(
    x: ArrayLike, dtype: type = float
) -> tuple[np.ndarray, bool]:
    """Convert to numpy array of dtype=float and detect copies."""
    return get1_and_is_copied(np_asarray_c_double(x), x)


def unique_nosort(array: ArrayLike, axis: int | None = None) -> np.ndarray:
    """Return unique values without sorting them.

    See https://github.com/numpy/numpy/issues/7265

    """
    uniq, index = np.unique(array, return_index=True, axis=axis)
    return uniq[index.argsort()]


def np2d_to_c_double_array(
    x: np.ndarray, ctype_shape: tuple[str, str] = ("int", "int")
) -> tuple[ffi.CData, ffi.CData, ffi.CData]:
    """Convert from Numpy array to C array.

    Assumes that the input is correct. Otherwise, use np_asarray_c_double().

    """
    nrows = ffi.cast(ctype_shape[0], x.shape[0])
    ncols = ffi.cast(ctype_shape[1], x.shape[1])
    if not (x.dtype == float and x.flags.c_contiguous and x.flags.aligned):
        raise TypeError("Expected a C ordered contiguous aligned float64 array")
    x = ffi.from_buffer("double []", x)
    return x, nrows, ncols


def np1d_to_c_array(
    x: np.ndarray, ctype_data: str, ctype_size: str
) -> tuple[ffi.CData, ffi.CData]:
    """Convert from Numpy array to C array.

    Assumes that the input is correct. Otherwise, use np_asarray_c_double().

    """
    if not (x.flags.c_contiguous and x.flags.aligned):
        raise TypeError("Expected a C ordered contiguous aligned array")
    size = ffi.cast(ctype_size, x.shape[0])
    return ffi.from_buffer(ctype_data + "[]", x), size


def np1d_to_c_double_array(
    x: np.ndarray, ctype_size: str = "int"
) -> tuple[ffi.CData, ffi.CData]:
    """Convert from Numpy array to C array.

    Assumes that the input is correct. Otherwise, use np_asarray_c_double().

    """
    if not x.dtype == float:
        raise TypeError("Expected a float64 array")
    return np1d_to_c_array(x, ctype_data="double", ctype_size=ctype_size)


def np1d_to_c_int_array(
    x: np.ndarray, ctype_size: str = "int"
) -> tuple[ffi.CData, ffi.CData]:
    """Convert from Numpy array to C array.

    Assumes that the input is correct. Otherwise, use np_asarray_c_int().

    """
    if not x.dtype == np.intc:
        raise TypeError(f"Expected a {np.intc} array")
    return np1d_to_c_array(x, ctype_data="int", ctype_size=ctype_size)


def array_1d_of_length_n(x: ArrayLike, n: int, name: str = "x") -> np.ndarray:
    x = np.ravel(x)
    if len(x) == 1:
        return np.full((n), x[0])
    if x.shape[0] == n:
        return x
    raise ValueError(
        f"{name!r} must have length {n}, but it has length {x.shape[0]}"
    )


def is_integer_value(n: Any) -> bool:
    if isinstance(n, int):
        return True
    if n is None:
        return False
    try:
        return bool(n.is_integer())
    except AttributeError:
        return False


def _get_seed_for_c(seed: int | np.random.Generator | None) -> ffi.CData:
    if not is_integer_value(seed):
        seed = np.random.default_rng(seed).integers(2**32 - 2, dtype=np.uint32)
    return ffi.cast("uint32_t", seed)
