r"""Using moocore with Pandas
=============================

This example shows how to use ``moocore`` functions with ``pandas`` (https://pandas.pydata.org/).
"""

import pandas as pd
import moocore

# %%
# First, we create a toy Pandas DataFrame.

df = pd.DataFrame(
    dict(
        obj1=[1, 2, 3, 4, 5],
        obj2=[5, 4, 3, 2, 1],
        obj3=[100, 200, 200, 300, 100],
        algo=2 * ["foo"] + 2 * ["bar"] + ["foo"],
    )
)
df

# %%
# Now we normalize it being careful to replace the correct columns.

obj_cols = ["obj1", "obj2", "obj3"]
df[obj_cols] = moocore.normalise(df[obj_cols], to_range=[1, 2])
df

# %%
# Now we calculate the hypervolume for each algo using :meth:`pandas.core.groupby.DataFrameGroupBy.apply`.

ref = 2.1

hv = (
    df.groupby("algo")
    .apply(moocore.hypervolume, ref=ref, include_groups=False)
    .reset_index(name="hv")
)
hv

# %%
# We can also use


hv = moocore.apply_within_sets(
    df[obj_cols], df["algo"], moocore.hypervolume, ref=ref
)
hv


# %%
# Note that :func:`moocore.apply_within_sets()` processes each group in order, even if the elements of the same group are not contiguous. That is, if processes the groups like :meth:`pandas.Series.unique` and not like :class:`set` or :func:`numpy.unique()`.

df["algo"].unique()
