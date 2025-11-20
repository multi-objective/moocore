"""Hypervolume Computation Benchmarks
=======================================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

from bench import Bench, read_data, check_float_values

import numpy as np
import moocore
import pathlib
import matplotlib.pyplot as plt

from pymoo.indicators.igd_plus import IGDPlus as pymoo_IGDplus
## FIXME: Currently DESDEO is a thousand times slower than moocore, so it is not worth running it.
# from desdeo.tools.indicators_unary import igd_plus_indicator as desdeo_igd_plus


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
            # FIXME: Currently DESDEO is a thousand times slower than moocore, so it is not worth running it.
            # "desdeo": lambda z, ref=ref: desdeo_igd_plus(z, reference_set=ref).igd_plus,
        },
        check=check_float_values,
    )

    for maxrow in n:
        values = bench(maxrow, x[:maxrow, :])

    bench.plots(file_prefix=file_prefix, title=title)

if "__file__" not in globals():  # Running interactively.
    plt.show()
