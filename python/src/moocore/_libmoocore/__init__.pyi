
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
