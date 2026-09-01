from __future__ import annotations

from numpy.typing import ArrayLike  # For type hints
from typing import Any

import numpy as np
from cffi import FFI
from ._libmoocore import ffi


def get1_and_is_copied(x: Any, x_: Any) -> tuple[Any, bool]:
    x_copied = id(x) != id(x_)
    return x, x_copied


def asarray_maybe_copy(
    x: ArrayLike, dtype: type = float
) -> tuple[np.ndarray, bool]:
    """Convert to numpy array of dtype=float and detect copies."""
    return get1_and_is_copied(np.asarray(x, dtype=dtype), x)


def unique_nosort(array: ArrayLike, axis: int | None = None) -> np.ndarray:
    """Return unique values without sorting them.

    See https://github.com/numpy/numpy/issues/7265

    """
    uniq, index = np.unique(array, return_index=True, axis=axis)
    return np.asarray(uniq[index.argsort()])


def np2d_to_double_array(
    x: np.ndarray, ctype_shape: tuple[str, str] = ("int", "int")
) -> tuple[FFI.CData, FFI.CData, FFI.CData]:
    nrows = ffi.cast(ctype_shape[0], x.shape[0])
    ncols = ffi.cast(ctype_shape[1], x.shape[1])
    # FIXME: This may cause an unexpected copy. Make this an assert and force
    # the caller to enforce it if needed.
    x_c = np.ascontiguousarray(x)
    x_buf = ffi.from_buffer("double []", x_c)
    return x_buf, nrows, ncols


def np1d_to_c_array(
    x: ArrayLike, ctype_data: str, ctype_size: str
) -> tuple[FFI.CData, FFI.CData]:
    ctype_dtype = np.intc() if ctype_data == "int" else None
    x_arr = np.ascontiguousarray(x, dtype=ctype_dtype)
    size = ffi.cast(ctype_size, x_arr.shape[0])
    x_buf = ffi.from_buffer(ctype_data + "[]", x_arr)
    return x_buf, size


def np1d_to_double_array(
    x: ArrayLike, ctype_size: str = "int"
) -> tuple[FFI.CData, FFI.CData]:
    return np1d_to_c_array(x, ctype_data="double", ctype_size=ctype_size)


def np1d_to_int_array(
    x: ArrayLike, ctype_size: str = "int"
) -> tuple[FFI.CData, FFI.CData]:
    return np1d_to_c_array(x, ctype_data="int", ctype_size=ctype_size)


def array_1d_of_length_n(x: ArrayLike, n: int, name: str = "x") -> np.ndarray:
    x_arr = np.ravel(x)
    if len(x_arr) == 1:
        return np.full((n), x_arr[0])
    if x_arr.shape[0] == n:
        return x_arr
    raise ValueError(
        f"{name!r} must have length {n}, but it has length {x_arr.shape[0]}"
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


def _get_seed_for_c(seed: int | np.random.Generator | None) -> FFI.CData:
    if not is_integer_value(seed):
        seed_int: int = int(
            np.random.default_rng(seed).integers(2**32 - 2, dtype=np.uint32)
        )
    else:
        seed_int = int(seed)  # type: ignore[arg-type]
    return ffi.cast("uint32_t", seed_int)
