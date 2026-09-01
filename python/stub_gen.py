"""
Script to regenerate type stubs for the _libmoocore C extension.

Requires the moocore package to be installed (e.g. via ``pip install -e .``).
Run with::

    python stub_gen.py

or via tox::

    tox -e stubgen

Notes
-----
cffi-stubgen (https://github.com/LorenzoPeri17/cffi-stubgen) cannot handle:
- Integer constants (their ``__doc__`` is the Python ``int`` docstring, not a
  C declaration).
- Functions that use ``double **`` (pointer-to-pointer) parameters, such as
  ``read_datasets``.

These items are therefore handled manually in this script.

The generated stubs use ``IntOrCData`` / ``FloatOrCData`` union types for C
scalar parameters so that callers can pass plain Python ``int``/``float``
values as cffi accepts at runtime.  Functions that return scalar C types use
``Annotated[CDATA, ...]`` aliases because cffi always returns CData objects.
"""

from __future__ import annotations

import os
import sys

# ---------------------------------------------------------------------------
# Locate the output directory relative to this script.
# ---------------------------------------------------------------------------
_HERE = os.path.dirname(os.path.abspath(__file__))
_STUB_DIR = os.path.join(_HERE, "src", "moocore", "_libmoocore")
_LIB_STUB_DIR = os.path.join(_STUB_DIR, "lib")
os.makedirs(_LIB_STUB_DIR, exist_ok=True)

# ---------------------------------------------------------------------------
# Import the installed module so cffi-stubgen can inspect it.
# ---------------------------------------------------------------------------
try:
    import moocore._libmoocore as _mod
except ImportError as exc:
    sys.exit(
        f"Could not import moocore._libmoocore: {exc}\n"
        "Make sure the package is installed (e.g. 'pip install -e .')."
    )

from cffi_stubgen.functions import parse_func, CType, CVarArg  # type: ignore[import-untyped]

ffi = _mod.ffi
lib = _mod.lib

# ---------------------------------------------------------------------------
# Separate constants (plain int) from callable functions.
# ---------------------------------------------------------------------------
_constants: dict[str, str] = {}
_func_names: list[str] = []

for _name in dir(lib):
    _obj = getattr(lib, _name)
    if isinstance(_obj, int):
        _constants[_name] = "int"
    else:
        _func_names.append(_name)

# ---------------------------------------------------------------------------
# Functions that cffi-stubgen cannot parse: add manual stubs here.
# ---------------------------------------------------------------------------
# read_datasets uses ``double **`` (pointer-to-pointer) which cffi-stubgen
# cannot represent.
_MANUAL_STUBS: dict[str, str] = {
    "read_datasets": (
        "# io.h\n"
        "def read_datasets(\n"
        "    filename: bytes | CDATA,\n"
        "    data_p: CDATA,\n"
        "    ncols_p: int_ptr,\n"
        "    datasize_p: int_ptr,\n"
        ") -> Int: ..."
    ),
}

# C scalar types that cffi accepts Python int/float for at call sites.
# Keys are the cffi-stubgen CType.pyname values.
_INT_SCALAR_TYPES = {
    "Int", "Size_T", "Uint_Fast8_T", "Unsigned", "Unsigned_Int", "UnsignedInt",
    "_Bool", "Uint_Fast32_T", "Uint32_T", "Uint64_T", "Int64_T",
}
_FLOAT_SCALAR_TYPES = {"Double", "Float"}
# Pointer types always require CData.
_POINTER_TYPES = {"double_ptr", "int_ptr", "uint8_t_ptr", "void_ptr", "Uint8_T_Ptr"}


def _param_type(arg_pyname: str, is_return: bool) -> str:
    """Return the stub type name for a parameter or return type."""
    if is_return:
        return arg_pyname  # return types are always CData
    if arg_pyname in _INT_SCALAR_TYPES:
        return "IntOrCData"
    if arg_pyname in _FLOAT_SCALAR_TYPES:
        return "FloatOrCData"
    return arg_pyname  # pointer type or unknown — keep as-is


# ---------------------------------------------------------------------------
# Parse all parseable functions.
# ---------------------------------------------------------------------------
parsed_funcs = []
all_ctypes: dict[str, CType] = {}  # pyname -> CType
failed_funcs: list[str] = []

for fname in _func_names:
    if fname in _MANUAL_STUBS:
        continue
    obj = getattr(lib, fname)
    try:
        results = parse_func(obj)
        for func in results:
            all_ctypes[func.ret_t.pyname] = func.ret_t
            for arg in func.args:
                if arg != CVarArg and hasattr(arg, "ctype"):
                    all_ctypes[arg.ctype.pyname] = arg.ctype
        parsed_funcs.extend(results)
    except Exception:
        failed_funcs.append(fname)

# ---------------------------------------------------------------------------
# Build lib/__init__.pyi
# ---------------------------------------------------------------------------
_lib_lines: list[str] = []

_lib_lines.append("""
from cffi import FFI

from typing import (
    Annotated,
    TypeAlias,
    Union,
)

CDATA: TypeAlias = FFI.CData

""")

# Type aliases for all discovered CTypes
for pyname, ctype in sorted(all_ctypes.items()):
    _lib_lines.append(
        f"{pyname}: TypeAlias = Annotated[CDATA, '{ctype.cname}']\n"
    )

_lib_lines.append("\n")
_lib_lines.append("# cffi accepts Python native scalars for C scalar parameters\n")
_lib_lines.append("IntOrCData: TypeAlias = Union[int, CDATA]\n")
_lib_lines.append("FloatOrCData: TypeAlias = Union[float, CDATA]\n")
_lib_lines.append("\n")

# Constants
if _constants:
    _lib_lines.append("# Constants\n")
    for cname in sorted(_constants.keys()):
        _lib_lines.append(f"{cname}: int\n")
    _lib_lines.append("\n")

# Manual stubs
for mstub in _MANUAL_STUBS.values():
    _lib_lines.append(mstub + "\n\n")

# Auto-generated stubs
for func in parsed_funcs:
    _lib_lines.append(f"def {func.name}(\n")
    for arg in func.args:
        if arg == CVarArg:
            _lib_lines.append("    *args: CDATA,\n")
        else:
            ptype = _param_type(arg.ctype.pyname, is_return=False)
            _lib_lines.append(f"    {arg.name}: {ptype},\n")
    ret = _param_type(func.ret_t.pyname, is_return=True)
    _lib_lines.append(f") -> {ret}: ...\n\n")

if failed_funcs:
    _lib_lines.append(
        f"# NOTE: could not auto-generate stubs for: {failed_funcs}\n"
        "# Add manual stubs above in _MANUAL_STUBS.\n"
    )

lib_stub_path = os.path.join(_LIB_STUB_DIR, "__init__.pyi")
with open(lib_stub_path, "w") as f:
    f.writelines(_lib_lines)
print(f"Written: {lib_stub_path}")

# ---------------------------------------------------------------------------
# Write _libmoocore/__init__.pyi (ffi stub + lib import)
# ---------------------------------------------------------------------------
_ffi_stub = """
from cffi import FFI

from typing import (
    Self,
    Any,
    TypeAlias
)

CDATA : TypeAlias = FFI.CData
CTYPE : TypeAlias = FFI.CType

class _FFI_T(FFI):
    def cast(self: Self, cdecl: str | CTYPE, source: Any) -> CDATA: ...

ffi : _FFI_T = ...

from . import lib
"""

ffi_stub_path = os.path.join(_STUB_DIR, "__init__.pyi")
with open(ffi_stub_path, "w") as f:
    f.write(_ffi_stub)
print(f"Written: {ffi_stub_path}")

# Write py.typed markers
for d in (_STUB_DIR, _LIB_STUB_DIR):
    pt = os.path.join(d, "py.typed")
    if not os.path.exists(pt):
        open(pt, "w").close()
    print(f"Verified: {pt}")

if failed_funcs:
    print(
        f"\nWARNING: could not auto-generate stubs for: {failed_funcs}\n"
        "Add manual stubs to _MANUAL_STUBS in stub_gen.py."
    )
