import pathlib
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import moocore
import timeit
import cpuinfo

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


def read_data(filename):
    if not filename.startswith("http"):
        filename = pathlib.Path(filename).expanduser()
    x = np.loadtxt(filename)
    x = moocore.filter_dominated(x)
    return x


def read_datasets_and_filter_dominated(filename):
    filename = pathlib.Path(filename).expanduser()
    x = moocore.read_datasets(filename)[:, :-1]
    x = moocore.filter_dominated(x)
    return x


def get_range(lenx, start, stop, step):
    return np.arange(start, min(stop, lenx) + 1, step)


def get_package_version(package):
    match package:
        case "moocore":
            from moocore import __version__ as version
        case "botorch":
            from botorch import __version__ as version
        case "pymoo":
            from pymoo import __version__ as version
        case "jMetalPy":
            from jmetal import __version__ as version
        case "DEAP_er":
            # Requires version >= 0.2.0
            from deap_er import __version__ as version
        case _:
            raise ValueError(f"unknown package {package}")

    return version


class Bench:
    cpu_model = cpuinfo.get_cpu_info()["brand_raw"]

    def __init__(self, name, n, bench):
        self.name = name
        self.n = n
        self.bench = bench
        self.times = {k: [] for k in bench.keys()}
        self.versions = {
            what: f"{what} ({get_package_version(what)})"
            for what in bench.keys()
        }

    def keys(self):
        return self.bench.keys()

    def __call__(self, what, maxrow, *args):
        fun = self.bench[what]
        duration, value = timeit.Timer(lambda: fun(*args)).timeit(number=3)
        self.times[what] += [duration]
        print(f"{self.name}:{maxrow}:{what}:{duration}")
        return value

    def plots(self, title, file_prefix):
        for what in self.keys():
            self.times[what] = np.asarray(self.times[what])

        df = (
            pd.DataFrame(dict(n=self.n, **self.times))
            .set_index("n")
            .rename(columns=self.versions)
        )
        df.plot(
            grid=True,
            ylabel="CPU time (seconds)",
            style="o-",
            title="",
            logy=True,
        )
        plt.title(f"({self.cpu_model})", fontsize=10)
        plt.suptitle(f"{title} for {self.name}", fontsize=12)
        plt.savefig(f"{file_prefix}_bench-{self.name}-time.png")

        reltimes = {}
        for what in self.keys():
            if what == "moocore":
                continue
            reltimes["Rel_" + what] = self.times[what] / self.times["moocore"]

        df = (
            pd.DataFrame(dict(n=self.n, **reltimes))
            .set_index("n")
            .rename(columns=self.versions)
        )
        df.plot(
            grid=True,
            ylabel="Time relative to moocore",
            style="o-",
            title="",
        )
        plt.title(f"({self.cpu_model})", fontsize=10)
        plt.suptitle(f"{title} for {self.name}", fontsize=12)
        plt.savefig(f"{file_prefix}_bench-{self.name}-reltime.png")
