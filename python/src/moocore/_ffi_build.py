"""C library compilation config.

This script is part of the compilation of the C library using CFFi.

Every time a new C function is created, its prototype must be added to the `ffibuilder.cdef` function call

The header files required must be placed in the first argument of `ffibuilder.set_source`, and any additional `.C` files must be added to the `sources` argument of `ffibuilder.set_source`

"""

import os
import platform
from cffi import FFI

# Compile in debug mode.
DEBUG = int(os.environ.get("MOOCORE_DEBUG", "0"))

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
#include "hvapprox.h"
"""
sources = [
    "avl.c",
    "eaf.c",
    "eaf3d.c",
    "eafdiff.c",
    "hv.c",
    "hvapprox.c",
    "hv3dplus.c",
    "hv4d.c",
    "hvc3d.c",
    "hv_contrib.c",
    "io.c",
    "libutil.c",  # For fatal_error()
    "mt19937/mt19937.c",
    "pareto.c",
    "rng.c",
    "whv.c",
    "whv_hype.c",
]
sources = [sources_path + f for f in sources]


def get_config():
    from distutils.core import Distribution
    from distutils.sysconfig import get_config_vars

    get_config_vars()  # workaround for a bug of distutils, e.g. on OS/X
    config = Distribution().get_command_obj("config")
    return config


def uses_msvc():
    config = get_config()
    return config.try_compile('#ifndef _MSC_VER\n#error "not MSVC"\n#endif')


def _get_target_platform():
    arch_flags = os.environ.get("ARCHFLAGS", "")
    flags = [f for f in arch_flags.split(" ") if f.strip() != ""]
    try:
        pos = flags.index("-arch")

        return flags[pos + 1].lower()
    except ValueError:
        pass

    return platform.machine()


is_windows = platform.system() == "Windows"
target_platform = _get_target_platform()
is_x86_64 = target_platform in ("i686", "x86", "x86_64", "AMD64")

# Compiler flags.
MSVC_CFLAGS = ["/GL", "/O2", "/GS-", "/wd4996", "/DMOOCORE_SHARED_LIB"]
MSVC_LDFLAGS = ["/LTCG"]
GCC_CFLAGS = ["-O3", "-flto", "-fvisibility=hidden"]
if is_x86_64:
    # Compile for sufficiently old x86-64 architecture.
    MSVC_CFLAGS += ["/arch:AVX"]
    MSVC_LDFLAGS += ["/arch:AVX"]
    GCC_CFLAGS += ["-march=x86-64-v2"]

extra_compile_args = []
extra_link_args = []
if is_windows and uses_msvc():
    extra_compile_args.extend(MSVC_CFLAGS)
    extra_link_args.extend(MSVC_LDFLAGS)
else:
    extra_compile_args.extend(GCC_CFLAGS)
    extra_link_args.extend(GCC_CFLAGS)

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

# This is not done automatically by ffibuilder.compile(debug=True) !
undef_macros = ["NDEBUG"] if DEBUG >= 1 else []

ffibuilder.set_source(
    "moocore._libmoocore",
    headers,
    sources=sources,
    define_macros=[("DEBUG", str(DEBUG))],
    undef_macros=undef_macros,
    include_dirs=[libmoocore_path],
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args,
    py_limited_api=True,  # build against the stable ABI (abi3)
)

if __name__ == "__main__":
    ffibuilder.compile(verbose=True, debug=bool(DEBUG))
