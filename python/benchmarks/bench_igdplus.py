"""Hypervolume Computation Benchmarks
=======================================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

from bench import Bench, check_float_values, get_range

import numpy as np
import moocore
import matplotlib.pyplot as plt

from desdeo.tools.indicators_unary import igd_plus_indicator as desdeo_igd_plus
from jmetal.core.quality_indicator import (
    InvertedGenerationalDistancePlus as jmetal_IGDplus,
)

files = {
    "ran.1000pts.3d": "ran.1000pts.3d.10",
}

title = "IGD+ computation"
file_prefix = "igd_plus"

print(f"Running benchmark: {title}")
names = files.keys()
for name in names:
    ref = moocore.get_dataset(files[name])
    n = get_range(len(ref), 10, 100, 10)

    x = ref.copy()
    rng = np.random.default_rng(42)
    rng.shuffle(x, axis=0)

    bench = Bench(
        name=name,
        n=n,
        bench={
            "moocore": lambda z, ref=ref: moocore.igd_plus(z, ref=ref),
            "jMetalPy": lambda z, igdp=jmetal_IGDplus(ref): igdp.compute(z),
            "desdeo": lambda z, ref=ref: (
                desdeo_igd_plus(z, reference_set=ref).igd_plus
            ),
        },
        check=check_float_values,
    )

    bench(lambda n: x[:n, :])
    bench.plots(file_prefix=file_prefix, title=title)

if "__file__" not in globals():  # Running interactively.
    plt.show()
