"""Dominance Filtering Benchmarks
==============================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

from bench import Bench

import numpy as np
import moocore
import matplotlib.pyplot as plt


import torch
from botorch.utils.multi_objective.pareto import (
    is_non_dominated as botorch_is_non_dominated,
)
from pymoo.util.nds.non_dominated_sorting import (
    find_non_dominated as pymoo_find_non_dominated,
)

# See https://github.com/multi-objective/testsuite/tree/main/data
files = {
    "test2D-200k": dict(file="test2D-200k.inp.xz", ranges=(1000, 10000, 1000)),
    "ran3d-10k": dict(file="ran.1000pts.3d.10", ranges=(1000, 5000, 1000)),
}


def get_n(lenx, start, stop, step):
    return np.arange(start, min(stop, len(x)) + 1, step)


title = "is_non_dominated()"
file_prefix = "ndom"

names = files.keys()
for name in names:
    x = moocore.get_dataset(files[name]["file"])[:, :-1]
    n = get_n(len(x), *files[name]["ranges"])

    bench = Bench(
        name=name,
        n=n,
        bench={
            "moocore": lambda z: np.nonzero(
                moocore.is_nondominated(z, maximise=True)
            )[0],
            "pymoo": lambda z: pymoo_find_non_dominated(-z),
            "botorch": lambda z: np.nonzero(
                np.asarray(botorch_is_non_dominated(z))
            )[0],
        },
    )

    values = {}
    for maxrow in n:
        z = x[:maxrow, :]
        for what in bench.keys():
            if what == "botorch":
                # Exclude the conversion to torch from the timing.
                zz = torch.from_numpy(z)
            else:
                zz = z
            values[what] = bench(what, maxrow, zz)

        # Check values
        for what in bench.keys():
            if what == "moocore":
                continue
            a = values["moocore"]
            b = values[what]
            assert np.allclose(a, b), (
                f"In {name}, maxrow={maxrow}, {what}={b}  not equal to moocore={a}"
            )

    del values
    bench.plots(file_prefix=file_prefix, title=title)

plt.show()
