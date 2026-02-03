import pathlib
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import FixedLocator, FixedFormatter
import moocore
import timeit
import cpuinfo
import shutil
import subprocess
import importlib

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


def get_geomrange(lenx, start, stop, num):
    return np.geomspace(start, min(stop, lenx), num=num, dtype=int)


def get_package_version(package):
    package = package.split(maxsplit=1)[0]
    match package:
        case "jMetalPy":
            package = "jmetal"
        case "DEAP_er":
            package = "deap_er"

    module = importlib.import_module(package)
    if hasattr(module, "__version__"):
        return getattr(module, "__version__")
    # It does not provide __version__ !
    return importlib.metadata.version(package)


def check_float_values(a, b, what, n, name):
    assert np.isclose(a, b), (
        f"In {name}, maxrow={n}, {what}={b}  not equal to moocore={a}"
    )


def check_float_vector(a, b, what, n, name):
    np.testing.assert_allclose(
        a,
        b,
        err_msg=f"In {name}, maxrow={n}, {what}={b}  not equal to moocore={a}",
    )


class Bench:
    cpu_model = cpuinfo.get_cpu_info()["brand_raw"]

    def __init__(
        self,
        name,
        n,
        bench,
        setup=None,
        check=None,
        report_values=None,
        return_all_values=False,
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
        self.setup = setup
        self.check = check

    def keys(self):
        return self.bench.keys()

    def bench1(self, what, n, *args, **kwargs):
        if self.setup:
            if isinstance(self.setup, dict):
                setup = self.setup.get(what)
                if setup:
                    args = (setup(*args, **kwargs),)
                    kwargs = {}

        fun = self.bench[what]
        duration, value = timeit.Timer(lambda: fun(*args, **kwargs)).timeit(
            number=3
        )
        self.times[what] += [duration]
        if self.values is not None:
            self.values[what] += [value]
        print(f"{self.name}:{n}:{what}:{duration}")
        return value

    def __call__(self, n, *args, **kwargs):
        # FIXME: Ideally, bench() would call fun for each value in self.n
        assert n in self.n
        values = {
            what: self.bench1(what, n, *args, **kwargs) for what in self.keys()
        }
        if self.check:
            a = values["moocore"]
            for what in self.keys():
                if what == "moocore":
                    continue
                b = values[what]
                self.check(a, b, what=what, n=n, name=self.name)

        return values

    def plots(self, title, file_prefix, log="y", relative=False):
        for what in self.keys():
            self.times[what] = np.asarray(self.times[what])

        logx = "x" in log
        logy = "y" in log
        df = (
            pd.DataFrame(dict(n=self.n, **self.times))
            .set_index("n")
            .rename(columns=self.versions)
        )
        ax = df.plot(
            grid=True,
            logx=logx,
            logy=logy,
            style="o-",
            title="",
            xticks=df.index,
            ylabel="CPU time (seconds)",
        )
        if logx:
            ax.xaxis.set_major_locator(FixedLocator(df.index))
            ax.xaxis.set_major_formatter(FixedFormatter(df.index))
        plt.title(f"({self.cpu_model})", fontsize=10)
        plt.suptitle(f"{title} for {self.name}", fontsize=12)
        plt.savefig(f"{file_prefix}_bench-{self.name}-time.png")

        if relative and "moocore" in self.keys():
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
            ax = df.plot(
                grid=True,
                logx=logx,
                logy=False,  # Looks bad with logy
                style="o-",
                title="",
                xticks=df.index,
                ylabel="Time relative to moocore",
            )
            if logx:
                ax.xaxis.set_major_locator(FixedLocator(df.index))
                ax.xaxis.set_major_formatter(FixedFormatter(df.index))
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
            logx=logx,
            logy=logy,
            style="o-",
            title="",
            xticks=df.index,
            ylabel=self.value_label,
        )
        plt.title(f"({self.cpu_model})", fontsize=10)
        plt.suptitle(f"{title} for {self.name}", fontsize=12)
        png_file = f"{file_prefix}_bench-{self.name}-values.png"
        plt.savefig(png_file)
        # Optimize with optipng if available.
        optipng = shutil.which("optipng")
        if optipng:
            subprocess.run([optipng, "-quiet", png_file])
