"""Hypervolume Computation Benchmarks
=======================================

This example benchmarks the hypervolume implementation in ``moocore`` against other implementations.

"""

import pathlib

import matplotlib.pyplot as plt
import numpy as np
import torch
from bench import (
    Bench,
    check_float_values,
    get_geomrange,
    read_datasets_and_filter_dominated,
)
from botorch.utils.multi_objective.hypervolume import Hypervolume as botorch_HV
from fast_pareto import hypervolume as fp_hv

## paretobench is 2x slower than botorch, so too slow for benchmarking.
## https://github.com/electronsandstuff/ParetoBench/issues/57
# import paretobench as pb
## Trieste is hundreds of times slower than botorch. It is so slow that
## we cannot run the benchmark with the initial value of 500 points.
## https://github.com/secondmind-labs/trieste/issues/917
# from trieste.acquisition.multi_objective import Pareto as trieste_Pareto
# import tensorflow as tf
from moarchiving import get_mo_archive as moarch_get_mo_archive
from nevergrad.optimization.multiobjective import HypervolumeIndicator as ng_HV
from optuna._hypervolume import compute_hypervolume as optuna_hv
from pygmo import hypervolume as pygmo_hv

import moocore

# FIXME: How to test both float and Fractions?
moarch_get_mo_archive.hypervolume_computation_float_type = float
moarch_get_mo_archive.hypervolume_final_float_type = float

# See https://github.com/multi-objective/testsuite/tree/main/data
path_to_data = "../../testsuite/data/"
assert pathlib.Path(path_to_data).expanduser().exists()

files = {
    "DTLZLinearShape.3d": {
        "file": path_to_data + "DTLZLinearShape.3d.front.1000pts.10",
        "range": (200, 2000, 10),
    },
    "concave-cliff-3d": {
        "generate": (2000, 3, "concave-cliff", 42),
        "range": (200, 2000, 10),
    },
    "DTLZLinearShape.4d": {
        "file": path_to_data + "DTLZLinearShape.4d.front.1000pts.10",
        "range": (100, 1200, 10),
    },
    "concave-cliff-4d": {
        "generate": (1200, 4, "concave-cliff", 42),
        "range": (100, 1200, 10),
    },
    "DTLZLinearShape.5d": {
        "file": path_to_data + "DTLZLinearShape.5d.front.500pts.10",
        "range": (10, 600, 10),
    },
    "concave-cliff-5d": {
        "generate": (600, 5, "concave-cliff", 42),
        "range": (10, 600, 10),
    },
    "DTLZLinearShape.6d": {
        "file": path_to_data + "DTLZLinearShape.6d.front.700pts.10.xz",
        "range": (10, 500, 10),
    },
    "concave-cliff-6d": {
        "generate": (500, 6, "concave-cliff", 42),
        "range": (10, 500, 10),
    },
}


def get_dataset(name):
    if "generate" in files[name]:
        n, d, method, seed = files[name]["generate"]
        return moocore.generate_ndset(n, d, method=method, seed=seed)
    return read_datasets_and_filter_dominated(files[name]["file"])


title = "HV computation"
file_prefix = "hv"

print(f"# Running benchmark: {title}")
names = files.keys()
for name in names:
    x = get_dataset(name)
    dim = x.shape[1]
    ref = np.ones(dim)
    n = get_geomrange(len(x), *files[name]["range"])

    setup = {"botorch": lambda z: torch.from_numpy(-z)}
    benchmarks = {
        "moocore": moocore.Hypervolume(ref=ref),
        "optuna": lambda z: optuna_hv(
            z, reference_point=ref, assume_pareto=False
        ),
        "pygmo": lambda z: pygmo_hv(points=z).compute(ref_point=ref),
        "nevergrad": ng_HV(ref).compute,
        "moarchiving (float)": lambda z: float(
            moarch_get_mo_archive(z, reference_point=ref).hypervolume
        ),
        # "paretobench": lambda z, hv=pb.Hypervolume(ref_point=ref): hv(pb.Population(f=z), None),
        # "trieste": lambda z, tf_ref=tf.convert_to_tensor(ref): float(trieste_Pareto(z, already_non_dominated=True).hypervolume_indicator(tf_ref)),
        "botorch": botorch_HV(ref_point=torch.from_numpy(-ref)).compute,
        "fast_pareto": lambda z: fp_hv(z, ref_point=ref, assume_pareto=False),
    }
    # Too slow to run.
    if dim >= 6:
        del benchmarks["nevergrad"]
        #     del benchmarks["optuna"]
    if dim >= 5:
        # Only supports 2D, 3D and 4D
        del benchmarks["moarchiving (float)"]
    if dim >= 4:
        # del benchmarks["trieste"]
        del benchmarks["botorch"]
        del setup["botorch"]
        # fast_pareto is more than x10_000 slower than moocore in 4D
        del benchmarks["fast_pareto"]

    bench = Bench(
        name=name,
        n=n,
        # Exclude the conversion to torch from the timing.
        setup=setup,
        # elif what == "trieste":
        #     zz = tf.convert_to_tensor(z)
        bench=benchmarks,
        check=check_float_values,
        max_time=1,
    )

    bench(lambda n: x[:n, :])
    bench.plots(file_prefix=file_prefix, title=title, log="xy")

# To not run interactively, use python3 -m bench_ndom (without .py)
if "__file__" not in globals():  # Running interactively.
    plt.show()
