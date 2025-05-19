"""Hypervolume Computation Benchmarks
=======================================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

import numpy as np
import moocore
import pandas as pd
import pathlib
import matplotlib.pyplot as plt

from pymoo.indicators.igd_plus import IGDPlus as pymoo_IGDplus

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
    "ran.40000pts.3d": (
        path_to_data + "ran.40000pts.3d.1.xz",
        path_to_data + "ran.40001pts.3d.1.xz",
    ),
}

title = "IGD+ computation"
file_prefix = "igd_plus"
bench = {}
names = files.keys()

for name in names:
    x = moocore.filter_dominated(
        np.loadtxt(pathlib.Path(files[name][0]).expanduser())
    )
    ref = moocore.filter_dominated(
        np.loadtxt(pathlib.Path(files[name][1]).expanduser())
    )
    n = np.arange(100, min(len(x), 1300) + 1, 200)

    bench["moocore"] = lambda z, ref=ref: moocore.igd_plus(z, ref=ref)
    bench["pymoo"] = lambda z, ind=pymoo_IGDplus(ref): ind(z)
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
            print(f"{name}:{maxrow}:{what}:{duration}")

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
        title=f"{title} for {name} ({cpu})",
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
