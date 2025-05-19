"""Hypervolume Computation Benchmarks
=======================================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

import numpy as np
import moocore
import pandas as pd
import matplotlib.pyplot as plt

from botorch.utils.multi_objective.hypervolume import Hypervolume as botorch_HV
import torch

from pymoo.indicators.hv import Hypervolume as pymoo_HV
from jmetal.core.quality_indicator import HyperVolume as jmetal_HV
from deap_er.utilities.hypervolume import HyperVolume as deaper_HV

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
path_to_data = "~/work/perfassess/moocore/testsuite/data/"
files = {
    "DTLZLinearShape.3d": path_to_data + "DTLZLinearShape.3d.front.1000pts.10",
    "DTLZLinearShape.4d": path_to_data + "DTLZLinearShape.4d.front.1000pts.10",
}


def read_data(filename):
    x = moocore.read_datasets(filename)[:, :-1]
    x = moocore.filter_dominated(x)
    return x


bench = {}
names = files.keys()
for name in names:
    x = read_data(files[name])
    ref = np.ones(x.shape[1])
    n = np.arange(500, min(len(x), 3000) + 1, 500)

    bench["moocore"] = moocore.Hypervolume(ref=ref)
    bench["pymoo"] = lambda z, hv=pymoo_HV(ref_point=ref): hv(z)
    bench["jMetalPy"] = lambda z, hv=jmetal_HV(ref): hv.compute(z)
    bench["botorch"] = lambda z, hv=botorch_HV(
        ref_point=torch.from_numpy(-ref)
    ): hv.compute(z)
    bench["DEAP_er"] = lambda z, fun=deaper_HV(ref_point=ref).compute: fun(
        np.copy(z)
    )

    values = {}
    times = {k: [] for k in bench.keys()}

    for maxrow in n:
        z = x[:maxrow, :]
        for what in bench.keys():
            fun = bench[what]
            if what == "botorch":
                z_torch = torch.from_numpy(-z)
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
        title=f"HV computation for {name} ({cpu})",
        logy=True,
    )
    plt.savefig(f"hv_bench-{name}-time.png")

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
        title=f"HV computation for {name} ({cpu})",
    )
    plt.savefig(f"hv_bench-{name}-reltime.png")

plt.show()
