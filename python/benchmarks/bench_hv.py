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
}


title = "HV computation"
file_prefix = "hv"
names = files.keys()
for name in names:
    x = read_datasets_and_filter_dominated(files[name]["file"])
    ref = np.ones(x.shape[1])
    n = get_range(len(x), *files[name]["range"])

    bench = Bench(
        name=name,
        n=n,
        bench={
            "moocore": moocore.Hypervolume(ref=ref),
            "pymoo": lambda z, hv=pymoo_HV(ref_point=ref): hv(z),
            "jMetalPy": lambda z, hv=jmetal_HV(ref): hv.compute(z),
            "botorch": lambda z,
            hv=botorch_HV(ref_point=torch.from_numpy(-ref)): hv.compute(z),
        },
    )

    values = {}
    for maxrow in n:
        z = x[:maxrow, :]
        for what in bench.keys():
            if what == "botorch":
                zz = torch.from_numpy(-z)
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
