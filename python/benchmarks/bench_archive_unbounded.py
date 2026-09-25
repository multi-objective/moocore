from bench import Bench

import itertools
import numpy as np
import matplotlib.pyplot as plt

import moocore

from moarchiving import get_mo_archive as moarch_get_mo_archive

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
    "cliff-convex": lambda max_size, n_obj, rng: moocore.generate_ndset(
        max_size, n_obj, method="cliff-convex", seed=rng
    ),
    "cliff-concave": lambda max_size, n_obj, rng: moocore.generate_ndset(
        max_size, n_obj, method="cliff-concave", seed=rng
    ),
}

n_repeats = 10

title = "Adding a point to the archive"
file_prefix = "unbarchive_add"
max_exp = {2: 19, 3: 17, 4: 13}
print(f"Running benchmark: {title}")
names = datasets.keys()
n_objs = [2, 3, 4]
for n_obj, name in itertools.product(n_objs, names):
    if n_obj == 2 and name.startswith("cliff"):
        continue
    test_archive_sizes = 2 ** np.arange(1, max_exp[n_obj])
    max_size = test_archive_sizes[-1]

    # This is a bit wasteful because we rebuild the archive from scratch for
    # each size.  On the other hand, it is probably less sensitive to noise.
    setup = {
        "moocore (UnboundedArchive)": lambda z: dict(
            moa=moocore.UnboundedArchive(dim=n_obj), z=z
        ),
        "moarchiving (float)": lambda z: dict(
            moa=moarch_get_mo_archive(n_obj=n_obj), z=z.tolist()
        ),
    }
    benchmarks = {
        "moocore (UnboundedArchive)": lambda moa, z: [moa.add(p) for p in z],
        "moocore (filter_dominated)": lambda z: moocore.filter_dominated(z),
        "moarchiving (float)": lambda moa, z: [moa.add(p) for p in z],
    }
    normalise = ["moocore (UnboundedArchive)", "moarchiving (float)"]

    if n_obj == 2:
        setup["moocore (NDTree)"] = lambda z: dict(
            moa=moocore._archive.NDTreeArchive(dim=n_obj), z=z
        )
        benchmarks["moocore (NDTree)"] = lambda moa, z: [moa.add(p) for p in z]
        normalise.append("moocore (NDTree)")

    bench = Bench(
        name=f"{name}-{n_obj}d",
        n=test_archive_sizes,
        bench=benchmarks,
        setup=setup,
        reps=1,
        max_time=5,
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
