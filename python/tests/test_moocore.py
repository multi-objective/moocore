# ruff: noqa: D100, D101, D102, D103
import pytest
import numpy as np
import math

import moocore

# FIXME: For some reason this stopped working!!!
# def test_docstrings():
#     import doctest
#     doctest.FLOAT_EPSILON = 1e-9
#     # Run doctests for "moocore" module and fail if one of the docstring tests is incorrect.
#     failed, total = doctest.testmod(moocore, raise_on_error=True)
#     assert failed == 0
#     assert total > 0


def test_read_datasets_data(test_datapath):
    """Check that the moocore.read_datasets() functions returns the same array as that which is calculated from the R library."""

    def check_testdata(testpath, expected_name, expected_shape):
        testdata = moocore.read_datasets(testpath)
        assert (
            testdata.shape == expected_shape
        ), f"Read data array has incorrect shape, should be {expected_shape} but is {testdata.shape}"
        if expected_name != "":
            check_data = np.loadtxt(
                test_datapath(f"expected_output/read_datasets/{expected_name}")
            )

        assert np.allclose(
            testdata, check_data
        ), f"read_datasets does not produce expected array for file {testpath}"

    test_names = [
        "input1.dat",
        "spherical-250-10-3d.txt",
        "uniform-250-10-3d.txt",
        "wrots_l10w100_dat",
        "wrots_l100w10_dat",
        "ALG_1_dat.xz",
    ]
    expected_names = [
        "dat1_read_datasets.txt",
        "spherical_read_datasets.txt",
        "uniform_read_datasets.txt",
        "wrots_l10_read_datasets.txt",
        "wrots_l100_read_datasets.txt",
        "ALG_1_dat_read_datasets.txt",
    ]
    expected_shapes = [
        (100, 3),
        (2500, 4),
        (2500, 4),
        (3262, 3),
        (888, 3),
        (23260, 3),
    ]

    for test, expected_name, expected_shape in zip(
        test_names, expected_names, expected_shapes
    ):
        check_testdata(test_datapath(test), expected_name, expected_shape)


def test_read_datasets_badname():
    """Check that the `moocore.read_datasets()` functions fails correctly after a bad file name is input."""
    with pytest.raises(Exception) as expt:
        moocore.read_datasets("nonexistent_file.txt")

    assert str(expt.value) == "file 'nonexistent_file.txt' not found"
    assert expt.type is FileNotFoundError


def test_read_datasets_errorcode(test_datapath):
    """Checks that an exception is raised when `read_datasets()` returns an error code, as well as checking specific error types from the `ReadDatasetsError` type."""
    with pytest.raises(Exception) as expt:
        moocore.read_datasets(test_datapath("empty"))
    assert expt.type is moocore.ReadDatasetsError
    assert expt.value.message == "READ_INPUT_FILE_EMPTY"

    with pytest.raises(Exception) as expt:
        moocore.read_datasets(test_datapath("column_error.dat"))
    assert expt.type is moocore.ReadDatasetsError
    assert expt.value.message == "ERROR_COLUMNS"


class TestHypervolume:
    """Test hypervolume function."""

    input1 = moocore.get_dataset("input1.dat")

    def test_hv_output(self, test_datapath):
        """Checks the hypervolume calculation produces the correct value."""
        X = self.input1
        hv = moocore.hypervolume(X[X[:, 2] == 1, :2], ref=np.array([10, 10]))
        assert math.isclose(
            hv, 90.46272765
        ), "input1.dat hypervolume produces wrong output"

        hv = moocore.hypervolume(X[X[:, 2] == 1, :2], ref=[10, 10])
        assert math.isclose(
            hv, 90.46272765
        ), "input1.dat hypervolume produces wrong output"

        X = moocore.read_datasets(test_datapath("duplicated3.inp"))[:, :-1]
        hv = moocore.hypervolume(
            X, ref=[-14324, -14906, -14500, -14654, -14232, -14093]
        )
        assert math.isclose(hv, 1.52890128312393e20)

    def test_hv_wrong_ref(self, test_datapath):
        """Check that the moocore.hv() functions fails correctly after a ref with the wrong dimensions is input."""
        X = self.input1
        with pytest.raises(Exception) as expt:
            moocore.hypervolume(
                X[X[:, 2] == 1, :2], ref=np.array([10, 10, 10])
            )
        assert expt.type is ValueError


def test_igd():
    ref = np.array([10, 0, 6, 1, 2, 2, 1, 6, 0, 10]).reshape((-1, 2))
    A = np.array([4, 2, 3, 3, 2, 4]).reshape((-1, 2))
    B = np.array([8, 2, 4, 4, 2, 8]).reshape((-1, 2))
    assert math.isclose(moocore.igd(A, ref), 3.707092031609239)
    assert math.isclose(moocore.igd(B, ref), 2.59148346584763)

    assert math.isclose(moocore.igd_plus(A, ref), 1.482842712474619)
    assert math.isclose(moocore.igd_plus(B, ref), 2.260112615949154)

    assert math.isclose(moocore.avg_hausdorff_dist(A, ref), 3.707092031609239)
    assert math.isclose(moocore.avg_hausdorff_dist(B, ref), 2.59148346584763)


def test_is_nondominated(test_datapath):
    X = moocore.get_dataset("input1.dat")
    subset = X[X[:, 2] == 3, :2]
    dominated = moocore.is_nondominated(subset)
    assert (
        dominated
        == [False, False, False, False, True, False, True, True, False, True]
    ).all
    T = np.array(
        [[1, 0, 1], [1, 1, 1], [0, 1, 1], [1, 0, 1], [1, 1, 0], [1, 1, 1]]
    )
    non_dominated = T[moocore.is_nondominated(T)]
    assert (non_dominated == np.array([[0, 1, 1], [1, 0, 1], [1, 1, 0]])).all()
    non_dominated_weak = T[moocore.is_nondominated(T, keep_weakly=True)]
    expct_nondom_weak = np.array([[1, 0, 1], [0, 1, 1], [1, 0, 1], [1, 1, 0]])

    assert np.array_equal(non_dominated_weak, expct_nondom_weak)
    assert np.array_equal(
        moocore.filter_dominated(T, keep_weakly=True), expct_nondom_weak
    )

    x = np.array(
        [
            [0, 0, 0, 0],
            [0, 0, 0, 1],
            [0, 0, 1, 1],
            [0, 0, 1, 2],
            [1, 0, 0, 0],
            [10, 20, 0, 0],
            [20, 10, 0, 0],
            [2, 2, 0, 0],
        ]
    )
    x_nondom = x[moocore.is_nondominated(x, maximise=True)]
    expected_x_nondom = np.array(
        [[0, 0, 1, 2], [10, 20, 0, 0], [20, 10, 0, 0]]
    )
    assert np.array_equal(x_nondom, expected_x_nondom)
    assert np.array_equal(
        moocore.filter_dominated(x, maximise=True), expected_x_nondom
    )
    minmax = np.array([1, 2, 2, 1, 5, 6, 7, 5]).reshape((-1, 2))
    assert np.array_equal(
        moocore.filter_dominated(minmax, maximise=[True, False]),
        np.array([[2, 1], [7, 5]]),
    )
    assert np.array_equal(
        moocore.filter_dominated(minmax, maximise=[False, True]),
        np.array([[1, 2], [5, 6]]),
    )


def test_epsilon():
    """Same as in R package."""
    ref = np.array([10, 1, 6, 1, 2, 2, 1, 6, 1, 10]).reshape((-1, 2))
    A = np.array([4, 2, 3, 3, 2, 4]).reshape((-1, 2))
    assert math.isclose(moocore.epsilon_additive(A, ref), 1.0)
    assert math.isclose(moocore.epsilon_mult(A, ref), 2.0)
    assert math.isclose(moocore.epsilon_mult(A, ref, maximise=True), 2.5)
    assert math.isclose(moocore.epsilon_additive(A, ref, maximise=True), 6.0)


def test_normalise():
    A = np.array(
        [
            [0, 0, 0],
            [5, 3, 1],
            [10, 6, 2],
            [15, 9, 3],
            [20, 12, 4],
            [25, 15, 5],
        ]
    )
    # With default to_range = [0,1] - all columns should have their values normalised to same value
    expected_outcome = np.tile(np.linspace(0, 1, num=6).reshape(6, -1), 3)

    assert np.allclose(moocore.normalise(A), expected_outcome)
    assert np.allclose(
        moocore.normalise(A, to_range=[0, 10]), 10 * expected_outcome
    )
    expected_with_bounds = np.transpose(
        np.array(
            [
                np.linspace(0, 1, num=6),
                np.linspace(0, 0.6, num=6),
                np.linspace(0, 0.2, num=6),
            ]
        )
    )
    assert np.allclose(
        moocore.normalise(A, upper=[25, 25, 25], lower=[0, 0, 0]),
        expected_with_bounds,
    )

    # Check that normalise does not modify the original array.
    A = np.array([[1.0, 2.0], [2.0, 1.0]])
    A_copy = A.copy()
    B = moocore.normalise(A)
    assert np.allclose(A, A_copy)
    assert np.allclose(B, np.array([[0.0, 1.0], [1.0, 0.0]]))


def test_eaf(test_datapath):
    # FIXME ALG_1_dat is creating slightly different percentile values than expected in its EAF output
    test_names = [
        "input1.dat",
        "spherical-250-10-3d.txt",
        "uniform-250-10-3d.txt",
        "wrots_l10w100_dat",
        "wrots_l100w10_dat",
        # "ALG_1_dat.xz",
    ]
    expected_eaf_names = [
        "dat1_eaf.txt",
        "spherical_eaf.txt",
        "uniform_eaf.txt",
        "wrots_l10_eaf.txt",
        "wrots_l100_eaf.txt",
        # "ALG_1_dat_get_eaf.txt"
    ]
    for test_name, expected_eaf_name in zip(test_names, expected_eaf_names):
        filename = test_datapath(test_name)
        dataset = moocore.read_datasets(filename)
        eaf_test = moocore.eaf(dataset)
        eaf_pct_test = moocore.eaf(dataset, percentiles=[0, 50, 100])
        expected_eaf_result = np.loadtxt(
            test_datapath(f"expected_output/eaf/{expected_eaf_name}")
        )
        expected_eaf_pct_result = np.loadtxt(
            test_datapath(f"expected_output/eaf/pct_{expected_eaf_name}")
        )
        assert (
            eaf_test.shape == expected_eaf_result.shape
        ), f"Shapes of {test_name} and {expected_eaf_name} do not match"
        assert np.allclose(
            eaf_test, expected_eaf_result
        ), f"{expected_eaf_name} test failed"
        assert np.allclose(
            eaf_pct_test, expected_eaf_pct_result
        ), f"pct_{expected_eaf_name} test failed"


# def test_eafdiff(test_datapath):
#     diff1 = np.loadtxt(test_datapath("100_diff_points_1.txt"))
#     diff2 = np.loadtxt(test_datapath("100_diff_points_2.txt"))
#     diff = moocore.eaf(diff1, diff2)
#     diff_intervals = moocore.eafdiff(diff1, diff2, intervals=3)

#     expected_diff12 = np.loadtxt(
#         test_datapath("expected_output/eafdiff/points12_get_diff_eaf.txt")
#     )
#     expected_diff12_intervals3 = np.loadtxt(
#         test_datapath("expected_output/eafdiff/int3_points12_get_diff_eaf.txt")
#     )
#     assert np.allclose(diff, expected_diff12)
#     assert np.allclose(diff_intervals, expected_diff12_intervals3)

# FIXME add more tests including intervals
