"""Computing Multi-Objective Quality Metrics
=========================================

TODO: Expand this

"""

import numpy as np
import moocore

# %%
# First, read the datasets.
#

spherical = moocore.get_dataset("spherical-250-10-3d.txt")
uniform = moocore.get_dataset("uniform-250-10-3d.txt")

ref = 1.1
ref_set = moocore.filter_dominated(
    np.vstack((spherical[:, :-1], uniform[:, :-1]))
)


def apply_within_sets(x, fun, **kwargs):
    """Apply ``fun`` for each dataset in ``x``."""
    _, uniq_index = np.unique(x[:, -1], return_index=True)
    x_split = np.vsplit(x[:, :-1], uniq_index[1:])
    return [fun(g, **kwargs) for g in x_split]


uniform_igd_plus = apply_within_sets(uniform, moocore.igd_plus, ref=ref_set)
spherical_igd_plus = apply_within_sets(
    spherical, moocore.igd_plus, ref=ref_set
)

print(f"""
            Uniform       Spherical
            -------       ---------
Mean IGD+:  {np.mean(uniform_igd_plus):.5f}       {np.mean(spherical_igd_plus):.5f}

""")
