"""C library compilation config.

This script is part of the compilation of the C library using CFFi.

Every time a new C function is created, its prototype must be added to the `ffibuilder.cdef` function call

The header files required must be placed in the first argument of `ffibuilder.set_source`, and any additional `.C` files must be added to the `sources` argument of `ffibuilder.set_source`

"""

import os
import platform
from cffi import FFI

libmoocore_h = "src/moocore/libmoocore.h"
sources_path = "src/moocore/libmoocore/"
headers = """
#include "io.h"
#include "hv.h"
#include "igd.h"
#include "nondominated.h"
#include "epsilon.h"
#include "eaf.h"
#include "whv.h"
#include "whv_hype.h"
"""
sources = [
    "avl.c",
    "eaf.c",
    "eafdiff.c",
    "eaf3d.c",
    "hv.c",
    "io.c",
    "pareto.c",
    "mt19937/mt19937.c",
    "rng.c",
    "whv.c",
    "whv_hype.c",
    "libutil.c",  # For fatal_error()
]
sources = [sources_path + f for f in sources]


is_windows = platform.system() == "Windows"


def get_config():
    from distutils.core import Distribution
    from distutils.sysconfig import get_config_vars

    get_config_vars()  # workaround for a bug of distutils, e.g. on OS/X
    config = Distribution().get_command_obj("config")
    return config


def uses_msvc():
    config = get_config()
    return config.try_compile('#ifndef _MSC_VER\n#error "not MSVC"\n#endif')


# Try to detect cross-compilation.
def _get_target_platform(arch_flags, default):
    flags = [f for f in arch_flags.split(" ") if f.strip() != ""]
    try:
        pos = flags.index("-arch")

        return flags[pos + 1].lower()
    except ValueError:
        pass

    return default


MSVC_CFLAGS = ["/GL", "/O2", "/GS-", "/wd4996"]
MSVC_LDFLAGS = ["/LTCG"]
GCC_CFLAGS = ["-flto", "-O3"]

extra_compile_args = []
extra_link_args = []
if is_windows and uses_msvc():
    extra_compile_args.extend(MSVC_CFLAGS)
    extra_link_args.extend(MSVC_LDFLAGS)
else:
    extra_compile_args.extend(GCC_CFLAGS)
    extra_link_args.extend(GCC_CFLAGS)
    target_platform = _get_target_platform(
        os.environ.get("ARCHFLAGS", ""), platform.machine()
    )
    # Optimized version requires SSE2 extensions. They have been around since
    # 2001 so we try to compile it on every recent-ish x86.
    sse2 = target_platform in ("i686", "x86", "x86_64", "AMD64")
    if sse2:
        extra_compile_args.append("-msse2")

cflags = os.environ.get("CFLAGS", "")
if cflags != "":
    extra_compile_args.extend(cflags.split())
ldflags = os.environ.get("LDFLAGS", "")
if ldflags != "":
    extra_link_args.extend(ldflags.split())

ffibuilder = FFI()
file_path = os.path.dirname(os.path.realpath(__file__))
libmoocore_path = os.path.join(file_path, "libmoocore")

with open(libmoocore_h) as f:
    ffibuilder.cdef(f.read())


ffibuilder.set_source(
    "moocore._libmoocore",
    headers,
    sources=sources,
    include_dirs=[libmoocore_path],
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_compile_args,
)

if __name__ == "__main__":
    ffibuilder.compile(verbose=True)
