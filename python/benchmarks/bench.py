import pathlib
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
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


def check_array_equal(a, b, what, n, name):
    np.testing.assert_equal(
        a,
        b,
        err_msg=f"In {name}, maxrow={n}, {what}={b}  not equal to moocore={a}",
    )


def check_float_vector(a, b, what, n, name):
    np.testing.assert_allclose(
        a,
        b,
        err_msg=f"In {name}, maxrow={n}, {what}={b}  not equal to moocore={a}",
    )


def save2png(filename):
    plt.savefig(filename)
    # Optimize with optipng if available.
    optipng = shutil.which("optipng")
    if optipng:
        subprocess.run([optipng, "-quiet", filename])


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
        name: str,
        n,
        bench: dict,
        setup: dict | None = None,
        check=None,
        report_values=None,
        return_all_values=False,
        max_time: float = 0,
        reps: int = 3,
        baseline: str = "moocore",
    ):
        """

        name:
            Name of this benchmark.
        n:
            List of numerical values.
        bench:
            A dictionary of functions or lambdas. Each element will be called for each value of n
        setup:
            Function or dictionary of functions to be called for each value of n.
        reps:
            Number of repetitions of the bench function (minimum time is kept).
            This should be set to 1 when calls are not independent.
        """
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
        self.reps = reps
        self.baseline = baseline

    def keys(self):
        return self.bench.keys()

    def bench1(self, what, _n, *args, quiet=False, **kwargs):
        # If setup is a dictionary, it is called once for each algorithm and
        # for each value of n.
        if self.setup and isinstance(self.setup, dict):
            setup = self.setup.get(what)
            if setup:
                args, kwargs = _normalize(setup(*args, **kwargs))

        fun = self.bench[what]
        duration, value = timeit.Timer(lambda: fun(*args, **kwargs)).timeit(
            number=self.reps
        )
        self.times[what] += [duration]
        if self.values is not None:
            self.values[what] += [value]
        if not quiet:
            print(f"{self.name}:{_n}:{what}:{duration}")
        return value

    def bench_testcase(self, _n, *args, _algos=None, **kwargs):
        assert _n in self.n
        if _algos is None:
            _algos = self.keys()

        # If setup is not a dictionary, it is called once before each value of n.
        if self.setup and not isinstance(self.setup, dict):
            args, kwargs = _normalize(self.setup(*args, **kwargs))

        quiet = len(self.n) > 100 and (_n & (_n - 1) != 0)
        values = {
            what: self.bench1(what, _n, *args, quiet=quiet, **kwargs)
            for what in _algos
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
        """Run the benchmark for one testcase, that is, call get_testcase(n=_n) for _n in self.n"""
        algos = self.keys()

        # Remember where this particular call starts, since self.times and
        # self.values may already contain results from previous calls.
        start_len = {what: len(self.times[what]) for what in self.keys()}

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

        # Complete this call's result arrays with NaN so that every algorithm
        # contributed exactly len(self.n) entries for this invocation.
        for what in self.keys():
            n_results = len(self.times[what]) - start_len[what]
            n_missing = len(self.n) - n_results
            if n_missing > 0:
                self.times[what].extend([np.nan] * n_missing)
                if self.values is not None:
                    self.values[what].extend([np.nan] * n_missing)

    def plots(
        self,
        title,
        file_prefix,
        log="y",
        logx_base=10,
        relative=False,
        xlabel="n",
        show_ci=False,
    ):
        """Plots the results after running the benchmark one or more times.

        __call__() is expected to pad each algorithm's results with np.nan so
        that every call contributes exactly len(self.n) entries.
        """
        logx = "x" in log
        logy = "y" in log

        def single_plot(results, ylabel, file_suffix):
            """Summarise and plot a set of benchmark results."""
            lengths = {what: len(values) for what, values in results.items()}
            if len(set(lengths.values())) != 1:
                raise ValueError(
                    f"In {self.name}, result lengths are inconsistent: {lengths}"
                )

            # Get the first value of lengths.
            nresults = next(iter(lengths.values()))
            if nresults % len(self.n) != 0:
                raise ValueError(
                    f"In {self.name}, number of results ({nresults}) is not "
                    f"a multiple of len(n) ({len(self.n)})."
                )

            # Each consecutive block of len(self.n) values is one call to
            # __call__.  Repeating self.n gives the corresponding n for every
            # observation.
            results = {
                what: np.asarray(values, dtype=float)
                for what, values in results.items()
            }
            df = pd.DataFrame(
                {
                    "n": np.tile(self.n, nresults // len(self.n)),
                    **results,
                }
            )

            if show_ci:
                ci = 1.96  # 95% CI
                if logy:
                    # Geometric mean + 95% CI, calculated in log space.
                    log_raw = (
                        df.drop(columns="n").apply(np.log).groupby(df["n"])
                    )
                    df = log_raw.mean()
                    sem = log_raw.sem()
                    lower = np.exp(df - ci * sem)
                    upper = np.exp(df + ci * sem)
                    df = np.exp(df)
                else:
                    # Arithmetic mean and ordinary CI
                    grouped = df.groupby("n")
                    df = grouped.mean(numeric_only=True)
                    sem = grouped.sem(numeric_only=True)
                    lower = df - ci * sem
                    upper = df + ci * sem

                lower = lower.rename(columns=self.versions)
                upper = upper.rename(columns=self.versions)
            else:
                df = df.groupby("n").mean(numeric_only=True)
                lower = upper = None

            df = df.rename(columns=self.versions)
            ax = df.plot(
                grid=True,
                logx=logx,
                logy=logy,
                style="o-",
                title="",
                ylabel=ylabel,
                xlabel=xlabel,
            )

            if logx:
                ax.set_xscale("log", base=logx_base)
                # Set only the benchmark sizes as major ticks.
                ax.xaxis.set_major_locator(mticker.FixedLocator(df.index))
                if logx_base == 2:
                    ax.xaxis.set_major_formatter(
                        lambda x, pos: rf"$2^{{{int(np.log2(x))}}}$"
                    )
                else:
                    ax.xaxis.set_major_formatter(
                        mticker.FixedFormatter([f"{x:g}" for x in df.index])
                    )

                # Hide any scientific-notation offset text.
                ax.xaxis.get_offset_text().set_visible(False)

            if lower is not None:
                x = df.index.to_numpy()
                for col in df.columns:
                    y1 = lower[col].to_numpy()
                    y2 = upper[col].to_numpy()
                    # Avoid problems with NaN CI bounds, which can occur when
                    # there are fewer than two valid observations.
                    valid = np.isfinite(y1) & np.isfinite(y2)
                    if np.any(valid):
                        ax.fill_between(x, y1, y2, where=valid, alpha=0.2)

            plt.title(f"({self.cpu_model})", fontsize=10)
            plt.suptitle(f"{title} for {self.name}", fontsize=12)
            save2png(f"{file_prefix}_bench-{self.name}-{file_suffix}.png")

        # CPU times.
        single_plot(self.times, ylabel="CPU time (seconds)", file_suffix="time")

        # Relative CPU times.
        if relative and self.baseline in self.keys():
            baseline = np.asarray(self.times[self.baseline], dtype=float)
            relative_results = {}
            for what in self.keys():
                if what == self.baseline:
                    continue
                values = np.asarray(self.times[what], dtype=float)
                if len(values) != len(baseline):
                    raise ValueError(
                        f"In {self.name}, number of results for {what!r} "
                        f"({len(values)}) does not match baseline ({len(baseline)})."
                    )

                with np.errstate(divide="ignore", invalid="ignore"):
                    relative_results[f"Rel_{what}"] = values / baseline

            single_plot(
                relative_results,
                ylabel=f"Time relative to {self.baseline}",
                file_suffix="reltime",
            )

        # Reported values.
        if self.values is not None:
            single_plot(
                self.values, ylabel=self.value_label, file_suffix="values"
            )
