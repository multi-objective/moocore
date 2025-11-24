# ruff: noqa: D100, D101, D102
from setuptools import setup
from setuptools.command.build_ext import build_ext


class ForceBuildExt(build_ext):
    def build_extension(self, ext):
        self.force = True
        super().build_extension(ext)


setup(
    cmdclass={"build_ext": ForceBuildExt},
    cffi_modules=["src/moocore/_ffi_build.py:ffibuilder"],
)
