"""Hypervolume Computation Benchmarks
=======================================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

import numpy as np
import moocore
import pathlib
import matplotlib.pyplot as plt
import timeit  # time_hv_exact

from bench import (
    Bench,
    read_datasets_and_filter_dominated,
    get_range,
    timeit_template_return_1_value,  # time_hv_exact
)

from pymoo.indicators.hv.monte_carlo import (
    ApproximateMonteCarloHypervolume as pymoo_hvapprox,
)

# See https://github.com/multi-objective/testsuite/tree/main/data
path_to_data = "../../testsuite/data/"
assert pathlib.Path(path_to_data).expanduser().exists()

files = {
    "DTLZLinearShape.3d": dict(
        file="DTLZLinearShape.3d.front.1000pts.10",
        ref=1,
        range=(100, 1000, 100),
    ),
    "DTLZLinearShape.4d": dict(
        file="DTLZLinearShape.4d.front.1000pts.10",
        ref=1,
        range=(100, 1000, 100),
    ),
    "DTLZLinearShape.6d": dict(
        file="DTLZLinearShape.6d.front.700pts.10.xz",
        ref=1,
        range=(50, 600, 50),
    ),
    "DTLZSphereShape.6d": dict(
        file="DTLZSphereShape.6d.front.500pts.10.xz",
        ref=1.1,
        range=(50, 600, 50),
    ),
    "ran.6d": dict(
        file="ran.800pts.6d.10",
        ref=10,
        range=(50, 600, 50),
    ),
    "ran.9d": dict(
        file="ran.80pts.9d.10",
        ref=10,
        range=(50, 150, 10),
    ),
    "DTLZLinearShape.9d": dict(
        file="DTLZLinearShape.9d.front.60pts.10",
        ref=1,
        range=(50, 150, 10),
    ),
    "DTLZSphereShape.10d": dict(
        file="DTLZSphereShape.10d.front.150pts.10",
        ref=1,
        range=(50, 150, 10),
    ),
}


def relerror(true, approx):
    return abs(true - approx) / true


def time_hv_exact(name, maxrow, z, ref):
    tmp_template = timeit.template
    timeit.template = timeit_template_return_1_value
    duration, exact = timeit.Timer(
        lambda: moocore.hypervolume(z, ref=ref)
    ).timeit(number=1)
    timeit.template = tmp_template
    print(f"{name}:{maxrow}:exact:{duration}")
    return exact


title = "HV approximation"
file_prefix = "hvapprox"

print(f"Running benchmark: {title}")
names = files.keys()
for name in names:
    x = read_datasets_and_filter_dominated(path_to_data + files[name]["file"])
    ref = np.full(x.shape[1], files[name]["ref"], dtype=float)
    n = get_range(len(x), *files[name]["range"])

    benchmarks = {
        "moocore DZ2019-MC": lambda z, exact: relerror(
            exact, moocore.hv_approx(z, ref=ref, method="DZ2019-MC")
        ),
        "moocore DZ2019-HW": lambda z, exact: relerror(
            exact, moocore.hv_approx(z, ref=ref, method="DZ2019-HW")
        ),
        "pymoo": lambda z, exact, hv=pymoo_hvapprox(ref_point=ref): relerror(
            exact, hv.add(z).hv
        ),
    }
    bench = Bench(
        name=name, n=n, bench=benchmarks, report_values="HV Relative Error"
    )
    for maxrow in n:
        z = x[:maxrow, :]
        exact = time_hv_exact(name, maxrow, z, ref)
        res = bench(maxrow, z, exact=exact)

    bench.plots(file_prefix=file_prefix, title=title)

if "__file__" not in globals():  # Running interactively.
    plt.show()
