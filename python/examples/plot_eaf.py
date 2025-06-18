r"""Empirical Attainment Function
=================================

This example illustrates functionality related to the EAF.

The AUC of the EAF and the AOC (Hypervolume)
--------------------------------------------

The Area-Over-the-Curve (i.e., the hypervolume) of a set of nondominated sets is exactly the the Area-Under-the-Curve (AUC) of their corresponding EAF :cite:p:`LopVerDreDoe2025`, as this example shows.

"""

import numpy as np
import pandas as pd
import moocore

A = moocore.get_dataset("ALG_1_dat.xz")
A[:, :2] = moocore.normalise(A[:, :2], to_range=[0, 1])

aoc = moocore.apply_within_sets(A[:, :2], A[:, 2], moocore.hypervolume, ref=1)
aoc = aoc.mean()
eaf_a = moocore.eaf(A[:, :-1], sets=A[:, -1])
eaf_a[:, 2] /= 100

auc = moocore.hypervolume(eaf_a, ref=[1, 1, 0], maximise=[False, False, True])
nruns = len(np.unique(A[:, 2]))
print(f"Runs = {nruns}\nAUC of EAF = {auc}\nMean AOC = {aoc}\n")

runs = range(5, nruns + 1)
aocs = []
aucs = []
for r in runs:
    a = A[A[:, 2] <= r, :]
    sets = a[:, -1]
    a = a[:, :-1]
    aoc = moocore.apply_within_sets(a, sets, moocore.hypervolume, ref=1).mean()
    eaf_a = moocore.eaf(a, sets)
    eaf_a[:, 2] /= 100
    auc = moocore.hypervolume(
        eaf_a, ref=[1, 1, 0], maximise=[False, False, True]
    )
    aocs += [aoc]
    aucs += [auc]

df = pd.DataFrame(dict(r=runs, AOC=aocs, AUC=aucs)).set_index("r")
df.plot(style=["rs-", "c^--"], xlabel="Number of sets", ylabel="Value")
