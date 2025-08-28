"""Dominance Filtering Benchmarks
==============================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

from bench import Bench, get_range

import numpy as np
import moocore
import matplotlib.pyplot as plt


import torch
from botorch.utils.multi_objective.pareto import (
    is_non_dominated as botorch_is_nondominated,
)
from pymoo.util.nds.non_dominated_sorting import (
    NonDominatedSorting as pymoo_NonDominatedSorting,
)

from desdeo.tools.non_dominated_sorting import (
    non_dominated as desdeo_is_nondominated,
)

# See https://github.com/multi-objective/testsuite/tree/main/data
files = {
    "test2D-200k": dict(file="test2D-200k.inp.xz", range=(2000, 20_000, 2000)),
    "ran3d-10k": dict(file="ran.1000pts.3d.10", range=(1000, 10_000, 1000)),
}


title = "is_non_dominated(keep_weakly=False)"
file_prefix = "ndom"


def bool2pos(x):
    return np.nonzero(np.asarray(x))[0]


names = files.keys()
for name in names:
    x = moocore.get_dataset(files[name]["file"])[:, :-1]
    n = get_range(len(x), *files[name]["range"])

    bench = Bench(
        name=name,
        n=n,
        bench={
            "moocore": lambda z: bool2pos(
                moocore.is_nondominated(z, maximise=True, keep_weakly=False)
            ),
            "botorch": lambda z: bool2pos(
                botorch_is_nondominated(z, deduplicate=True)
            ),
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
            np.testing.assert_array_equal(
                a,
                b,
                err_msg=f"In {name}, maxrow={maxrow}, {what}={b}  not equal to moocore={a}",
            )

    del values
    bench.plots(file_prefix=file_prefix, title=title)


title = "is_non_dominated(keep_weakly=True)"
file_prefix = "wndom"

names = files.keys()
for name in names:
    x = moocore.get_dataset(files[name]["file"])[:, :-1]
    n = get_range(len(x), *files[name]["range"])

    bench = Bench(
        name=name,
        n=n,
        bench={
            "moocore": lambda z: bool2pos(
                moocore.is_nondominated(z, maximise=True, keep_weakly=True)
            ),
            "botorch": lambda z: bool2pos(
                botorch_is_nondominated(z, deduplicate=False)
            ),
            "pymoo": lambda z, nds=pymoo_NonDominatedSorting(): nds.do(
                -z, only_non_dominated_front=True
            ),
            "desdeo": lambda z: bool2pos(desdeo_is_nondominated(-z)),
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
            np.testing.assert_allclose(
                a,
                b,
                err_msg=f"In {name}, maxrow={maxrow}, {what}={b}  not equal to moocore={a}",
            )

    del values
    bench.plots(file_prefix=file_prefix, title=title)

plt.show()
