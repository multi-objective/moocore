"""Hypervolume Computation Benchmarks
=======================================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

from bench import Bench, read_datasets_and_filter_dominated, get_range

import numpy as np
import moocore
import pathlib
import matplotlib.pyplot as plt

from pymoo.indicators.hv.monte_carlo import (
    ApproximateMonteCarloHypervolume as pymoo_hvapprox,
)

# See https://github.com/multi-objective/testsuite/tree/main/data
path_to_data = "../../testsuite/data/"
assert pathlib.Path(path_to_data).expanduser().exists()

files = {
    # "DTLZLinearShape.3d": dict(
    #     file=path_to_data + "DTLZLinearShape.3d.front.1000pts.10",
    #     range=(100, 1200, 100),
    # ),
    # "DTLZLinearShape.4d": dict(
    #     file=path_to_data + "DTLZLinearShape.4d.front.1000pts.10",
    #     range=(300, 1500, 200),
    # ),
    # "DTLZLinearShape.5d": dict(
    #     file=path_to_data + "DTLZLinearShape.5d.front.500pts.10",
    #     range=(100, 600, 100),
    # ),
    "DTLZLinearShape.6d": dict(
        file=path_to_data + "DTLZLinearShape.6d.front.700pts.10",
        range=(100, 1000, 100),
    ),
    "DTLZSphereShape.10d": dict(
        file=path_to_data + "DTLZSphereShape.10d.front.150pts.10",
        range=(50, 500, 50),
    ),
}

title = "HV approximation"
file_prefix = "hvapprox"
names = files.keys()
for name in names:
    x = read_datasets_and_filter_dominated(files[name]["file"])
    ref = np.ones(x.shape[1])
    n = get_range(len(x), *files[name]["range"])

    benchmarks = {
        "moocore DZ2019-MC": lambda z, exact: abs(
            exact - moocore.hv_approx(z, ref=ref, method="DZ2019-MC")
        ),
        "moocore DZ2019-HW": lambda z, exact: abs(
            exact - moocore.hv_approx(z, ref=ref, method="DZ2019-HW")
        ),
        "pymoo": lambda z, exact, hv=pymoo_hvapprox(ref_point=ref): abs(
            exact - hv.add(z).hv
        ),
    }
    bench = Bench(name=name, n=n, bench=benchmarks, report_values="HV Error")
    res = {}
    for maxrow in n:
        z = x[:maxrow, :]
        exact = moocore.hypervolume(z, ref=ref)
        for what in bench.keys():
            res[what] = bench(what, maxrow, z, exact=exact)

    del res
    bench.plots(file_prefix=file_prefix, title=title)

plt.show()
