"""Dominance Filtering Benchmarks
==============================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

import numpy as np
import moocore
import matplotlib.pyplot as plt

from bench import Bench, get_geomrange

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

from paretoset import paretoset

# See https://github.com/multi-objective/testsuite/tree/main/data
files = {
    # range are the (start, stop, num) parameters for np.geomspace()
    "test2D-200k": dict(file="test2D-200k.inp.xz", range=(1000, 50_000, 10)),
    "ran3d-40k": dict(file="ran.40000pts.3d.1.xz", range=(1000, 40_000, 10)),
    "sphere-4d": dict(
        generate=(30000, 4, "sphere", 42), range=(1000, 30000, 10)
    ),
    "sphere-5d": dict(
        generate=(20000, 5, "sphere", 42), range=(1000, 20000, 10)
    ),
}


def bool2pos(x):
    return np.nonzero(np.asarray(x))[0]


def get_dataset(name):
    if "generate" in files[name]:
        n, d, method, seed = files[name]["generate"]
        return moocore.generate_ndset(n, d, method=method, seed=seed)
    return moocore.get_dataset(files[name]["file"])[:, :-1]


title = "is_non_dominated(keep_weakly=False)"
file_prefix = "ndom"
names = files.keys()
for name in names:
    x = get_dataset(name)
    n = get_geomrange(len(x), *files[name]["range"])

    bench = Bench(
        name=name,
        n=n,
        bench={
            "moocore": lambda z: moocore.is_nondominated(
                z, maximise=True, keep_weakly=False
            ),
            "botorch": lambda z: botorch_is_nondominated(z, deduplicate=True),
            "paretoset (numba)": lambda z: paretoset(
                z, sense=z.shape[1] * ["max"], distinct=True, use_numba=True
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
    bench.plots(file_prefix=file_prefix, title=title, log="xy")


title = "is_non_dominated(keep_weakly=True)"
file_prefix = "wndom"

names = files.keys()
for name in names:
    x = get_dataset(name)
    n = get_geomrange(len(x), *files[name]["range"])

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
            "paretoset (numba)": lambda z: bool2pos(
                paretoset(
                    z,
                    sense=z.shape[1] * ["max"],
                    distinct=False,
                    use_numba=True,
                )
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
    bench.plots(file_prefix=file_prefix, title=title, log="xy")


if "__file__" not in globals():  # Running interactively.
    plt.show()
