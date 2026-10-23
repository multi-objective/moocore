"""Crossover Return Values
=======================

This script compares the offspring of the crossover operators in ``moocore``
with the offspring of the equivalent operators in other packages.

Each operator crosses over the same parents many times.  For each decision
variable, a two-sample Kolmogorov-Smirnov test compares the offspring of each
package with those of ``moocore``.  The bounds are wide enough that they never
bind, so the comparison does not depend on how each package handles them.

"""

import random

import numpy as np
import polars as pl
from scipy.stats import ks_2samp
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

N = 20_000
LOWER, UPPER = -10.0, 10.0
# Two parents, and a population whose first row is the target.
A = np.array([0.2, 0.7, 0.95])
B = np.array([0.6, 0.3, 0.9])
POPULATION = np.array(
    [
        [0.2, 0.7, 0.95],
        [0.6, 0.3, 0.9],
        [0.5, 0.5, 0.1],
        [0.1, 0.9, 0.4],
        [0.8, 0.2, 0.6],
        [0.4, 0.4, 0.4],
        [0.9, 0.1, 0.2],
        [0.3, 0.6, 0.8],
    ]
)

rng = np.random.default_rng(42)
random.seed(42)


def pair_matrix(a, b, n=N):
    return np.tile(np.array([a, b], dtype=float), (n, 1))


def first_offspring(x):
    """First offspring of each pair, when rows 2i and 2i+1 are a pair."""
    return np.asarray(x)[0::2]


# pymoo
def pymoo_pairs(op, x):
    pop = Population.new("X", x)
    parents = np.arange(len(x)).reshape(-1, 2)
    off = op.do(PymooProblem(n_var=x.shape[1]), pop, parents, random_state=rng)
    # pymoo returns all the first offspring before all the second offspring.
    return off.get("X")[: len(parents)]


def pymoo_with_targets(op, population, others, n=N):
    """Offspring of target row 0, with the other parents from `others`."""
    parents = np.column_stack([np.zeros(n, dtype=int), others(n)])
    problem = PymooProblem(n_var=population.shape[1])
    off = op.do(
        problem, Population.new("X", population), parents, random_state=rng
    )
    return off.get("X")


def random_rows(k, exclude_zero=True):
    def draw(n):
        start = 1 if exclude_zero else 0
        return np.array(
            [
                rng.choice(np.arange(start, len(POPULATION)), k, replace=False)
                for _ in range(n)
            ]
        )

    return draw


def pymoo_de_rand_1_bin(population, n=N):
    # VariantDE crosses over every row once per call, so call it repeatedly
    # and keep the offspring of row 0.
    op = VariantDE(variant="DE/rand/1/bin", CR=0.9, F=0.5, gamma=None)
    problem = PymooProblem(n_var=population.shape[1])
    pop = Population.new("X", population)
    calls = n // 4
    return np.array(
        [
            op.do(problem, pop, len(pop), random_state=rng).get("X")[0]
            for _ in range(calls)
        ]
    )


# DEAP
def deap_pairs(fun, x, *args):
    ind = x.tolist()
    for i in range(0, len(ind) - 1, 2):
        ind[i], ind[i + 1] = fun(ind[i], ind[i + 1], *args)
    return first_offspring(ind)


# jMetalPy
def jmetalpy_solutions(x):
    lower, upper = [LOWER] * x.shape[1], [UPPER] * x.shape[1]
    solutions = []
    for row in x:
        s = FloatSolution(lower, upper, number_of_objectives=1)
        s.variables = row.tolist()
        solutions.append(s)
    return solutions


def jmetalpy_pairs(op, x):
    s = jmetalpy_solutions(x)
    offspring = []
    for i in range(0, len(s) - 1, 2):
        offspring.extend(op.execute([s[i], s[i + 1]]))
    return first_offspring([o.variables for o in offspring])


def jmetalpy_de(population, current, n=N):
    s = jmetalpy_solutions(population)
    if current:
        op = DifferentialEvolutionCrossover(CR=1.0, F=0.5)
        donors = random_rows(2, exclude_zero=False)(n)
    else:
        op = DifferentialEvolutionCrossover(CR=0.9, F=0.5)
        donors = random_rows(3)(n)
    op.current_individual = s[0]
    offspring = []
    for d in donors:
        # execute([a, b, c]) returns c + F (a - b), crossed over with the target.
        parents = (
            [s[d[0]], s[d[1]], s[0]] if current else [s[d[1]], s[d[2]], s[d[0]]]
        )
        offspring.append(op.execute(parents)[0].variables)
    return np.array(offspring)


# DESDEO
def desdeo_run(cls, x, targets=None, binary=False, **kwargs):
    nvar = x.shape[1]
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
    problem = DesdeoProblem(
        name="compare",
        description="Crossover comparison",
        variables=variables,
        objectives=[objective],
    )
    op = cls(
        problem=problem, publisher=Publisher(), verbosity=0, seed=42, **kwargs
    )
    population = pl.DataFrame(
        x, schema=[v.symbol for v in variables], orient="row"
    )
    if targets is None:
        targets = list(range(len(x)))
    return op.do(population=population, to_mate=targets).to_numpy()


def desdeo_pairs(cls, x, **kwargs):
    # DESDEO stacks all the first offspring before all the second offspring.
    return desdeo_run(cls, x, **kwargs)[: len(x) // 2]


x = pair_matrix(A, B)
xb = pair_matrix(np.zeros(6), np.ones(6))
targets = np.zeros(N, dtype=int)
bounds = dict(lower=LOWER, upper=UPPER)

comparisons = {
    "uniform": (
        first_offspring(moocore.uniform_crossover(x, seed=rng)),
        {
            "pymoo": pymoo_pairs(UniformCrossover(prob=1.0), x),
            "deap": deap_pairs(deap_tools.cxUniform, x, 0.5),
            "desdeo": desdeo_pairs(desdeo_crossover.UniformCrossover, x),
        },
    ),
    "single_point": (
        first_offspring(moocore.single_point_crossover(xb, seed=rng)),
        {
            "pymoo": pymoo_pairs(SinglePointCrossover(prob=1.0), xb),
            "deap": deap_pairs(deap_tools.cxOnePoint, xb),
            "desdeo": desdeo_pairs(
                desdeo_crossover.SinglePointBinaryCrossover, xb, binary=True
            ),
        },
    ),
    "blend_alpha": (
        moocore.blend_alpha_crossover(x, seed=rng, **bounds),
        {
            "deap": deap_pairs(deap_tools.cxBlend, x, 0.5),
            "jMetalPy": jmetalpy_pairs(
                BLXAlphaCrossover(probability=1.0, alpha=0.5), x
            ),
            # DESDEO produces two independent offspring per pair.
            "desdeo": desdeo_run(desdeo_crossover.BlendAlphaCrossover, x),
        },
    ),
    "single_arithmetic": (
        first_offspring(moocore.single_arithmetic_crossover(x, seed=rng)),
        {"desdeo": desdeo_pairs(desdeo_crossover.SingleArithmeticCrossover, x)},
    ),
    "local": (
        first_offspring(moocore.local_crossover(x, seed=rng)),
        {"desdeo": desdeo_pairs(desdeo_crossover.LocalCrossover, x)},
    ),
    "bounded_exponential": (
        first_offspring(
            moocore.bounded_exponential_crossover(x, seed=rng, **bounds)
        ),
        {
            "desdeo": desdeo_pairs(
                desdeo_crossover.BoundedExponentialCrossover, x
            )
        },
    ),
    "de_rand_1_bin": (
        moocore.de_rand_1_bin_crossover(
            POPULATION, targets=targets, seed=rng, **bounds
        ),
        {
            "pymoo": pymoo_de_rand_1_bin(POPULATION),
            "jMetalPy": jmetalpy_de(POPULATION, current=False),
            "desdeo": desdeo_run(
                desdeo_crossover.DifferentialEvolutionCrossover,
                POPULATION,
                targets=targets.tolist(),
            ),
        },
    ),
    "de_current_1": (
        moocore.de_current_1_crossover(
            POPULATION, targets=targets, seed=rng, **bounds
        ),
        {
            "pymoo": pymoo_with_targets(
                DEX(F=0.5, CR=1.0, at_least_once=False),
                POPULATION,
                random_rows(2, exclude_zero=False),
            ),
            "jMetalPy": jmetalpy_de(POPULATION, current=True),
        },
    ),
    "parent_centric": (
        moocore.parent_centric_crossover(
            POPULATION, targets=targets, seed=rng, **bounds
        ),
        {
            "pymoo": pymoo_with_targets(
                PCX(eta=0.1, zeta=0.1, prob=1.0), POPULATION, random_rows(2)
            ),
            "desdeo": desdeo_run(
                desdeo_crossover.ParentCentricCrossover,
                POPULATION,
                targets=targets.tolist(),
            ),
        },
    ),
}

# Known differences from moocore.
notes = {
    ("single_point", "desdeo"): "Binary variables only.",
    ("blend_alpha", "deap"): "Two mirrored offspring per pair, no bounds.",
    ("blend_alpha", "jMetalPy"): "Clips each sample instead of the interval.",
    (
        "de_rand_1_bin",
        "pymoo",
    ): "Forces a mutant variable only when none was chosen, instead of a random one in every offspring.",
    (
        "de_current_1",
        "pymoo",
    ): "Multiplies F by a random factor within 1 +/- 5e-5.",
    (
        "de_current_1",
        "jMetalPy",
    ): "Forces a mutant variable, so it differs when CR < 1.",
}

print(
    "| Operator | Package | Offspring | Smallest KS p-value "
    "| Largest mean difference / sd | sd ratio | Notes |"
)
print("|---|---|---|---|---|---|---|")
for op, (ours, others) in comparisons.items():
    ours = np.asarray(ours, dtype=float)
    for package, theirs in others.items():
        theirs = np.asarray(theirs, dtype=float)
        nvar = ours.shape[1]
        pvalues = [
            ks_2samp(ours[:, j], theirs[:, j]).pvalue for j in range(nvar)
        ]
        sd = ours.std(axis=0)
        sd = np.where(sd > 0, sd, 1)
        mean_diff = np.abs(ours.mean(axis=0) - theirs.mean(axis=0)) / sd
        sd_ratio = theirs.std(axis=0) / sd
        print(
            f"| {op} | {package} | {len(theirs)} | {min(pvalues):.3f} | "
            f"{mean_diff.max():.3f} | {sd_ratio.min():.3f}-{sd_ratio.max():.3f} | "
            f"{notes.get((op, package), '')} |"
        )
