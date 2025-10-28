# ruff: noqa: D100
from setuptools import setup

setup(cffi_modules=["src/moocore/_ffi_build.py:ffibuilder"])
