"""Computing Multi-Objective Quality Metrics
=========================================

This is an example of computing various quality metrics on two datasets.

"""

import numpy as np
import moocore

# %%
# First, read the datasets.
#

spherical = moocore.get_dataset("spherical-250-10-3d.txt")
uniform = moocore.get_dataset("uniform-250-10-3d.txt")

spherical_objs = spherical[:, :-1]
spherical_sets = spherical[:, -1]
uniform_objs = uniform[:, :-1] / 10
uniform_sets = uniform[:, -1]

# %%
# Create reference set and reference point.
#

ref_set = moocore.filter_dominated(np.vstack((spherical_objs, uniform_objs)))
ref_point = 1.1

# %%
# Calculate metrics.
#

uniform_igd_plus = moocore.apply_within_sets(
    uniform_objs, uniform_sets, moocore.igd_plus, ref=ref_set
)
spherical_igd_plus = moocore.apply_within_sets(
    spherical_objs, spherical_sets, moocore.igd_plus, ref=ref_set
)

uniform_epsilon = moocore.apply_within_sets(
    uniform_objs, uniform_sets, moocore.epsilon_mult, ref=ref_set
)
spherical_epsilon = moocore.apply_within_sets(
    spherical_objs, spherical_sets, moocore.epsilon_mult, ref=ref_set
)

uniform_hypervolume = moocore.apply_within_sets(
    uniform_objs, uniform_sets, moocore.hypervolume, ref=ref_point
)
spherical_hypervolume = moocore.apply_within_sets(
    spherical_objs, spherical_sets, moocore.hypervolume, ref=ref_point
)

print(f"""
            Uniform       Spherical
            -------       ---------
Mean HV  :  {np.mean(uniform_hypervolume):.5f}       {np.mean(spherical_hypervolume):.5f}
Mean IGD+:  {np.mean(uniform_igd_plus):.5f}       {np.mean(spherical_igd_plus):.5f}
Mean eps*:  {np.mean(uniform_epsilon):.3f}       {np.mean(spherical_epsilon):.3f}

""")
