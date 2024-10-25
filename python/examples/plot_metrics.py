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

# %%
# Create reference set.
#

ref_set = moocore.filter_dominated(
    np.vstack((spherical[:, :-1], uniform[:, :-1]))
)

# %%
# Calculate metrics.
#

uniform_igd_plus = moocore.apply_within_sets(
    uniform[:, :-1], uniform[:, -1], moocore.igd_plus, ref=ref_set
)
spherical_igd_plus = moocore.apply_within_sets(
    spherical[:, :-1], spherical[:, -1], moocore.igd_plus, ref=ref_set
)

print(f"""
            Uniform       Spherical
            -------       ---------
Mean IGD+:  {np.mean(uniform_igd_plus):.5f}       {np.mean(spherical_igd_plus):.5f}

""")
