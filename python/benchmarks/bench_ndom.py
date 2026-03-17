"""Dominance Filtering Benchmarks
==============================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

import gc
import numpy as np
import moocore
import matplotlib.pyplot as plt

from bench import Bench, get_geomrange, check_float_vector

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

## paretobench is always slower than botorch, too slower for benchmarking.
# import paretobench
from paretoset import paretoset
from seqme.core.rank import is_pareto_front as seqme_is_pareto_front
from fast_pareto import is_pareto_front as fast_pareto_is_pf
from optuna.study._multi_objective import _is_pareto_front as optuna_is_pf


# See https://github.com/multi-objective/testsuite/tree/main/data
files = {
    # range are the (start, stop, num) parameters for np.geomspace()
    "test2D-200k": dict(file="test2D-200k.inp.xz", range=(1000, 50_000, 10)),
    "ran3d-40k": dict(file="ran.40000pts.3d.1.xz", range=(1000, 40_000, 10)),
    "sphere-4d": dict(
        generate=(30000, 4, "sphere", 42), range=(1000, 30000, 10)
    ),
    "convex-4d": dict(
        generate=(30000, 4, "convex-sphere", 42), range=(1000, 30000, 10)
    ),
    "sphere-5d": dict(
        generate=(25000, 5, "sphere", 42), range=(1000, 25000, 10)
    ),
    "rmnk-10d": dict(
        file="rmnk_0.0_10_16_1_0_ref.txt.xz", range=(1000, 20000, 10)
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

title = "is_non_dominated(keep_weakly=True)"
file_prefix = "wndom"

print(f"Running benchmark: {title}")
names = files.keys()
for name in names:
    x = get_dataset(name)
    dim = x.shape[1]
    n = get_geomrange(len(x), *files[name]["range"])

    benchmarks = {
        "moocore": lambda z: bool2pos(
            moocore.is_nondominated(z, maximise=True, keep_weakly=True)
        ),
        "botorch": lambda z: bool2pos(
            botorch_is_nondominated(z, deduplicate=False)
        ),
        "paretoset (numba)": lambda z: bool2pos(
            paretoset(
                z, sense=z.shape[1] * ["max"], distinct=False, use_numba=True
            )
        ),
        "pymoo": lambda z, nds=pymoo_NonDominatedSorting(): nds.do(
            -z, only_non_dominated_front=True
        ),
        "desdeo": lambda z: bool2pos(desdeo_is_nondominated(-z)),
        "seqme": lambda z: bool2pos(
            seqme_is_pareto_front(-z, assume_unique_lexsorted=True)
        ),
        "fast_pareto": lambda z: bool2pos(
            fast_pareto_is_pf(-z, assume_unique_lexsorted=False)
        ),
        "optuna": lambda z: bool2pos(
            optuna_is_pf(-z, assume_unique_lexsorted=False)
        ),
    }

    bench = Bench(
        name=name,
        n=n,
        setup=setup,
        bench=benchmarks,
        check=check_float_vector,
        max_time=10,
    )

    bench(lambda n: x[:n, :])
    gc.collect()
    bench.plots(file_prefix=file_prefix, title=title, log="xy")


title = "is_non_dominated(keep_weakly=False)"
file_prefix = "ndom"

print(f"Running benchmark: {title}")
names = files.keys()
for name in names:
    x = get_dataset(name)
    dim = x.shape[1]
    # Deduplicate so that we can benchmark packages that do not remove duplicates.
    x = np.unique(x, axis=0)
    n = get_geomrange(len(x), *files[name]["range"])

    benchmarks = {
        "moocore": lambda z: bool2pos(
            moocore.is_nondominated(z, maximise=True, keep_weakly=False)
        ),
        "botorch": lambda z: bool2pos(
            botorch_is_nondominated(z, deduplicate=True)
        ),
        "paretoset (numba)": lambda z: bool2pos(
            paretoset(
                z, sense=z.shape[1] * ["max"], distinct=True, use_numba=True
            )
        ),
        # The following packages do not support deduplication so they are
        # actually slower because the user needs to remove duplicates.
        "pymoo": lambda z, nds=pymoo_NonDominatedSorting(): nds.do(
            -z, only_non_dominated_front=True
        ),
        "desdeo": lambda z: bool2pos(desdeo_is_nondominated(-z)),
        "seqme": lambda z: bool2pos(
            seqme_is_pareto_front(-z, assume_unique_lexsorted=True)
        ),
        "fast_pareto": lambda z: bool2pos(
            fast_pareto_is_pf(-z, assume_unique_lexsorted=False)
        ),
        "optuna": lambda z: bool2pos(
            optuna_is_pf(-z, assume_unique_lexsorted=False)
        ),
        ## paretobench only supports deduplication.
        # "paretobench": lambda z: bool2pos(paretobench.Population(f=-z).get_nondominated_indices()),
    }

    bench = Bench(
        name=name,
        n=n,
        setup=setup,
        bench=benchmarks,
        check=check_float_vector,
        max_time=10,
    )

    bench(lambda n: x[:n, :])
    gc.collect()
    bench.plots(file_prefix=file_prefix, title=title, log="xy")

# To not run interactively, use python3 -m bench_ndom (without .py)
if "__file__" not in globals():  # Running interactively.
    plt.show()
