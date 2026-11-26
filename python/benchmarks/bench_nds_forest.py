import itertools

import matplotlib.pyplot as plt
import numpy as np
from bench import Bench

import moocore

datasets = {
    "hypercube-uniform": lambda max_size, n_obj, rng: moocore.generate_sequence(
        max_size, n_obj, method="cube", seed=rng
    ),
    "hypercube-uniform-rep4": lambda max_size, n_obj, rng: (
        moocore.generate_sequence(
            int(max_size / 4), n_obj, method="cube", seed=rng, n_rep=max_size
        )
    ),
    "each-dominates-next": lambda max_size, n_obj, rng: (
        moocore.generate_sequence(
            max_size, n_obj, method="each-dominates-next", seed=rng
        )
    ),
    "each-dominates-previous": lambda max_size, n_obj, rng: (
        moocore.generate_sequence(
            max_size, n_obj, method="each-dominates-previous", seed=rng
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
    "glas2017,c=1": lambda max_size, n_obj, rng: moocore.generate_sequence(
        max_size, n_obj, method="glas2017", c_value=1, seed=rng
    ),
    "glas2017,c=0.5": lambda max_size, n_obj, rng: moocore.generate_sequence(
        max_size, n_obj, method="glas2017", c_value=0.5, seed=rng
    ),
    "glas2017,c=2.0": lambda max_size, n_obj, rng: moocore.generate_sequence(
        max_size, n_obj, method="glas2017", c_value=2.0, seed=rng
    ),
}

n_repeats = 15

title = "Adding a point to the archive"
file_prefix = "nds_forest_add"
max_exp = {2: 18, 3: 17, 4: 12, 5: 17, 10: 17}
print(f"Running benchmark: {title}")
names = datasets.keys()
n_objs = [2, 3, 5, 10]
strategies = moocore.BaseNDSForest.RANK_QUERY_STRATEGIES
for n_obj, name in itertools.product(n_objs, names):
    if n_obj == 2 and name.endswith("cliff"):
        continue
    test_archive_sizes = 2 ** np.arange(1, max_exp[n_obj])
    max_size = test_archive_sizes[-1]

    # This is a bit wasteful because we rebuild the archive from scratch for
    # each size.  On the other hand, it is probably less sensitive to noise.
    setup = {
        f"moocore ({strategy})": lambda z: {
            "moa": moocore.NDSortedArchive(n_obj, query_strategy=strategy),
            "z": z,
        }
        for strategy in strategies
    }

    benchmarks = {
        f"moocore ({strategy})": lambda moa, z: [moa.add(p) for p in z]
        for strategy in strategies
    }
    # benchmarks["moocore (pareto_rank)"] = lambda z: moocore.pareto_rank(z)

    bench = Bench(
        name=f"{name}-{n_obj}d",
        n=test_archive_sizes,
        bench=benchmarks,
        setup=setup,
        reps=1,
        max_time=10,
        baseline=next(iter(benchmarks)),
    )

    for r in range(n_repeats):
        # We generate the dataset outside setup because we want to do it once
        # per repetition, not once per benchmark.
        rng = np.random.default_rng(42 + r)
        z = datasets[name](max_size, n_obj, rng)
        bench(lambda n: z[:n])

    # Divide time by number of points, so that we report the average time per
    # point for different input sizes.
    normalise = set(benchmarks)
    bench.normalise_times(normalise)

    bench.plots(
        file_prefix=file_prefix,
        title=title,
        log="xy",
        xlabel="Archive size",
        logx_base=2,
        show_ci=True,
    )

if "__file__" not in globals():  # Running interactively.
    plt.show()
