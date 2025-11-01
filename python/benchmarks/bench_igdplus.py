"""Hypervolume Computation Benchmarks
=======================================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

from bench import Bench, read_data

import numpy as np
import moocore
import pathlib
import matplotlib.pyplot as plt

from pymoo.indicators.igd_plus import IGDPlus as pymoo_IGDplus

path_to_data = "../../testsuite/data/"
if not pathlib.Path(path_to_data).expanduser().exists():
    path_to_data = (
        "https://github.com/multi-objective/testsuite/raw/refs/heads/main/data/"
    )

files = {
    "ran.40000pts.3d": (
        path_to_data + "ran.40000pts.3d.1.xz",
        path_to_data + "ran.40001pts.3d.1.xz",
    ),
}

title = "IGD+ computation"
file_prefix = "igd_plus"
names = files.keys()
for name in names:
    x = read_data(files[name][0])
    ref = read_data(files[name][1])
    n = np.arange(100, min(len(x), 1300) + 1, 200)

    bench = Bench(
        name=name,
        n=n,
        bench={
            "moocore": lambda z, ref=ref: moocore.igd_plus(z, ref=ref),
            "pymoo": lambda z, ind=pymoo_IGDplus(ref): ind(z),
        },
    )

    values = {}
    for maxrow in n:
        z = x[:maxrow, :]
        for what in bench.keys():
            values[what] = bench(what, maxrow, z)

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

if "__file__" not in globals():  # Running interactively.
    plt.show()
