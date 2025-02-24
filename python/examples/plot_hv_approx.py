r"""Comparing methods for approximating the hypervolume
===================================================

Comparing DZ2019 and HypE
-------------------------

This example shows how to approximate the hypervolume metric of the ``CPFs.txt`` dataset using both HypE, :func:`moocore.whv_hype()`, and DZ2019, :func:`moocore.hv_approx()` for several
values of the number of samples between :math:`10^1` and :math:`10^4`. We repeat each
calculation 10 times to account for stochasticity.
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import moocore

ref = 2.1
x = moocore.get_dataset("CPFs.txt")[:, :-1]
x = moocore.filter_dominated(x)
x = moocore.normalise(x, to_range=[1, 2])
true_hv = moocore.hypervolume(x, ref=ref)
rng1 = np.random.default_rng(42)
rng2 = np.random.default_rng(42)

hype = {}
dz = {}
imax = 4
for i in range(1, imax + 1):
    hype[i] = []
    dz[i] = []
    for r in range(15):
        res = moocore.whv_hype(x, ref=ref, ideal=0, nsamples=10**i, seed=rng1)
        hype[i].append(res)
        res = moocore.hv_approx(x, ref=ref, nsamples=10**i, seed=rng2)
        dz[i].append(res)
print(
    f"True HV    : {true_hv:.5f}",
    f"Mean HYPE  : {np.mean(hype[imax]):.5f} [{np.min(hype[imax]):.5f}, {np.max(hype[imax]):.5f}]",
    f"Mean DZ2019: {np.mean(dz[imax]):.5f} [{np.min(dz[imax]):.5f}, {np.max(dz[imax]):.5f}]",
    sep="\n",
)


# %%
# Next, we plot the results.

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

ax = sns.lineplot(x="samples", y="value", hue="Method", data=df, marker="o")
ax.set(xscale="log", yscale="log", ylabel="Relative error")
plt.show()

# %%
#
# Comparing Monte-Carlo and quasi-Monte-Carlo approximations
# ----------------------------------------------------------
#
# The quasi-Monte-Carlo approximation with ``method=DZ2019-HW`` is deterministic but not monotonic on the number of samples. Nevertheless, it tends to be better than the Monte-Carlo approximation generated with ``method=DZ2019-MC``, specially with large number of samples.

x = moocore.get_dataset("ran.10pts.9d.10")[:, :-1]
x = moocore.filter_dominated(x)
ref = 10
exact = moocore.hypervolume(x, ref=ref)

samples = 2 ** np.arange(14, 21)
maxiter = samples.max()
res = []
for i in samples:
    hv = moocore.hv_approx(x, ref=ref, nsamples=i, method="DZ2019-HW")
    res.append(dict(samples=i, method="HW", hv=hv))

for k in range(16):
    seed = k
    for i in samples:
        hv = moocore.hv_approx(
            x, ref=ref, nsamples=i, method="DZ2019-MC", seed=seed
        )
        res.append(dict(samples=i, method="MC", hv=hv))

df = pd.DataFrame(res)
df["hverror"] = np.abs(1.0 - (df.hv / exact))
df["method"] = df["method"].astype("category")

sns.set_theme(style="darkgrid")
# Plot the responses for different events and regions
ax = sns.lineplot(data=df, x="samples", y="hverror", hue="method", marker="o")
ax.set(xscale="log", yscale="log", ylabel="Relative error")
plt.xticks(samples, [f"2^{int(p)}" for p in np.log2(samples)])
plt.tight_layout()
plt.show()
