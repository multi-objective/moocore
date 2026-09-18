import itertools

import matplotlib.pyplot as plt
import numpy as np
from bench import Bench, check_bool_array_equal
from jmetal.core.solution import Solution as jmetal_Solution
from jmetal.util.archive import NonDominatedSolutionsArchive as jmetal_archive
from moarchiving import get_mo_archive as moarch_get_mo_archive

import moocore


# This is needed to test jmetal.
class DummySolution(jmetal_Solution[float]):
    """Concrete implementation of Solution for testing purposes."""

    def __init__(self, x: np.ndarray):
        super().__init__(0, len(x), 0)
        self._variables = []
        self.objectives = x.tolist()

    @property
    def variables(self) -> list[float]:
        return self._variables.copy()

    @variables.setter
    def variables(self, variables: list[float]) -> None:
        self._variables = variables.copy()

    def __copy__(self) -> "DummySolution":
        new_solution = self.__class__(self.number_of_objectives)
        new_solution.variables = self.variables
        new_solution.objectives = self.objectives.copy()
        return new_solution


def np_to_jmetal_solution(matrix):
    return [DummySolution(x) for x in matrix]


def moarchiving_return(return_value: int | bool | None) -> bool:
    """moarchiving returns int|None for 2D but bool for 3D and 4D."""
    if isinstance(return_value, bool):
        return return_value
    return isinstance(return_value, int)


# FIXME: How to test both float and Fractions?
moarch_get_mo_archive.hypervolume_computation_float_type = float
moarch_get_mo_archive.hypervolume_final_float_type = float

datasets = {
    "hypercube-uniform": lambda max_size, n_obj, rng: moocore.generate_sequence(
        max_size, n_obj, method="cube", seed=rng
    ),
    "hypercube-uniform-rep4": lambda max_size, n_obj, rng: (
        moocore.generate_sequence(
            int(max_size / 4), n_obj, method="cube", seed=rng, n_rep=max_size
        )
    ),
    "hypercube-ndsort=1": lambda max_size, n_obj, rng: (
        moocore.generate_sequence(
            max_size, n_obj, method="cube", ndsort=1, seed=rng
        )
    ),
    "hypercube-ndsort=-1": lambda max_size, n_obj, rng: (
        moocore.generate_sequence(
            max_size, n_obj, method="cube", ndsort=-1, seed=rng
        )
    ),
    "within-concave-sphere": lambda max_size, n_obj, rng: (
        moocore.generate_sequence(
            max_size, n_obj, method="sphere", r_min=0.5, seed=rng
        )
    ),
    "inverted-simplex": lambda max_size, n_obj, rng: moocore.generate_ndset(
        max_size, n_obj, method="inverted-simplex", seed=rng
    ),
    "glas2017,c=0.5": lambda max_size, n_obj, rng: moocore.generate_sequence(
        max_size, n_obj, method="glas2017", c_value=0.5, seed=rng
    ),
    "glas2017,c=2.0": lambda max_size, n_obj, rng: moocore.generate_sequence(
        max_size, n_obj, method="glas2017", c_value=2.0, seed=rng
    ),
    "convex-cliff": lambda max_size, n_obj, rng: moocore.generate_ndset(
        max_size, n_obj, method="convex-cliff", seed=rng
    ),
    "concave-cliff": lambda max_size, n_obj, rng: moocore.generate_ndset(
        max_size, n_obj, method="concave-cliff", seed=rng
    ),
}

n_repeats = 10

title = "Adding a point to the archive"
file_prefix = "unbarchive_add"
max_exp = {2: 19, 3: 17, 4: 13}
print(f"# Running benchmark: {title}")
names = datasets.keys()
n_objs = [2, 3, 4]
for n_obj, name in itertools.product(n_objs, names):
    if n_obj == 2 and name.endswith("cliff"):
        continue
    test_archive_sizes = 2 ** np.arange(1, max_exp[n_obj])
    max_size = test_archive_sizes[-1]

    # This is a bit wasteful because we rebuild the archive from scratch for
    # each size.  On the other hand, it is probably less sensitive to noise.
    setup = {
        "moocore (UnboundedArchive)": lambda z: {
            "moa": moocore.UnboundedArchive(dim=n_obj),
            "z": z,
        },
        "moarchiving (float)": lambda z: {
            "moa": moarch_get_mo_archive(n_obj=n_obj),
            "z": z.tolist(),
        },
        "jmetal (NonDominatedArchive)": lambda z: {
            "moa": jmetal_archive(),
            "z": np_to_jmetal_solution(z),
        },
    }

    def archive_filter_dominated(z):
        return [
            bool(moocore.is_nondominated(z[: (i + 1)])[-1])
            for i in range(len(z))
        ]

    benchmarks = {
        "moocore (UnboundedArchive)": lambda moa, z: [moa.add(p) for p in z],
        "moocore (filter_nondominated)": moocore.filter_dominated,
        "moarchiving (float)": lambda moa, z: [
            moarchiving_return(moa.add(p)) for p in z
        ],
        "jmetal (NonDominatedArchive)": lambda moa, z: [moa.add(p) for p in z],
    }

    if n_obj == 2:
        setup["moocore (NDTree)"] = lambda z: {
            "moa": moocore._archive.NDTreeArchive(dim=n_obj),
            "z": z,
        }
        benchmarks["moocore (NDTree)"] = lambda moa, z: [moa.add(p) for p in z]

    # Divide time by number of points, so that we report the average time per
    # point for different input sizes.
    normalise = set(benchmarks.keys())
    normalise.remove("moocore (filter_nondominated)")

    bench = Bench(
        name=f"{name}-{n_obj}d",
        n=test_archive_sizes,
        bench=benchmarks,
        setup=setup,
        reps=1,
        max_time=1,
        check=check_bool_array_equal,
        baseline="moocore (UnboundedArchive)",
        skip_check={"moocore (filter_nondominated)"},
    )

    for r in range(n_repeats):
        # We generate the dataset outside setup because we want to do it once
        # per repetition, not once per benchmark.
        rng = np.random.default_rng(42 + r)
        z = datasets[name](max_size, n_obj, rng)
        bench(lambda n: z[:n])

    bench.normalise_times(normalise)

    bench.plots(
        file_prefix=file_prefix,
        title=title,
        log="xy",
        xlabel="Number of points",
        logx_base=2,
        show_ci=True,
    )

if "__file__" not in globals():  # Running interactively.
    plt.show()
