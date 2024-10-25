# ruff: noqa: D100, D101, D102, D103
import pytest
import moocore

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
