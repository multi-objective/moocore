"""Hypervolume Computation Benchmarks
=======================================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

from bench import Bench, read_datasets_and_filter_dominated, get_range

import numpy as np
import moocore
import pathlib
import matplotlib.pyplot as plt

from botorch.utils.multi_objective.hypervolume import Hypervolume as botorch_HV
import torch
from pymoo.indicators.hv import Hypervolume as pymoo_HV
from jmetal.core.quality_indicator import HyperVolume as jmetal_HV
from nevergrad.optimization.multiobjective import HypervolumeIndicator as ng_HV


# from trieste.acquisition.multi_objective import Pareto as trieste_Pareto
# import tensorflow as tf


# See https://github.com/multi-objective/testsuite/tree/main/data
path_to_data = "../../testsuite/data/"
assert pathlib.Path(path_to_data).expanduser().exists()

files = {
    "DTLZLinearShape.3d": dict(
        file=path_to_data + "DTLZLinearShape.3d.front.1000pts.10",
        range=(500, 3000, 500),
    ),
    "DTLZLinearShape.4d": dict(
        file=path_to_data + "DTLZLinearShape.4d.front.1000pts.10",
        range=(100, 1000, 150),
    ),
    "DTLZLinearShape.5d": dict(
        file=path_to_data + "DTLZLinearShape.5d.front.500pts.10",
        range=(100, 600, 100),
    ),
    "DTLZLinearShape.6d": dict(
        file=path_to_data + "DTLZLinearShape.6d.front.700pts.10",
        range=(100, 500, 100),
    ),
}


def check_values(a, b, what, n, name):
    assert np.isclose(a, b), (
        f"In {name}, maxrow={n}, {what}={b}  not equal to moocore={a}"
    )


title = "HV computation"
file_prefix = "hv"
names = files.keys()
for name in names:
    x = read_datasets_and_filter_dominated(files[name]["file"])
    dim = x.shape[1]
    ref = np.ones(dim)
    n = get_range(len(x), *files[name]["range"])

    benchmarks = {
        "moocore": moocore.Hypervolume(ref=ref),
        "pymoo": lambda z, hv=pymoo_HV(ref_point=ref): hv(z),
        "jMetalPy": lambda z, hv=jmetal_HV(ref): hv.compute(z),
    }
    if dim < 6:
        # Nevergrad is too slow
        benchmarks["nevergrad"] = lambda z, hv=ng_HV(ref): hv.compute(z)
    if dim < 5:
        ## Trieste is hundreds of times slower than botorch. It is so slow that
        ## we cannot run the benchmark with the initial value of 500 points.
        # benchmarks["trieste"] = lambda z, tf_ref=tf.convert_to_tensor(ref): float(trieste_Pareto(z, already_non_dominated=True).hypervolume_indicator(tf_ref))
        benchmarks["botorch"] = lambda z, hv=botorch_HV(
            ref_point=torch.from_numpy(-ref)
        ): hv.compute(z)

    bench = Bench(
        name=name,
        n=n,
        # Exclude the conversion to torch from the timing.
        setup={"botorch": lambda z: torch.from_numpy(-z)},
        # elif what == "trieste":
        #     zz = tf.convert_to_tensor(z)
        bench=benchmarks,
        check=check_values,
    )

    for maxrow in n:
        values = bench(maxrow, x[:maxrow, :])

    bench.plots(file_prefix=file_prefix, title=title)

if "__file__" not in globals():  # Running interactively.
    plt.show()
