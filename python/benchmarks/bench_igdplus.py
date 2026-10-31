"""Hypervolume Computation Benchmarks
=======================================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

import matplotlib.pyplot as plt
import numpy as np
from bench import (
    Bench,
    check_float_values,
    get_geomrange,
    read_datasets_and_filter_dominated,
)
from desdeo.tools.indicators_unary import igd_plus_indicator as desdeo_igd_plus
from jmetal.core.quality_indicator import (
    InvertedGenerationalDistancePlus as jmetal_IGDplus,
)

import moocore

files = {
    "ran.1000pts.3d": "ran.1000pts.3d.10",
}

title = "IGD+ computation"
file_prefix = "igd_plus"

print(f"Running benchmark: {title}")
names = files.keys()
for name in names:
    ref = read_datasets_and_filter_dominated(
        moocore.get_dataset_path(files[name])
    )
    n = get_geomrange(len(ref), 10, 500, 10)

    x = ref.copy()
    rng = np.random.default_rng(42)
    rng.shuffle(x, axis=0)

    bench = Bench(
        name=name,
        n=n,
        bench={
            "moocore": lambda z: moocore.igd_plus(z, ref=ref),
            "jMetalPy": jmetal_IGDplus(ref).compute,
            "desdeo": lambda z: desdeo_igd_plus(z, reference_set=ref).igd_plus,
        },
        check=check_float_values,
    )

    bench(lambda n: x[:n, :])
    bench.plots(file_prefix=file_prefix, log="xy", title=title)

if "__file__" not in globals():  # Running interactively.
    plt.show()
