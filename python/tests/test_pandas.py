# ruff: noqa: D100, D101, D102, D103
import pytest
import moocore
from numpy.testing import assert_array_equal, assert_allclose

pd = pytest.importorskip("pandas")


def test_normalise_pandas():
    df = pd.DataFrame(
        dict(
            Bus=[1, 2, 3, 4, 5],
            TA=[8, 3, 5, 7, 4],
            Time=[22292, 47759, 49860, 88740, 92086],
            Method=2 * ["No Delay"] + 3 * ["Implicit"],
        )
    )
    obj_cols = ["Bus", "TA", "Time"]
    df[obj_cols] = moocore.normalise(df[obj_cols])

    df_true = pd.DataFrame(
        dict(
            Bus=[0.0, 0.25, 0.5, 0.75, 1.0],
            TA=[1.0, 0.0, 0.4, 0.8, 0.2],
            Time=[0.0, 0.3648881, 0.39499097, 0.95205892, 1.0],
            Method=2 * ["No Delay"] + 3 * ["Implicit"],
        )
    )

    pd.testing.assert_frame_equal(df, df_true)


def test_example_pandas():
    """Corresponds to ``examples/plot_pandas.py``."""
    df = pd.DataFrame(
        dict(
            obj1=[1, 2, 3, 4, 5],
            obj2=[5, 4, 3, 2, 1],
            obj3=[100, 200, 200, 300, 100],
            algo=2 * ["foo"] + 2 * ["bar"] + ["foo"],
        )
    )
    obj_cols = ["obj1", "obj2", "obj3"]

    df[obj_cols] = moocore.normalise(df[obj_cols], to_range=[1, 2])

    ref = 2.1
    hv = (
        df.groupby("algo")[obj_cols]
        .apply(moocore.hypervolume, ref=ref)
        .reset_index(name="hv")
    )
    pd.testing.assert_frame_equal(
        hv, pd.DataFrame(dict(algo=["bar", "foo"], hv=[0.22475, 0.34350]))
    )

    hv = moocore.apply_within_sets(
        df[obj_cols], df["algo"], moocore.hypervolume, ref=ref
    )
    assert_allclose(hv, [0.3435, 0.22475])

    df = pd.DataFrame(
        dict(
            algo=["a"] * 3 + ["b"] * 3 + ["a", "b"] * 2,
            run=[1, 1, 2, 1, 1, 2, 2, 2, 1, 1],
            obj1=[1, 2, 3, 4, 5, 6, 5, 4, 3, 1],
            obj2=[6, 5, 4, 3, 2, 1, 5, 4, 5, 6],
            obj3=[1, 2, 3, 4, 5, 6, 6, 7, 5, 2],
        )
    )
    pd.testing.assert_frame_equal(
        df.groupby(["algo", "run"])[obj_cols]
        .apply(moocore.filter_dominated)
        .reset_index(level=["algo", "run"]),
        df.iloc[[0, 1, 2, 3, 4, 9, 5, 7], :],
    )

    sets = df["algo"].astype(str) + "-" + df["run"].astype(str)
    is_nondom = moocore.is_nondominated_within_sets(df[obj_cols], sets=sets)
    assert_array_equal(
        is_nondom,
        [True, True, True, True, True, True, False, True, False, True],
    )
