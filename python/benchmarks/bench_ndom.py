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

from seqme.core.rank import is_pareto_front as seqme_is_pareto_front

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


# Exclude the conversion to torch from the timing.
setup = {"botorch": lambda z: torch.from_numpy(z)}


def check_values(a, b, what, n, name):
    np.testing.assert_allclose(
        a,
        b,
        err_msg=f"In {name}, maxrow={n}, {what}={b}  not equal to moocore={a}",
    )


title = "is_non_dominated(keep_weakly=False)"
file_prefix = "ndom"
names = files.keys()
for name in names:
    x = get_dataset(name)
    n = get_geomrange(len(x), *files[name]["range"])

    bench = Bench(
        name=name,
        n=n,
        setup=setup,
        bench={
            "moocore": lambda z: moocore.is_nondominated(
                z, maximise=True, keep_weakly=False
            ),
            "botorch": lambda z: botorch_is_nondominated(z, deduplicate=True),
            "paretoset (numba)": lambda z: paretoset(
                z, sense=z.shape[1] * ["max"], distinct=True, use_numba=True
            ),
        },
        check=check_values,
    )

    for maxrow in n:
        values = bench(maxrow, x[:maxrow, :])

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
        setup=setup,
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
            "seqme": lambda z: bool2pos(
                seqme_is_pareto_front(-z, assume_unique_lexsorted=True)
            ),
        },
        check=check_values,
    )

    for maxrow in n:
        values = bench(maxrow, x[:maxrow, :])

    bench.plots(file_prefix=file_prefix, title=title, log="xy")


if "__file__" not in globals():  # Running interactively.
    plt.show()
