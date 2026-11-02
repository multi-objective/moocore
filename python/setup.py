# ruff: noqa: D100, D101, D102
import sysconfig

from setuptools import setup
from setuptools.command.build_ext import build_ext


class ForceBuildExt(build_ext):
    def build_extension(self, ext):
        self.force = True
        super().build_extension(ext)


setup(
    cmdclass={"build_ext": ForceBuildExt},
    # The free-threaded Stable ABI starts at Python 3.15. GIL-enabled
    # builds retain the existing Python 3.10 Stable ABI wheel family.
    options={
        "bdist_wheel": {
            "py_limited_api": (
                "cp315"
                if sysconfig.get_config_var("Py_GIL_DISABLED")
                else "cp310"
            )
        }
    },
    cffi_modules=["src/moocore/_ffi_build.py:ffibuilder"],
)
