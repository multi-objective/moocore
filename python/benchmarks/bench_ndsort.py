"""Dominance Filtering Benchmarks
==============================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

from bench import Bench, get_geomrange, check_float_vector

import numpy as np
import moocore
import matplotlib.pyplot as plt

from pymoo.util.nds.non_dominated_sorting import (
    NonDominatedSorting as pymoo_NDS,
)
from desdeo.tools.non_dominated_sorting import (
    fast_non_dominated_sort as desdeo_nds,
)

from paretoset import paretorank as paretoset_paretorank
## It cannot be installed: https://github.com/esa/pygmo2/issues/152
# from pygmo import pareto_dominance as pg_pareto_dominance

# See https://github.com/multi-objective/testsuite/tree/main/data
files = {
    # range are the (start, stop, num) parameters for np.geomspace()
    "ran-2d": dict(generate=(50_000, 2), range=(100, 50_000, 10)),
    "ran-3d": dict(generate=(40_000, 3), range=(100, 40_000, 10)),
    "ran-4d": dict(generate=(30_000, 4), range=(100, 30000, 10)),
    "ran-5d": dict(generate=(20_000, 5), range=(100, 20000, 10)),
}


rng = np.random.default_rng(42)


def get_dataset(name):
    n, d = files[name]["generate"]
    x = rng.random(size=(n, d))
    while len(np.unique(x, axis=0)) < len(x):
        x = rng.random(size=(n, d))
    return x


title = "pareto_rank()"
file_prefix = "ndsort"

print(f"Running benchmark: {title}")
names = files.keys()
for name in names:
    x = get_dataset(name)
    n = get_geomrange(len(x), *files[name]["range"])

    bench = Bench(
        name=name,
        n=n,
        bench={
            "moocore": lambda z: moocore.pareto_rank(z),
            "paretoset": lambda z: paretoset_paretorank(z, use_numba=True) - 1,
            "pymoo": lambda z, nds=pymoo_NDS(): nds.do(z, return_rank=True)[1],
            "desdeo": lambda z: desdeo_nds(z).argmax(axis=0),
        },
        check=check_float_vector,
    )

    for maxrow in n:
        values = bench(maxrow, x[:maxrow, :])

    bench.plots(file_prefix=file_prefix, title=title, log="xy")

if "__file__" not in globals():  # Running interactively.
    plt.show()
