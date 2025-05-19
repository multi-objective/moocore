"""Hypervolume Computation Benchmarks
=======================================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

import numpy as np
import moocore
import pandas as pd
import pathlib
import matplotlib.pyplot as plt

from jmetal.core.quality_indicator import EpsilonIndicator as jmetal_EPS

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

# FIXME: How to get this info automatically?
cpu = "Intel i5-6200U 2.30GHz"
# See https://github.com/multi-objective/testsuite/tree/main/data
path_to_data = "~/work/perfassess/moocore/testsuite/data/"
files = {
    "rmnk_10D_random_search": (
        path_to_data + "rmnk_0.0_10_16_1_0_random_search_1.txt.xz",
        path_to_data + "rmnk_0.0_10_16_1_0_ref.txt.xz",
    ),
}


def read_data(filename):
    x = np.loadtxt(pathlib.Path(filename).expanduser())
    x = moocore.filter_dominated(x)
    return x


title = "eps+ computation"
file_prefix = "eps"

bench = {}
names = files.keys()
for name in files.keys():
    x = read_data(files[name][0])
    ref = read_data(files[name][1])
    n = np.arange(200, min(len(x), 1000) + 1, 200)

    bench["moocore"] = lambda z, ref=ref: moocore.epsilon_additive(z, ref=ref)
    bench["jMetalPy"] = lambda z, eps=jmetal_EPS(ref): eps.compute(z)

    values = {}
    times = {k: [] for k in bench.keys()}

    for maxrow in n:
        z = x[:maxrow, :]
        for what in bench.keys():
            fun = bench[what]
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
            assert np.isclose(a, b), (
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
        title=f"{title} computation for {name} ({cpu})",
        logy=True,
    )
    plt.savefig(f"{file_prefix}_bench-{name}-time.png")

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
        title=f"{title} computation for {name} ({cpu})",
    )
    plt.savefig(f"{file_prefix}_bench-{name}-reltime.png")

plt.show()
