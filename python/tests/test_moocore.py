# ruff: noqa: D100, D103
import pytest
import numpy as np
from numpy.testing import (
    assert_array_equal,
    assert_allclose,
)
import math
import moocore


def test_read_datasets_data(test_datapath):
    """Check that the moocore.read_datasets() functions returns the same array as that which is calculated from the R library."""

    def check_testdata(testpath, expected_name, expected_shape):
        testdata = moocore.read_datasets(testpath)
        assert testdata.shape == expected_shape, (
            f"Read data array has incorrect shape, should be {expected_shape} but is {testdata.shape}"
        )
        if expected_name != "":
            check_data = np.loadtxt(
                test_datapath(f"expected_output/read_datasets/{expected_name}")
            )

        assert_allclose(
            testdata,
            check_data,
            err_msg=f"read_datasets does not produce expected array for file {testpath}",
        )

    tests = [
        ("input1.dat", "dat1_read_datasets.txt", (100, 3)),
        (
            "spherical-250-10-3d.txt",
            "spherical_read_datasets.txt.xz",
            (2500, 4),
        ),
        ("uniform-250-10-3d.txt", "uniform_read_datasets.txt.xz", (2500, 4)),
        ("wrots_l10w100_dat.xz", "wrots_l10_read_datasets.txt", (3262, 3)),
        ("wrots_l100w10_dat.xz", "wrots_l100_read_datasets.txt", (888, 3)),
        ("ALG_1_dat.xz", "ALG_1_dat_read_datasets.txt.xz", (23260, 3)),
    ]

    for test, expected_name, expected_shape in tests:
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
        dat = X[X[:, 2] == 1, :2]
        hv = moocore.hypervolume(dat, ref=np.array([10, 10]))
        assert math.isclose(hv, 90.46272765), (
            "input1.dat hypervolume produces wrong output"
        )

        hv_ind = moocore.Hypervolume(ref=[10, 10])
        hv = hv_ind(dat)
        assert math.isclose(hv, 90.46272765), (
            "input1.dat Hypervolume(ref = [10,10]) produces wrong output"
        )

        dat = X[X[:, 2] == 2, :2]
        hv = moocore.hypervolume(dat, ref=[10, 10])
        assert math.isclose(hv, 53.969708954), (
            "input1.dat hypervolume produces wrong output"
        )

        hv = hv_ind(dat)
        assert math.isclose(hv, 53.969708954), (
            "input1.dat Hypervolume(ref = [10,10]) produces wrong output"
        )

        X = moocore.read_datasets(test_datapath("duplicated3.inp"))[:, :-1]
        hv = moocore.hypervolume(
            X, ref=[-14324, -14906, -14500, -14654, -14232, -14093]
        )
        assert math.isclose(hv, 1.52890128312393e20)

        ref_set = moocore.read_datasets(test_datapath("duplicated3.inp"))[
            :, :-1
        ]
        ref_set = moocore.filter_dominated(ref_set)
        rhv_ind = moocore.RelativeHypervolume(
            ref=[-14324, -14906, -14500, -14654, -14232, -14093],
            ref_set=ref_set,
        )
        assert rhv_ind(X) == 0

        ref = [2, 2, 2]
        x = [[1, 0, 1], [0, 1, 0]]
        assert moocore.hypervolume(x, ref) == 5.0

    def test_hv_wrong_ref(self, test_datapath):
        """Check that the moocore.hv() functions fails correctly after a ref with the wrong dimensions is input."""
        X = self.input1
        with pytest.raises(Exception) as expt:
            moocore.hypervolume(X[X[:, 2] == 1, :2], ref=np.array([10, 10, 10]))
        assert expt.type is ValueError


def assert_igd(x, ref, value):
    assert math.isclose(moocore.igd(x, ref), value)


def assert_igd_plus(x, ref, value):
    assert math.isclose(moocore.igd_plus(x, ref), value)


def test_igd():
    ref = np.array([10, 0, 6, 1, 2, 2, 1, 6, 0, 10]).reshape((-1, 2))
    A = np.array([4, 2, 3, 3, 2, 4]).reshape((-1, 2))
    B = np.array([8, 2, 4, 4, 2, 8]).reshape((-1, 2))
    assert_igd(A, ref, 3.707092031609239)
    assert_igd(B, ref, 2.59148346584763)

    assert_igd_plus(A, ref, 1.482842712474619)
    assert_igd_plus(B, ref, 2.260112615949154)

    assert math.isclose(moocore.avg_hausdorff_dist(A, ref), 3.707092031609239)
    assert math.isclose(moocore.avg_hausdorff_dist(B, ref), 2.59148346584763)

    ref = np.array([[1, 1]])
    assert_igd(ref, ref, 0.0)
    assert_igd_plus(ref, ref, 0.0)

    ref = np.array([[1, 1], [2, 2]])
    x = np.array([[1, 1]])
    assert_igd(x, ref, 0.7071067811865476)
    assert_igd_plus(x, ref, 0.0)

    x = [[1.5, 1.5], [2.2, 2.2], [1.9, 1.9]]
    assert_igd(x, ref, 0.4242640687119286)

    ref = [[1.0, 1.0], [2.1, 2.1]]
    x = [[1.5, 1.5], [2.2, 2.2]]
    assert_igd(x, ref, 0.4242640687119286)

    ref = np.array([[1, 1, 1], [2, 2, 2]])
    x = np.array([[1, 1, 1]])
    assert_igd(x, ref, 0.8660254037844386)


@pytest.mark.parametrize("dim", range(0, 3))
def test_is_nondominated_keep_weakly(dim):
    def check_keep_weakly(x, true_ndom, true_wndom):
        true_ndom = np.array(true_ndom)
        true_wndom = np.array(true_wndom)

        test_is_ndom = moocore.is_nondominated(x)
        assert_array_equal(test_is_ndom, true_ndom)
        test_filter = moocore.filter_dominated(x)
        assert_array_equal(test_filter, x[true_ndom, :])
        test_is_weak_ndom = moocore.is_nondominated(x, keep_weakly=True)
        assert_array_equal(test_is_weak_ndom, true_wndom)
        test_weak_filter = moocore.filter_dominated(x, keep_weakly=True)
        assert_array_equal(test_weak_filter, x[true_wndom, :])

    x = np.array(
        [[2, 0], [1, 1], [3, 0], [2, 0], [1, 2], [3, 0], [0, 2], [1, 1], [1, 1]]
    )
    check_keep_weakly(
        x=np.append(x, np.zeros((len(x), dim)), axis=1),
        true_ndom=[True, True, False, False, False, False, True, False, False],
        true_wndom=[True, True, False, True, False, False, True, True, True],
    )

    x = np.array(
        [[1, 0, 1], [1, 1, 1], [0, 1, 1], [1, 0, 1], [1, 1, 0], [1, 1, 1]]
    )
    check_keep_weakly(
        x=np.append(x, np.zeros((len(x), dim)), axis=1),
        true_ndom=[True, False, True, False, True, False],
        true_wndom=[True, False, True, True, True, False],
    )


def test_is_nondominated(test_datapath):
    X = moocore.get_dataset("input1.dat")
    subset = X[X[:, 2] == 3, :2]
    non_dominated = moocore.is_nondominated(subset)
    assert_array_equal(
        non_dominated,
        [False, False, False, False, True, False, True, True, False, True],
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
    expected_x_nondom = np.array([[0, 0, 1, 2], [10, 20, 0, 0], [20, 10, 0, 0]])
    assert_array_equal(x_nondom, expected_x_nondom)
    assert_array_equal(
        moocore.filter_dominated(x, maximise=True), expected_x_nondom
    )
    minmax = np.array([1, 2, 2, 1, 5, 6, 7, 5]).reshape((-1, 2))
    assert_array_equal(
        moocore.filter_dominated(minmax, maximise=[True, False]),
        np.array([[2, 1], [7, 5]]),
    )
    assert_array_equal(
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

    assert_allclose(moocore.normalise(A), expected_outcome)
    assert_allclose(
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
    assert_allclose(
        moocore.normalise(A, upper=[25, 25, 25], lower=[0, 0, 0]),
        expected_with_bounds,
    )

    # Check that normalise does not modify the original array.
    A = np.array([[1.0, 2.0], [2.0, 1.0]])
    A_copy = A.copy()
    B = moocore.normalise(A)
    assert_allclose(A, A_copy)
    assert_allclose(B, np.array([[0.0, 1.0], [1.0, 0.0]]))


def test_eaf(test_datapath):
    # FIXME: ALG_1_dat is creating slightly different percentile values than expected in its EAF output
    tests = [
        ("input1.dat", "dat1_eaf.txt"),
        ("spherical-250-10-3d.txt", "spherical_eaf.txt.xz"),
        ("uniform-250-10-3d.txt", "uniform_eaf.txt.xz"),
        ("wrots_l10w100_dat.xz", "wrots_l10_eaf.txt.xz"),
        ("wrots_l100w10_dat.xz", "wrots_l100_eaf.txt.xz"),
        # ("ALG_1_dat.xz",          # "ALG_1_dat_get_eaf.txt.xz"),
    ]
    for test_name, expected_eaf_name in tests:
        filename = test_datapath(test_name)
        dataset = moocore.read_datasets(filename)
        x = dataset[:, :-1]
        sets = dataset[:, -1]
        eaf_test = moocore.eaf(x, sets=sets)
        eaf_pct_test = moocore.eaf(x, sets=sets, percentiles=[0, 50, 100])
        expected_eaf_result = np.loadtxt(
            test_datapath(f"expected_output/eaf/{expected_eaf_name}")
        )
        expected_eaf_pct_result = np.loadtxt(
            test_datapath(f"expected_output/eaf/pct_{expected_eaf_name}")
        )
        assert eaf_test.shape == expected_eaf_result.shape, (
            f"Shapes of {test_name} and {expected_eaf_name} do not match"
        )
        assert_allclose(
            eaf_test,
            expected_eaf_result,
            err_msg=f"{expected_eaf_name} test failed",
        )
        assert_allclose(
            eaf_pct_test,
            expected_eaf_pct_result,
            err_msg=f"pct_{expected_eaf_name} test failed",
        )


def test_get_dataset_path():
    with pytest.raises(Exception) as expt:
        moocore.get_dataset_path("notavailable")
    assert expt.type is ValueError


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


# method="DZ2019-HW" is very slow with dim > 15.
@pytest.mark.parametrize("dim", range(2, 15))
def test_hv_approx(dim):
    x = np.full((1, dim), 0.5)
    ref = np.full(dim, 1.0)
    true_hv = moocore.hypervolume(x, ref=ref)
    appr_hv = moocore.hv_approx(x, ref=ref, method="DZ2019-MC", seed=42)
    # print(f"{dim}: {(true_hv - appr_hv)/true_hv}")
    ## Precision goes down significantly with higher dimensions.
    signif = 3 if dim < 7 else 2
    np.testing.assert_approx_equal(true_hv, appr_hv, significant=signif)

    # method="DZ2019-HW" is the default
    appr_hv = moocore.hv_approx(x, ref=ref)
    # print(f"{dim}: {(true_hv - appr_hv)/true_hv}")
    ## Precision goes down significantly with higher dimensions.
    signif = 4 if dim < 8 else 3 if dim < 10 else 2
    np.testing.assert_approx_equal(true_hv, appr_hv, significant=signif)


def test_hv_approx_default_seed():
    x = np.full((1, 5), 0.5)
    ref = np.full(5, 0.0)
    true_hv = moocore.hypervolume(x, ref=ref, maximise=True)
    appr_hv = moocore.hv_approx(
        x, ref=ref, method="DZ2019-MC", seed=None, maximise=True
    )
    ## Precision goes down significantly with higher dimensions.
    signif = 2
    np.testing.assert_approx_equal(true_hv, appr_hv, significant=signif)


def check_hvc(points, ref, err_msg):
    hvc = moocore.hv_contributions(points, ref=ref)
    is_nondom = moocore.is_nondominated(points, keep_weakly=True)
    nondom = points[is_nondom, :]
    hv_total = moocore.hypervolume(nondom, ref=ref)
    true_hvc = np.zeros(len(is_nondom))
    true_hvc[is_nondom] = hv_total - np.array(
        [
            moocore.hypervolume(np.delete(nondom, i, axis=0), ref=ref)
            for i in range(len(nondom))
        ]
    )
    assert_allclose(true_hvc, hvc, err_msg=err_msg)
    hvc = moocore.hv_contributions(-points, ref=-ref, maximise=True)
    assert_allclose(true_hvc, hvc, err_msg=err_msg)


@pytest.mark.parametrize("dim", range(2, 5))
def test_hvc(dim):
    seed = np.random.default_rng().integers(2**32 - 2)
    rng = np.random.default_rng(seed)
    maximum = 100
    nrows = 25
    ref = np.full(dim, maximum + 1)
    points = rng.integers(1, maximum, (nrows, dim))
    check_hvc(points, ref, err_msg=f"dim={dim}, seed={seed}: ")


@pytest.mark.parametrize("dim", range(2, 5))
def test_pareto_rank(dim):
    seed = np.random.default_rng().integers(2**32 - 2)
    rng = np.random.default_rng(seed)
    nrows = 25
    points = rng.random(size=(nrows, dim))
    ranks = moocore.pareto_rank(points)
    r_max = ranks.max()
    r = 0
    while True:
        nondom = moocore.is_nondominated(points, keep_weakly=True)
        assert_array_equal(
            ranks == r, nondom, err_msg=f"dim={dim}, seed={seed}: "
        )
        if r == r_max:
            break
        points = points[~nondom]
        ranks = ranks[ranks > r]
        r += 1


def check_nondominated_sorting_fronts(x, expected):
    ranks = moocore.pareto_rank(np.array(x))
    fronts = [(g == ranks) for g in np.unique(ranks)]
    assert len(fronts) == len(expected)
    assert_array_equal(fronts, np.array(expected))


def test_nondominated_sorting_3d():
    three_fronts = [
        [1, 2, 3],
        [3, 1, 2],
        [2, 3, 1],
        [10, 20, 30],
        [30, 10, 20],
        [20, 30, 10],
        [100, 200, 300],
        [300, 100, 200],
        [200, 300, 100],
    ]
    three_fronts_expected = [
        [*(3 * [True]), *(6 * [False])],
        [*(3 * [False]), *(3 * [True]), *(3 * [False])],
        [*(6 * [False]), *(3 * [True])],
    ]

    check_nondominated_sorting_fronts(three_fronts, three_fronts_expected)

    # Should result in one front
    one_front = [[1, 2, 3], [3, 1, 2], [2, 3, 1]]
    one_front_expected = [3 * [True]]
    check_nondominated_sorting_fronts(one_front, one_front_expected)

    # Should result in 5 fronts (one for each solution)
    five_fronts = [[1, 1, 1], [2, 2, 2], [3, 3, 3], [4, 4, 4], [5, 5, 5]]
    five_fronts_expected = [
        [True, False, False, False, False],
        [False, True, False, False, False],
        [False, False, True, False, False],
        [False, False, False, True, False],
        [False, False, False, False, True],
    ]
    check_nondominated_sorting_fronts(five_fronts, five_fronts_expected)
