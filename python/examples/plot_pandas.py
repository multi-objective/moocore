"""Using moocore with Pandas
==========================

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
# Note that :func:`moocore.apply_within_sets()` processes each group in order, even if the elements of the same group are not contiguous. That is, it processes the groups like :meth:`pandas.Series.unique` and not like :class:`set` or :func:`numpy.unique()`.

df["algo"].unique()

# %%
# If we have multiple columns that we want to use to define the sets, such as ``algo`` and ``run``:

df = pd.DataFrame(
    dict(
        obj1=[1, 2, 3, 4, 5, 6, 5, 4, 3, 1],
        obj2=[6, 5, 4, 3, 2, 1, 5, 4, 5, 6],
        obj3=[1, 2, 3, 4, 5, 6, 6, 7, 5, 2],
        algo=["a"] * 3 + ["b"] * 3 + ["a", "b"] * 2,
        run=[1, 1, 2, 1, 1, 2, 2, 2, 1, 1],
    )
)
df

# %%
# We can still use :meth:`pandas.DataFrame.groupby` but we may need to reset and clean-up the index.

df.groupby(["algo", "run"]).apply(
    moocore.filter_dominated, include_groups=False
).reset_index().drop(columns="level_2")

# %%
# Or we can combine the multiple columns as one to define the sets.
#
sets = df["algo"].astype(str) + "-" + df["run"].astype(str)
sets

# %%
# Identify nondominated rows within each set.
#
is_nondom = moocore.is_nondominated_within_sets(
    df[["obj1", "obj2", "obj2"]], sets=sets
)
is_nondom

# %%
# And use the boolean vector above to filter rows.
#
df[is_nondom]
