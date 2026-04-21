# ruff: noqa: F403, F405
from spack.package import *


class PyMoocore(PythonPackage):
    """Core Algorithms for Multi-Objective Optimization."""

    homepage = "https://multi-objective.github.io/moocore/python/"
    pypi = "moocore/moocore-0.2.0.tar.gz"

    version(
        "0.2.0",
        sha256="3dc601f85f9a4743ed50ddd027dca30e3bb55c899916a092c2ece495b1b2de08",
    )

    depends_on("c", type="build")
    depends_on("gmake@4.4:", type="build")

    depends_on("python@3.10:", type=("build", "run"))
    depends_on("py-setuptools@77.0.3:", type="build")
    depends_on("py-wheel", type="build")
    depends_on("py-cffi@1.16:", type=("build", "run"))
    depends_on("py-numpy@1.24:", type=("build", "run"))
    depends_on("py-platformdirs", type=("build", "run"))
