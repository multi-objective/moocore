# ruff: noqa: D100, D101, D102, N801
from setuptools import setup
from wheel.bdist_wheel import bdist_wheel as _bdist_wheel


class bdist_wheel_abi3(_bdist_wheel):
    def finalize_options(self):
        _bdist_wheel.finalize_options(self)
        # Automatically detect py_limited_api from extension modules
        if not self.py_limited_api:
            for ext in self.distribution.ext_modules or []:
                if getattr(ext, "py_limited_api", False):
                    # Use cp310 as the minimum Python version for abi3
                    self.py_limited_api = "cp310"
                    break


setup(
    cffi_modules=["src/moocore/_ffi_build.py:ffibuilder"],
    cmdclass={"bdist_wheel": bdist_wheel_abi3},
)
