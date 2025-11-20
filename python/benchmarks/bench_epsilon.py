"""Hypervolume Computation Benchmarks
=======================================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

from bench import Bench, read_data, check_float_values

import numpy as np
import moocore
import pathlib
import matplotlib.pyplot as plt

from jmetal.core.quality_indicator import EpsilonIndicator as jmetal_EPS

path_to_data = "../../testsuite/data/"
if not pathlib.Path(path_to_data).expanduser().exists():
    path_to_data = (
        "https://github.com/multi-objective/testsuite/raw/refs/heads/main/data/"
    )

files = {
    "rmnk_10D_random_search": (
        path_to_data + "rmnk_0.0_10_16_1_0_random_search_1.txt.xz",
        path_to_data + "rmnk_0.0_10_16_1_0_ref.txt.xz",
    ),
}

title = "eps+ computation"
file_prefix = "eps"

names = files.keys()
for name in names:
    x = read_data(files[name][0])
    ref = read_data(files[name][1])
    n = np.arange(200, min(len(x), 1000) + 1, 200)

    bench = Bench(
        name=name,
        n=n,
        bench={
            "moocore": lambda z, ref=ref: moocore.epsilon_additive(z, ref=ref),
            "jMetalPy": lambda z, eps=jmetal_EPS(ref): eps.compute(z),
        },
        check=check_float_values,
    )

    for maxrow in n:
        values = bench(maxrow, x[:maxrow, :])

    bench.plots(file_prefix=file_prefix, title=title)

if "__file__" not in globals():  # Running interactively.
    plt.show()
