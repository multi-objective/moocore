r"""Comparing methods for approximating the hypervolume
===================================================

This example shows how to approximate the hypervolume metric of the
``CPFs.txt`` dataset using both HypE, :func:`moocore.whv_hype()`, and DZ2019,
:func:`moocore.hv_approx()` for several values of the number of samples between
:math:`10^1` and :math:`10^5`.

"""

import numpy as np
import moocore

# %%
#
# First calculate the exact hypervolume.

ref = 2.1
x = moocore.get_dataset("CPFs.txt")[:, :-1]
x = moocore.filter_dominated(x)
x = moocore.normalise(x, to_range=[1, 2])
true_hv = moocore.hypervolume(x, ref=ref)

# %%
#
# Next, we approximate the hypervolume using :math:`\{10^1, 10^2, \ldots,
# 10^5\}` random samples to show the higher samples reduce the approximation
# error. Since the approximation is stochastic, we perform 10 repetitions of
# each computation.

nreps = 10
nsamples_exp = 5
rng1 = np.random.default_rng(42)
rng2 = np.random.default_rng(42)
hype = {}
dz = {}
for i in range(1, nsamples_exp + 1):
    hype[i] = []
    dz[i] = []
    for r in range(nreps):
        res = moocore.whv_hype(x, ref=ref, ideal=0, nsamples=10**i, seed=rng1)
        hype[i].append(res)
        res = moocore.hv_approx(x, ref=ref, nsamples=10**i, seed=rng2)
        dz[i].append(res)

print(
    f"True HV    : {true_hv:.5f}",
    f"Mean HYPE  : {np.mean(hype[5]):.5f} [{np.min(hype[5]):.5f}, {np.max(hype[5]):.5f}]",
    f"Mean DZ2019: {np.mean(dz[5]):.5f} [{np.min(dz[5]):.5f}, {np.max(dz[5]):.5f}]",
    sep="\n",
)


# %%
#
# Next, we plot the results.

import pandas as pd

hype = pd.DataFrame(hype)
dz = pd.DataFrame(dz)
hype["Method"] = "HypE"
dz["Method"] = "DZ2019"
df = (
    pd.concat([hype, dz])
    .reset_index(names="rep")
    .melt(id_vars=["rep", "Method"], var_name="samples")
)
df["samples"] = 10 ** df["samples"]
df["value"] = np.abs(df["value"] - true_hv) / true_hv

import matplotlib.pyplot as plt
import seaborn as sns

ax = sns.lineplot(x="samples", y="value", hue="Method", data=df, marker="o")
ax.set(xscale="log", yscale="log", ylabel="Relative error")
plt.show()
