"""=========================
Using moocore with Pandas
=========================

This example shows how to use ``moocore`` functions with Pandas (https://pandas.pydata.org/). This example requires pandas version >= 2.0.0

"""

import moocore
import pandas as pd

print(f"pandas version: {pd.__version__}")

# %%
# First, we create a toy Pandas :class:`~pandas.DataFrame`.

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
# Normalize it (only replace the objective columns!).

obj_cols = ["obj1", "obj2", "obj3"]
df[obj_cols] = moocore.normalise(df[obj_cols], to_range=[1, 2])
df

# %%
# Calculate the hypervolume for each ``algo`` using :meth:`~pandas.DataFrame.groupby` and :meth:`~pandas.core.groupby.DataFrameGroupBy.apply`.

ref = 2.1
hv = (
    df.groupby("algo")[obj_cols]
    .apply(moocore.hypervolume, ref=ref)
    .reset_index(name="hv")
)
hv

# %%
# Or we can just use:

hv = moocore.apply_within_sets(
    df[obj_cols], df["algo"], moocore.hypervolume, ref=ref
)
hv


# %%
# :func:`moocore.apply_within_sets()` processes each group in
# order, even if the elements of the same group are not contiguous. That is, it
# processes the groups like :meth:`pandas.Series.unique` and not like
# :class:`set` or :func:`numpy.unique()`.

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
obj_cols = ["obj1", "obj2", "obj3"]
df

# %%
# We can still use :meth:`~pandas.DataFrame.groupby` but we may need to reset and clean-up the index.

df.groupby(["algo", "run"])[obj_cols].apply(
    moocore.filter_dominated
).reset_index(level=["algo", "run"])

# %%
# Or we can combine the multiple columns as one to define the sets:
#
sets = df["algo"].astype(str) + "-" + df["run"].astype(str)
sets

# %%
# then identify nondominated rows within each set:
#
is_nondom = moocore.is_nondominated_within_sets(df[obj_cols], sets=sets)
is_nondom

# %%
# And use the boolean vector above to filter rows:
#
df[is_nondom]
