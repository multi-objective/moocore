import pathlib
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import moocore
import timeit
import cpuinfo

timeit_template_return_1_value = """
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

timeit_template_return_all_values = """
def inner(_it, _timer{init}):
    {setup}
    _dt = float('inf')
    retval = []
    for _i in _it:
        _t0 = _timer()
        retval.append({stmt})
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
    package = package.split(maxsplit=1)[0]
    match package:
        case "moocore":
            from moocore import __version__ as version
        case "botorch":
            from botorch import __version__ as version
        case "pymoo":
            from pymoo import __version__ as version
        case "jMetalPy":
            from jmetal import __version__ as version
        case "trieste":
            from trieste import __version__ as version
        case "DEAP_er":
            # Requires version >= 0.2.0
            from deap_er import __version__ as version
        case _:
            raise ValueError(f"unknown package {package}")

    return version


class Bench:
    cpu_model = cpuinfo.get_cpu_info()["brand_raw"]

    def __init__(
        self, name, n, bench, report_values=None, return_all_values=False
    ):
        self.name = name
        self.n = n
        self.bench = bench
        self.times = {k: [] for k in bench.keys()}
        self.versions = {
            what: f"{what} ({get_package_version(what)})"
            for what in bench.keys()
        }
        timeit.template = timeit_template_return_1_value
        if report_values:
            self.values = {k: [] for k in bench.keys()}
            self.value_label = report_values
            if return_all_values:
                timeit.template = timeit_template_return_all_values
        else:
            self.values = None
            self.value_label = None

    def keys(self):
        return self.bench.keys()

    def __call__(self, what, n, *args, **kwargs):
        # FIXME: Ideally, bench() would call fun for each value in self.n
        assert n in self.n
        fun = self.bench[what]
        duration, value = timeit.Timer(lambda: fun(*args, **kwargs)).timeit(
            number=3
        )
        self.times[what] += [duration]
        if self.values is not None:
            self.values[what] += [value]
        print(f"{self.name}:{n}:{what}:{duration}")
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
            logy=True,
            style="o-",
            title="",
            xticks=df.index,
            ylabel="CPU time (seconds)",
        )
        plt.title(f"({self.cpu_model})", fontsize=10)
        plt.suptitle(f"{title} for {self.name}", fontsize=12)
        plt.savefig(f"{file_prefix}_bench-{self.name}-time.png")

        if "moocore" in self.keys():
            reltimes = {}
            for what in self.keys():
                if what == "moocore":
                    continue
                reltimes["Rel_" + what] = (
                    self.times[what] / self.times["moocore"]
                )

            df = (
                pd.DataFrame(dict(n=self.n, **reltimes))
                .set_index("n")
                .rename(columns=self.versions)
            )
            df.plot(
                grid=True,
                style="o-",
                title="",
                xticks=df.index,
                ylabel="Time relative to moocore",
            )
            plt.title(f"({self.cpu_model})", fontsize=10)
            plt.suptitle(f"{title} for {self.name}", fontsize=12)
            plt.savefig(f"{file_prefix}_bench-{self.name}-reltime.png")

        if self.values is None:
            return

        for what in self.keys():
            self.values[what] = np.asarray(self.values[what])

        df = (
            pd.DataFrame(dict(n=self.n, **self.values))
            .set_index("n")
            .rename(columns=self.versions)
        )
        df.plot(
            grid=True,
            logy=True,
            style="o-",
            title="",
            xticks=df.index,
            ylabel=self.value_label,
        )
        plt.title(f"({self.cpu_model})", fontsize=10)
        plt.suptitle(f"{title} for {self.name}", fontsize=12)
        plt.savefig(f"{file_prefix}_bench-{self.name}-values.png")
