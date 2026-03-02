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


def _normalize(result):
    if isinstance(result, tuple):
        # (args, kwargs)
        if len(result) == 2 and isinstance(result[1], dict):
            return result[0], result[1]
        return result, {}
    elif isinstance(result, dict):
        return (), result
    return (result,), {}


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
        max_time=0,
        baseline="moocore",
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
        self.max_time = max_time
        self.baseline = baseline

    def keys(self):
        return self.bench.keys()

    def bench1(self, what, _n, *args, **kwargs):
        if self.setup and isinstance(self.setup, dict):
            setup = self.setup.get(what)
            if setup:
                args, kwargs = _normalize(setup(*args, **kwargs))

        fun = self.bench[what]
        duration, value = timeit.Timer(lambda: fun(*args, **kwargs)).timeit(
            number=3
        )
        self.times[what] += [duration]
        if self.values is not None:
            self.values[what] += [value]
        print(f"{self.name}:{_n}:{what}:{duration}")
        return value

    def bench_testcase(self, _n, *args, _algos=None, **kwargs):
        assert _n in self.n

        if _algos is None:
            _algos = self.keys()

        if self.setup and not isinstance(self.setup, dict):
            args, kwargs = _normalize(self.setup(*args, **kwargs))

        values = {
            what: self.bench1(what, _n, *args, **kwargs) for what in _algos
        }
        if self.check:
            a = values[self.baseline]
            for what in values.keys():
                if what == self.baseline:
                    continue
                b = values[what]
                self.check(a, b, what=what, n=_n, name=self.name)

        return values

    def __call__(self, get_testcase):
        # Call fun for each value in self.n
        algos = self.keys()
        for _n in self.n:
            args, kwargs = _normalize(get_testcase(n=_n))
            self.bench_testcase(_n, *args, **kwargs, _algos=algos)
            # Remove anything that has gone over-time.
            # FIXME: Ideally we will stop timeit.Timer() when it goes over-time.
            if self.max_time > 0:
                algos = [
                    what
                    for what in algos
                    if what == self.baseline
                    or self.times[what][-1] <= self.max_time
                ]

    def plots(self, title, file_prefix, log="y", relative=False):

        # Pad with nan so we don't have problems later when converting to DataFrame.
        max_len = np.max([len(v) for v in self.times.values()])
        for k, v in self.times.items():
            self.times[k] = np.pad(
                np.array(v, dtype=float),
                (0, max_len - len(v)),
                "constant",
                constant_values=np.nan,
            )

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

        if relative and self.baseline in self.keys():
            df
            reltimes = {}
            for what in self.keys():
                if what == self.baseline:
                    continue
                reltimes["Rel_" + what] = (
                    self.times[what] / self.times[self.baseline]
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
