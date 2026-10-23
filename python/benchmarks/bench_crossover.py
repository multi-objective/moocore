"""Crossover Benchmarks
====================

This example benchmarks the crossover operators in ``moocore`` against the
equivalent operators in other packages, when they exist.

Each package receives the parents in its own data structure, which is created
before the timing starts.  When a package expects the caller to choose the
other parents of DE or PCX, choosing them is timed, because ``moocore`` does it
inside the call.

"""

import random

from bench import Bench

import numpy as np
import polars as pl
import matplotlib.pyplot as plt
import moocore

from deap import tools as deap_tools
from desdeo.emo.operators import crossover as desdeo_crossover
from desdeo.problem.schema import (
    Objective,
    Problem as DesdeoProblem,
    Variable,
    VariableTypeEnum,
)
from desdeo.tools.patterns import Publisher
from jmetal.core.solution import FloatSolution
from jmetal.operator.crossover import (
    BLXAlphaCrossover,
    DifferentialEvolutionCrossover,
)
from pymoo.algorithms.moo.nsde import VariantDE
from pymoo.core.population import Population
from pymoo.core.problem import Problem as PymooProblem
from pymoo.operators.crossover.dex import DEX
from pymoo.operators.crossover.pcx import PCX
from pymoo.operators.crossover.pntx import SinglePointCrossover
from pymoo.operators.crossover.ux import UniformCrossover

LOWER, UPPER = 0.0, 1.0
# Number of parents for a fixed number of variables, and vice versa.
N_PARENTS = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000]
N_VARS = [2, 5, 10, 20, 50, 100, 200, 500, 1000]
FIXED_NVAR = 30
FIXED_NPARENTS = 100

rng = np.random.default_rng(42)
# DEAP and jMetalPy draw from the random module.
random.seed(42)


def distinct_indices(targets, n, k, exclude_targets=True):
    """Draw k distinct row indices in [0, n) for each target."""
    m = len(targets)
    if exclude_targets:
        excluded = targets[:, None].copy()
    else:
        excluded = np.empty((m, 0), dtype=int)
    chosen = np.empty((m, k), dtype=int)
    for j in range(k):
        drawn = rng.integers(0, n - excluded.shape[1], size=m)
        for col in range(excluded.shape[1]):
            drawn += drawn >= excluded[:, col]
        chosen[:, j] = drawn
        excluded = np.sort(np.column_stack([excluded, drawn]), axis=1)
    return chosen


def check_offspring(a, b, what, n, name):
    """Check that each package returns as many offspring as moocore."""
    if hasattr(b, "to_numpy"):  # DESDEO
        b = b.to_numpy()
    elif len(b) and hasattr(b[0], "variables"):  # jMetalPy
        b = [s.variables for s in b]
    b = np.asarray(b, dtype=float)
    assert b.shape == a.shape, (
        f"In {name}, n={n}, {what} returned shape {b.shape}, but moocore returned {a.shape}"
    )


# pymoo: a Population and an unbounded Problem, so that pymoo does not repair
# the offspring.  Offspring are clipped as in moocore when needed.
def pymoo_setup(x):
    return PymooProblem(n_var=x.shape[1]), Population.new("X", x)


def pymoo_bounded_setup(x):
    problem = PymooProblem(n_var=x.shape[1], xl=LOWER, xu=UPPER)
    return problem, Population.new("X", x)


def pymoo_pairs(op):
    def run(problem, pop):
        parents = np.arange(len(pop)).reshape(-1, 2)
        return op.do(problem, pop, parents, random_state=rng).get("X")

    return run


pymoo_dex = DEX(F=0.5, CR=1.0, at_least_once=False)
pymoo_pcx = PCX(eta=0.1, zeta=0.1, prob=1.0)
pymoo_variant_de = VariantDE(
    variant="DE/rand/1/bin", CR=0.9, F=0.5, gamma=None, de_repair="to-bounds"
)


def pymoo_de_rand_1_bin(problem, pop):
    return pymoo_variant_de.do(problem, pop, len(pop), random_state=rng).get(
        "X"
    )


def pymoo_de_current_1(problem, pop):
    targets = np.arange(len(pop))
    donors = distinct_indices(targets, len(pop), 2, exclude_targets=False)
    parents = np.column_stack([targets, donors])
    off = pymoo_dex.do(problem, pop, parents, random_state=rng)
    return np.clip(off.get("X"), LOWER, UPPER)


def pymoo_parent_centric(problem, pop):
    targets = np.arange(len(pop))
    parents = np.column_stack([targets, distinct_indices(targets, len(pop), 2)])
    off = pymoo_pcx.do(problem, pop, parents, random_state=rng)
    return np.clip(off.get("X"), LOWER, UPPER)


# DEAP: lists of floats.  The operators change both individuals in place.
def deap_setup(x):
    return x.tolist()


def deap_pairs(fun, *args):
    def run(ind):
        for i in range(0, len(ind) - 1, 2):
            ind[i], ind[i + 1] = fun(ind[i], ind[i + 1], *args)
        return ind

    return run


# jMetalPy: lists of FloatSolution.
def jmetalpy_setup(x):
    lower, upper = [LOWER] * x.shape[1], [UPPER] * x.shape[1]
    solutions = []
    for row in x:
        s = FloatSolution(lower, upper, number_of_objectives=1)
        s.variables = row.tolist()
        solutions.append(s)
    return solutions


def jmetalpy_pairs(op):
    def run(solutions):
        offspring = []
        for i in range(0, len(solutions) - 1, 2):
            offspring.extend(op.execute([solutions[i], solutions[i + 1]]))
        return offspring

    return run


jmetalpy_de_rand = DifferentialEvolutionCrossover(CR=0.9, F=0.5)
jmetalpy_de_current = DifferentialEvolutionCrossover(CR=1.0, F=0.5)


def jmetalpy_de_rand_1_bin(solutions):
    # execute([a, b, c]) returns c + F (a - b), crossed over with the target.
    targets = np.arange(len(solutions))
    offspring = []
    for i, (r1, r2, r3) in zip(
        targets, distinct_indices(targets, len(solutions), 3)
    ):
        jmetalpy_de_rand.current_individual = solutions[i]
        offspring.extend(
            jmetalpy_de_rand.execute(
                [solutions[r2], solutions[r3], solutions[r1]]
            )
        )
    return offspring


def jmetalpy_de_current_1(solutions):
    targets = np.arange(len(solutions))
    donors = distinct_indices(targets, len(solutions), 2, exclude_targets=False)
    offspring = []
    for i, (r1, r2) in zip(targets, donors):
        jmetalpy_de_current.current_individual = solutions[i]
        offspring.extend(
            jmetalpy_de_current.execute(
                [solutions[r1], solutions[r2], solutions[i]]
            )
        )
    return offspring


# DESDEO: a polars DataFrame and an operator built for the problem.
def desdeo_problem(nvar, binary=False):
    if binary:
        kind, lower, upper = VariableTypeEnum.binary, 0, 1
    else:
        kind, lower, upper = VariableTypeEnum.real, LOWER, UPPER
    variables = [
        Variable(
            name=f"x_{i}",
            symbol=f"x_{i}",
            variable_type=kind,
            lowerbound=lower,
            upperbound=upper,
            initial_value=lower,
        )
        for i in range(1, nvar + 1)
    ]
    objective = Objective(name="f_1", symbol="f_1", func="x_1", maximize=False)
    return DesdeoProblem(
        name="bench",
        description="Crossover benchmark",
        variables=variables,
        objectives=[objective],
    )


def desdeo_setup(cls, binary=False, **kwargs):
    def setup(x):
        problem = desdeo_problem(x.shape[1], binary=binary)
        op = cls(
            problem=problem,
            publisher=Publisher(),
            verbosity=0,
            seed=42,
            **kwargs,
        )
        symbols = [v.symbol for v in problem.get_flattened_variables()]
        population = pl.DataFrame(x, schema=symbols, orient="row")
        return op, population, list(range(len(x)))

    return setup


def desdeo_run(op, population, rows):
    return op.do(population=population, to_mate=rows)


def moocore_blend_alpha(x):
    # moocore produces one offspring per pair.  Call it twice to produce as
    # many offspring as the other packages.
    return np.vstack(
        [
            moocore.blend_alpha_crossover(x, lower=LOWER, upper=UPPER, seed=rng)
            for _ in range(2)
        ]
    )


# For each operator: (data, {package: (setup, function)}).
real = "real"
binary = "binary"
operators = {
    "uniform": (
        real,
        {
            "moocore": (None, lambda x: moocore.uniform_crossover(x, seed=rng)),
            "pymoo": (pymoo_setup, pymoo_pairs(UniformCrossover(prob=1.0))),
            "deap": (deap_setup, deap_pairs(deap_tools.cxUniform, 0.5)),
            "desdeo": (
                desdeo_setup(desdeo_crossover.UniformCrossover),
                desdeo_run,
            ),
        },
    ),
    "single_point": (
        binary,
        {
            "moocore": (
                None,
                lambda x: moocore.single_point_crossover(x, seed=rng),
            ),
            "pymoo": (pymoo_setup, pymoo_pairs(SinglePointCrossover(prob=1.0))),
            "deap": (deap_setup, deap_pairs(deap_tools.cxOnePoint)),
            "desdeo": (
                desdeo_setup(
                    desdeo_crossover.SinglePointBinaryCrossover, binary=True
                ),
                desdeo_run,
            ),
        },
    ),
    "blend_alpha": (
        real,
        {
            "moocore": (None, moocore_blend_alpha),
            "deap": (deap_setup, deap_pairs(deap_tools.cxBlend, 0.5)),
            "jMetalPy": (
                jmetalpy_setup,
                jmetalpy_pairs(BLXAlphaCrossover(probability=1.0, alpha=0.5)),
            ),
            "desdeo": (
                desdeo_setup(desdeo_crossover.BlendAlphaCrossover),
                desdeo_run,
            ),
        },
    ),
    "single_arithmetic": (
        real,
        {
            "moocore": (
                None,
                lambda x: moocore.single_arithmetic_crossover(x, seed=rng),
            ),
            "desdeo": (
                desdeo_setup(desdeo_crossover.SingleArithmeticCrossover),
                desdeo_run,
            ),
        },
    ),
    "local": (
        real,
        {
            "moocore": (None, lambda x: moocore.local_crossover(x, seed=rng)),
            "desdeo": (
                desdeo_setup(desdeo_crossover.LocalCrossover),
                desdeo_run,
            ),
        },
    ),
    "bounded_exponential": (
        real,
        {
            "moocore": (
                None,
                lambda x: moocore.bounded_exponential_crossover(
                    x, lower=LOWER, upper=UPPER, seed=rng
                ),
            ),
            "desdeo": (
                desdeo_setup(desdeo_crossover.BoundedExponentialCrossover),
                desdeo_run,
            ),
        },
    ),
    "de_rand_1_bin": (
        real,
        {
            "moocore": (
                None,
                lambda x: moocore.de_rand_1_bin_crossover(
                    x, lower=LOWER, upper=UPPER, seed=rng
                ),
            ),
            "pymoo": (pymoo_bounded_setup, pymoo_de_rand_1_bin),
            "jMetalPy": (jmetalpy_setup, jmetalpy_de_rand_1_bin),
            "desdeo": (
                desdeo_setup(desdeo_crossover.DifferentialEvolutionCrossover),
                desdeo_run,
            ),
        },
    ),
    "de_current_1": (
        real,
        {
            "moocore": (
                None,
                lambda x: moocore.de_current_1_crossover(
                    x, lower=LOWER, upper=UPPER, seed=rng
                ),
            ),
            "pymoo": (pymoo_setup, pymoo_de_current_1),
            "jMetalPy": (jmetalpy_setup, jmetalpy_de_current_1),
        },
    ),
    "parent_centric": (
        real,
        {
            "moocore": (
                None,
                lambda x: moocore.parent_centric_crossover(
                    x, lower=LOWER, upper=UPPER, seed=rng
                ),
            ),
            "pymoo": (pymoo_setup, pymoo_parent_centric),
            "desdeo": (
                desdeo_setup(desdeo_crossover.ParentCentricCrossover),
                desdeo_run,
            ),
        },
    ),
}


def get_parents(nparents, nvar, data):
    if data == binary:
        return rng.integers(0, 2, size=(nparents, nvar)).astype(float)
    return rng.uniform(LOWER, UPPER, size=(nparents, nvar))


file_prefix = "crossover"
for op, (data, packages) in operators.items():
    title = f"{op}_crossover()"
    print(f"# Running benchmark: {title}")
    sweeps = {
        "nparents": (
            N_PARENTS,
            "Number of parents",
            lambda n: get_parents(n, FIXED_NVAR, data),
        ),
        "nvars": (
            N_VARS,
            "Number of variables",
            lambda n: get_parents(FIXED_NPARENTS, n, data),
        ),
    }
    for sweep, (n, xlabel, get_testcase) in sweeps.items():
        bench = Bench(
            name=f"{op}-{sweep}",
            n=n,
            bench={k: fun for k, (_, fun) in packages.items()},
            setup={k: setup for k, (setup, _) in packages.items() if setup},
            check=check_offspring,
            max_time=10,
        )
        bench(get_testcase)
        bench.plots(
            file_prefix=file_prefix, title=title, log="xy", xlabel=xlabel
        )
    plt.close("all")

if "__file__" not in globals():  # Running interactively.
    plt.show()
