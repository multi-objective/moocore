"""Dominance Filtering Benchmarks
==============================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

import numpy as np
import moocore
import pandas as pd
import matplotlib.pyplot as plt


import torch
from botorch.utils.multi_objective.pareto import (
    is_non_dominated as botorch_is_non_dominated,
)
from pymoo.util.nds.non_dominated_sorting import (
    find_non_dominated as pymoo_find_non_dominated,
)

import timeit

timeit.template = """
def inner(_it, _timer{init}):
    {setup}
    _dt = float('inf')
    for _i in _it:
        _t0 = _timer()
        retval = {stmt}
        _t1 = _timer()
        _dt = min(_dt, _t1 - _t0)
    return _dt, retval
"""

# FIXME: How to get CPU details automatically?
cpu = "Intel i5-6200U 2.30GHz"
# See https://github.com/multi-objective/testsuite/tree/main/data
files = {
    "test2D-200k": "test2D-200k.inp.xz",
    "ran3d-10k": "ran.1000pts.3d.10",
}
ranges = {
    "test2D-200k": (1000, 10000, 1000),
    "ran3d-10k": (1000, 10000, 1000),
}


def get_n(lenx, start, stop, step):
    return np.arange(start, min(stop, len(x)) + 1, step)


bench = {}
names = files.keys()
for name in names:
    x = moocore.get_dataset(files[name])[:, :-1]
    n = get_n(len(x), *ranges[name])

    bench["botorch"] = lambda z: np.nonzero(
        np.asarray(botorch_is_non_dominated(z))
    )[0]
    bench["pymoo"] = lambda z: pymoo_find_non_dominated(-z)
    bench["moocore"] = lambda z: np.nonzero(
        moocore.is_nondominated(z, maximise=True)
    )[0]

    times = {k: [] for k in bench.keys()}
    values = {}

    for maxrow in n:
        z = x[:maxrow, :]
        for what in bench.keys():
            fun = bench[what]
            if what == "botorch":
                # Exclude the conversion to torch from the timing.
                z_torch = torch.from_numpy(z)
                test = "fun(z_torch)"
            else:
                test = "fun(z)"
            duration, value = timeit.timeit(test, globals=globals(), number=3)
            times[what] += [duration]
            values[what] = value
        # Check values
        for what in bench.keys():
            if what == "moocore":
                continue
            a = values["moocore"]
            b = values[what]
            assert np.allclose(a, b), (
                f"In {name}, maxrow={maxrow}, {what}={b}  not equal to moocore={a}"
            )

    del values

    for what in bench.keys():
        times[what] = np.array(times[what])

    df = pd.DataFrame(dict(n=n, **times)).set_index("n")
    df.plot(
        grid=True,
        ylabel="CPU time (seconds)",
        style="o-",
        title=f"is_non_dominated() for {name} ({cpu})",
        logy=True,
    )
    plt.savefig(f"ndom_bench-{name}-time.png")

    reltimes = {}
    for what in bench.keys():
        if what == "moocore":
            continue
        reltimes["Rel_" + what] = times[what] / times["moocore"]

    df = pd.DataFrame(dict(n=n, **reltimes)).set_index("n")
    df.plot(
        grid=True,
        ylabel="Time relative to moocore",
        style="o-",
        title=f"is_non_dominated() for {name} ({cpu})",
    )
    plt.savefig(f"ndom_bench-{name}-reltime.png")

plt.show()
