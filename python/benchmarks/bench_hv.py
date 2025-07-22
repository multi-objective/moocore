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
        range=(300, 1500, 200),
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


title = "HV computation"
file_prefix = "hv"
names = files.keys()
for name in names:
    x = read_datasets_and_filter_dominated(files[name]["file"])
    ref = np.ones(x.shape[1])
    n = get_range(len(x), *files[name]["range"])

    benchmarks = {
        "moocore": moocore.Hypervolume(ref=ref),
        "pymoo": lambda z, hv=pymoo_HV(ref_point=ref): hv(z),
        "jMetalPy": lambda z, hv=jmetal_HV(ref): hv.compute(z),
    }
    if name not in ["DTLZLinearShape.5d", "DTLZLinearShape.6d"]:
        ## Trieste is hundreds of times slower than botorch. It is so slow that
        ## we cannot run the benchmark with the initial value of 500 points.
        # benchmarks["trieste"] = lambda z, tf_ref=tf.convert_to_tensor(ref): float(trieste_Pareto(z, already_non_dominated=True).hypervolume_indicator(tf_ref))
        benchmarks["botorch"] = lambda z, hv=botorch_HV(
            ref_point=torch.from_numpy(-ref)
        ): hv.compute(z)

    bench = Bench(name=name, n=n, bench=benchmarks)
    values = {}
    for maxrow in n:
        z = x[:maxrow, :]
        for what in bench.keys():
            if what == "botorch":
                zz = torch.from_numpy(-z)
            # elif what == "trieste":
            #     zz = tf.convert_to_tensor(z)
            else:
                zz = z
            values[what] = bench(what, maxrow, zz)

        # Check values
        for what in bench.keys():
            if what == "moocore":
                continue
            a = values["moocore"]
            b = values[what]
            assert np.isclose(a, b), (
                f"In {name}, maxrow={maxrow}, {what}={b}  not equal to moocore={a}"
            )

    del values
    bench.plots(file_prefix=file_prefix, title=title)

plt.show()
