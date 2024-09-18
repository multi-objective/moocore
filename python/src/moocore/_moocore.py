from __future__ import annotations

import os
from io import StringIO
import numpy as np
from numpy.typing import ArrayLike  # For type hints
from typing import Literal

from math import gamma as gamma_function
# NOTE: if we ever start using SciPy, we can use
# from scipy.special import gamma_function

import lzma
import shutil
import tempfile

from importlib.resources import files

from ._utils import (
    asarray_maybe_copy,
    unique_nosort,
    np2d_to_double_array,
    np1d_to_double_array,
    np1d_to_int_array,
    atleast_1d_of_length_n,
    is_integer_value,
)

## The CFFI library is used to create C bindings.
from ._libmoocore import lib, ffi


class ReadDatasetsError(Exception):
    """Custom exception class for an error returned by the :func:`read_datasets()` function.

    Parameters
    ----------
    error_code : int
        Error code returned by c:func:`read_datasets()` C function, which maps to a string.

    """

    _error_strings = [
        "NO_ERROR",
        "READ_INPUT_FILE_EMPTY",
        "READ_INPUT_WRONG_INITIAL_DIM",
        "ERROR_FOPEN",
        "ERROR_CONVERSION",
        "ERROR_COLUMNS",
    ]

    def __init__(self, error_code):
        self.error = error_code
        self.message = self._error_strings[abs(error_code)]
        super().__init__(self.message)


def read_datasets(filename: str | os.PathLike | StringIO) -> np.ndarray:
    """Read an input dataset file, parsing the file and returning a numpy array.

    Parameters
    ----------
    filename:
        Filename of the dataset file or :class:`~io.StringIO` directly containing the file contents.
        If it does not contain an absolute path, the filename is relative to the current working directory.
        If the filename has extension ``.xz``, it is decompressed to a temporary file before reading it.
        Each line of the file corresponds to one point of one dataset. Different datasets are separated by an empty line.

    Returns
    -------
        An array containing a representation of the data in the file.
        The first :math:`n-1` columns contain the numerical data for each of the objectives.
        The last column contains an identifier for which set the data is relevant to.

    Examples
    --------
    >>> filename = moocore.get_dataset_path("input1.dat")
    >>> moocore.read_datasets(filename)  # doctest: +ELLIPSIS
    array([[ 8.07559653,  2.40702554,  1.        ],
           [ 8.66094446,  3.64050144,  1.        ],
           [ 0.20816431,  4.62275469,  1.        ],
           ...
           [ 4.92599726,  2.70492519, 10.        ],
           [ 1.22234394,  5.68950311, 10.        ],
           [ 7.99466959,  2.81122537, 10.        ],
           [ 2.12700289,  2.43114174, 10.        ]])

    The numpy array represents this data:

    +-------------+-------------+------------+
    | Objective 1 | Objective 2 | Set Number |
    +-------------+-------------+------------+
    | 8.07559653  | 2.40702554  | 1.0        |
    +-------------+-------------+------------+
    | 8.66094446  | 3.64050144  | 1.0        |
    +-------------+-------------+------------+
    | etc.        | etc.        | etc.       |
    +-------------+-------------+------------+

    It is also possible to read data from a string:

    >>> from io import StringIO
    >>> fh = StringIO("0.5 0.5\\n\\n1 0\\n0 1\\n\\n0.5 0.5")
    >>> moocore.read_datasets(fh)
    array([[0.5, 0.5, 1. ],
           [1. , 0. , 2. ],
           [0. , 1. , 2. ],
           [0.5, 0.5, 3. ]])

    """  # noqa: D301
    if isinstance(filename, os.PathLike):
        filename = os.fspath(filename)
    elif isinstance(filename, StringIO):
        with tempfile.NamedTemporaryFile(mode="wt", delete=False) as fdst:
            shutil.copyfileobj(filename, fdst)
        # FIXME: Avoid recursion
        return read_datasets(fdst.name)
    else:
        filename = os.path.expanduser(filename)

    if not os.path.isfile(filename):
        raise FileNotFoundError(f"file '{filename}' not found")

    if filename.endswith(".xz"):
        with lzma.open(filename, "rb") as fsrc:
            with tempfile.NamedTemporaryFile(delete=False) as fdst:
                shutil.copyfileobj(fsrc, fdst)
        filename = fdst.name
    else:
        fdst = None

    # Encode filename to a binary string
    filename = filename.encode("utf-8")
    # Create return pointers for function
    data_p = ffi.new("double **")
    ncols_p = ffi.new("int *")
    datasize_p = ffi.new("int *")
    err_code = lib.read_datasets(filename, data_p, ncols_p, datasize_p)
    if fdst:
        os.remove(fdst.name)
    if err_code != 0:
        raise ReadDatasetsError(err_code)

    # Create buffer with the correct array size in bytes
    data_buf = ffi.buffer(data_p[0], datasize_p[0])
    # Convert 1d numpy array to 2d array with (n obj... , sets) columns
    return np.frombuffer(data_buf).reshape((-1, ncols_p[0]))


def _parse_maximise(maximise, nobj):
    # Converts maximise array or single bool to ndarray format
    return atleast_1d_of_length_n(maximise, nobj).astype(bool)


def _unary_refset_common(data, ref, maximise):
    # Convert to numpy.array in case the user provides a list.  We use
    # np.asarray(dtype=float) to convert it to floating-point, otherwise if a user inputs
    # something like ref = np.array([10, 10]) then numpy would interpret it as
    # an int array.
    data = np.asarray(data, dtype=float)
    ref = np.atleast_2d(np.asarray(ref, dtype=float))
    nobj = data.shape[1]
    if nobj != ref.shape[1]:
        raise ValueError(
            f"data and ref need to have the same number of columns ({nobj} != {ref.shape[1]})"
        )
    maximise = _parse_maximise(maximise, nobj)
    data_p, npoints, nobj = np2d_to_double_array(data)
    ref_p, ref_size = np1d_to_double_array(ref)
    maximise_p = ffi.from_buffer("bool []", maximise)
    return data_p, nobj, npoints, ref_p, ref_size, maximise_p


def igd(data, /, ref, *, maximise: bool | list[bool] = False) -> float:
    """Inverted Generational Distance (IGD).

    .. seealso:: For details about parameters, return value and examples, see :func:`igd_plus`.  For details of the calculation, see :ref:`igd_hausdorf`.

    Returns
    -------
        A single numerical value

    """
    data_p, nobj, npoints, ref_p, ref_size, maximise_p = _unary_refset_common(
        data, ref, maximise
    )
    return lib.IGD(data_p, nobj, npoints, ref_p, ref_size, maximise_p)


def igd_plus(
    data: ArrayLike, /, ref: ArrayLike, *, maximise: bool | list[bool] = False
) -> float:
    """Modified IGD (IGD+).

    IGD+ is a Pareto-compliant version of :func:`igd`.

    .. seealso:: For details of the calculation, see :ref:`igd_hausdorf`.

    Parameters
    ----------
    data :
        2D matrix of numerical values, where each row gives the coordinates of a point in objective space.
        If the array is created from the :func:`read_datasets` function, remove the last (set) column.

    ref :
        Reference point (1D vector). Must have same length as the number of column in ``data``.

    maximise :
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
        Also accepts a 1d numpy array with value 0/1 for each objective.

    Returns
    -------
        A single numerical value

    Examples
    --------
    >>> dat = np.array([[3.5, 5.5], [3.6, 4.1], [4.1, 3.2], [5.5, 1.5]])
    >>> ref = np.array([[1, 6], [2, 5], [3, 4], [4, 3], [5, 2], [6, 1]])
    >>> moocore.igd(dat, ref=ref)
    1.0627908666722465

    >>> moocore.igd_plus(dat, ref=ref)
    0.9855036468106652

    >>> moocore.avg_hausdorff_dist(dat, ref)
    1.0627908666722465

    """  # noqa: D401
    data_p, nobj, npoints, ref_p, ref_size, maximise_p = _unary_refset_common(
        data, ref, maximise
    )
    return lib.IGD_plus(data_p, nobj, npoints, ref_p, ref_size, maximise_p)


def avg_hausdorff_dist(data, /, ref, *, maximise=False, p: float = 1) -> float:  # noqa: D417
    """Average Hausdorff distance.

    .. seealso:: For details about parameters, return value and examples, see :func:`igd_plus`.  For details of the calculation, see :ref:`igd_hausdorf`.

    Parameters
    ----------
    p :
        Hausdorff distance parameter. Must be larger than 0.

    Returns
    -------
        A single numerical value

    """
    if p <= 0:
        raise ValueError("'p' must be larger than zero")

    data_p, nobj, npoints, ref_p, ref_size, maximise_p = _unary_refset_common(
        data, ref, maximise
    )
    p = ffi.cast("unsigned int", p)
    return lib.avg_Hausdorff_dist(
        data_p, nobj, npoints, ref_p, ref_size, maximise_p, p
    )


def epsilon_additive(
    data: ArrayLike, /, ref: ArrayLike, *, maximise: bool | list[bool] = False
) -> float:
    """Additive epsilon metric.

    ``data`` and ``ref`` must all be larger than 0 for :func:`epsilon_mult`.

    .. seealso:: For details of the calculation, see :ref:`epsilon_metric`.

    Parameters
    ----------
    data :
        Numpy array of numerical values, where each row gives the coordinates of a point in objective space.
        If the array is created from the :func:`read_datasets` function, remove the last (set) column
    ref :
        Reference point set as a numpy array or list. Must have same number of columns as a single point in the \
        dataset
    maximise :
        Whether the objectives must be maximised instead of minimised. \
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective. \
        Also accepts a 1d numpy array with value 0/1 for each objective

    Returns
    -------
        A single numerical value

    Examples
    --------
    >>> dat = np.array([[3.5,5.5], [3.6,4.1], [4.1,3.2], [5.5,1.5]])
    >>> ref = np.array([[1, 6], [2,5], [3,4], [4,3], [5,2], [6,1]])
    >>> moocore.epsilon_additive(dat, ref = ref)
    2.5

    >>> moocore.epsilon_mult(dat, ref = ref)
    3.5

    """
    data_p, nobj, npoints, ref_p, ref_size, maximise_p = _unary_refset_common(
        data, ref, maximise
    )
    return lib.epsilon_additive(
        data_p, nobj, npoints, ref_p, ref_size, maximise_p
    )


def epsilon_mult(data, /, ref, *, maximise=False) -> float:
    """Multiplicative epsilon metric.

    .. seealso:: For details about parameters, return value and examples, see :func:`epsilon_additive`.  For details of the calculation, see :ref:`epsilon_metric`.

    Returns
    -------
        A single numerical value

    """
    data_p, nobj, npoints, ref_p, ref_size, maximise_p = _unary_refset_common(
        data, ref, maximise
    )
    return lib.epsilon_mult(data_p, nobj, npoints, ref_p, ref_size, maximise_p)


def hypervolume(
    data: ArrayLike, /, ref: ArrayLike, maximise: bool | list[bool] = False
) -> float:
    r"""Hypervolume indicator.

    Compute the hypervolume metric with respect to a given reference point
    assuming minimization of all objectives. For 2D and 3D, the algorithm used
    :footcite:p:`FonPaqLop06:hypervolume,BeuFonLopPaqVah09:tec` has :math:`O(n
    \log n)` complexity. For 4D or higher, the algorithm
    :footcite:p:`FonPaqLop06:hypervolume` has :math:`O(n^{d-2} \log n)` time
    and linear space complexity in the worst-case, but experimental results
    show that the pruning techniques used may reduce the time complexity even
    further.

    Parameters
    ----------
    data :
        Numpy array of numerical values, where each row gives the coordinates of a point.
        If the array is created from the :func:`read_datasets` function, remove the last column.
    ref :
        Reference point as a 1D vector. Must be same length as a single point in the ``data``.
    maximise :
        Whether the objectives must be maximised instead of minimised. \
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective. \
        Also accepts a 1d numpy array with value 0/1 for each objective

    Returns
    -------
        A single numerical value, the hypervolume indicator

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> dat = np.array([[5, 5], [4, 6], [2, 7], [7, 4]])
    >>> moocore.hypervolume(dat, ref=[10, 10])
    38.0

    Merge all the sets of a dataset by removing the set number column:

    >>> dat = moocore.get_dataset("input1.dat")[:, :-1]
    >>> len(dat)
    100

    Dominated points are ignored, so this:

    >>> moocore.hypervolume(dat, ref=[10, 10])
    93.55331425585321

    gives the same hypervolume as this:

    >>> dat = moocore.filter_dominated(dat)
    >>> len(dat)
    6
    >>> moocore.hypervolume(dat, ref=[10, 10])
    93.55331425585321

    """
    # Convert to numpy.array in case the user provides a list.  We use
    # np.asarray to convert it to floating-point, otherwise if a user inputs
    # something like ref = np.array([10, 10]) then numpy would interpret it as
    # an int array.
    data, data_copied = asarray_maybe_copy(data)
    nobj = data.shape[1]
    ref = atleast_1d_of_length_n(np.array(ref, dtype=float), nobj)
    if nobj != ref.shape[0]:
        raise ValueError(
            f"data and ref need to have the same number of objectives ({nobj} != {ref.shape[0]})"
        )

    maximise = _parse_maximise(maximise, nobj)
    # FIXME: Do this in C.
    if maximise.any():
        if not data_copied:
            data = data.copy()
        data[:, maximise] = -data[:, maximise]
        ref = ref.copy()
        ref[maximise] = -ref[maximise]

    data_p, npoints, nobj = np2d_to_double_array(data)
    ref_buf = ffi.from_buffer("double []", ref)
    hv = lib.fpli_hv(data_p, nobj, npoints, ref_buf)
    return hv


def hv_approx(
    data: ArrayLike,
    /,
    ref: ArrayLike,
    maximise: bool | list[bool] = False,
    nsamples: int = 100000,
    seed=None,
    method: Literal["DZ2019"] = "DZ2019",
) -> float:
    r"""Approximate the hypervolume indicator.

    Approximate the value of the hypervolume metric with respect to a given
    reference point assuming minimization of all objectives. The default
    ``method="DZ2019"`` relies on Monte-Carlo sampling
    :footcite:p:`DenZha2019approxhv` and, thus, it gets more accurate, but
    slower, for higher values of ``nsamples``.

    Parameters
    ----------
    data :
        Numpy array of numerical values, where each row gives the coordinates of a point.
        If the array is created from the :func:`read_datasets` function, remove the last column.
    ref :
        Reference point as a 1D vector. Must be same length as a single point in the ``data``.
    maximise :
        Whether the objectives must be maximised instead of minimised. \
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective. \
        Also accepts a 1d numpy array with value 0/1 for each objective

    nsamples :
        Number of samples for Monte-Carlo sampling. Higher values give more accurate approximation of the true hypervolume but require more time.

    seed : int or numpy.random.Generator
        Either an integer to seed the NumPy random number generator (RNG) or an instance of Numpy-compatible RNG. ``None`` uses the default RNG of Numpy.

    method :
        Method to approximate the hypervolume.

    Returns
    -------
        A single numerical value, the approximate hypervolume indicator

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> x = np.array([[5, 5], [4, 6], [2, 7], [7, 4]])
    >>> moocore.hv_approx(x, ref=[10, 10], seed = 42)
    37.95471
    >>> moocore.hypervolume(x, ref=[10, 10])
    38.0

    Merge all the sets of a dataset by removing the set number column:

    >>> x = moocore.get_dataset("input1.dat")[:, :-1]

    Dominated points are ignored, so this:

    >>> moocore.hv_approx(x, ref=10, seed = 42)
    93.348976559100

    gives the same hypervolume approximation as this:

    >>> x = moocore.filter_dominated(x)
    >>> moocore.hv_approx(x, ref=10, seed = 42)
    93.348976559100

    The approximation is far from perfect for large sets:

    >>> x = moocore.get_dataset("CPFs.txt")[:,:-1]
    >>> x = moocore.filter_dominated(-x, maximise = True)
    >>> x = moocore.normalise(x, to_range = [1,2])
    >>> reference = 0.9
    >>> moocore.hypervolume(x, ref = reference, maximise = True)
    1.0570447464301551
    >>> moocore.hv_approx(x, ref = reference, maximise = True, seed = 42)
    1.056312559097445

    .. minigallery:: moocore.hv_approx
       :add-heading:

    """
    # Convert to numpy.array in case the user provides a list.  We use
    # np.asarray to convert it to floating-point, otherwise if a user inputs
    # something like ref = np.array([10, 10]) then numpy would interpret it as
    # an int array.
    data = np.asarray(data, dtype=float)
    nobj = data.shape[1]
    ref = atleast_1d_of_length_n(np.array(ref, dtype=float), nobj)
    if nobj != ref.shape[0]:
        raise ValueError(
            f"data and ref need to have the same number of objectives ({nobj} != {ref.shape[0]})"
        )

    if not is_integer_value(nsamples):
        raise ValueError(f"nsamples must be an integer value: {nsamples}")

    if seed is None or is_integer_value(seed):
        seed = np.random.default_rng(seed)

    # FIXME: Do this in C.
    data = ref - data
    maximise = _parse_maximise(maximise, nobj)
    if maximise.any():
        data *= np.where(maximise, -1, 1)

    # Equivalent to max(0, y - ref), i.e., ignore points that do not strictly
    # dominate ref.
    data = data[(data > 0).all(axis=1), :]

    expected = np.empty(nsamples)
    # We could do it without the loop but it will consume lots of memory.
    #  y / W[:, np.newaxis,:]
    # or we could use np.apply_along_axis()
    for k in range(nsamples):
        # Sample in the positive orthant of the hyper-sphere.
        w = np.abs(seed.normal(size=nobj))
        w /= np.linalg.norm(w)
        s_w = (data / w).min(axis=1)
        expected[k] = s_w.max()

    expected = (expected**nobj).mean()
    c_m = (np.pi ** (nobj / 2)) / ((2**nobj) * gamma_function(nobj / 2 + 1))
    return float(c_m * expected)


def is_nondominated(
    data: ArrayLike,
    maximise: bool | list[bool] = False,
    keep_weakly: bool = False,
) -> np.ndarray:
    r"""Identify dominated points according to Pareto optimality.

    For two dimensions, the algorithm has complexity :math:`O(n \log n)`.

    Parameters
    ----------
    data :
        Array of numerical values, where each row gives the coordinates of a point in objective space.
        If the array is created by the :func:`read_datasets()` function, remove the last column.
    maximise :
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of boolean values, with one value per objective.
        Also accepts a 1D numpy array with value 0/1 for each objective.
    keep_weakly:
        If ``False``, return ``False`` for any duplicates of nondominated points.

    Returns
    -------
        Returns a boolean array of the same length as the number of rows of data,
        where ``True`` means that the point is not dominated by any other point.

    See Also
    --------
    filter_dominated : to filter out dominated points.

    pareto_rank : to rank points according to Pareto dominance.


    Examples
    --------
    >>> S = np.array([[1, 1], [0, 1], [1, 0], [1, 0]])
    >>> moocore.is_nondominated(S)
    array([False,  True,  True, False])
    >>> moocore.filter_dominated(S)
    array([[0, 1],
           [1, 0]])
    >>> moocore.is_nondominated(S, keep_weakly=True)
    array([False,  True,  True,  True])
    >>> moocore.filter_dominated(S, keep_weakly=True)
    array([[0, 1],
           [1, 0],
           [1, 0]])

    """
    data = np.asarray(data, dtype=float)
    nrows, nobj = data.shape
    maximise = _parse_maximise(maximise, nobj)
    data_p, npoints, nobj = np2d_to_double_array(data)
    maximise_p = ffi.from_buffer("bool []", maximise)
    keep_weakly = ffi.cast("bool", bool(keep_weakly))
    nondom = lib.is_nondominated(
        data_p, nobj, npoints, maximise_p, keep_weakly
    )
    nondom = ffi.buffer(nondom, nrows)
    return np.frombuffer(nondom, dtype=bool)


def filter_dominated(
    data, /, *, maximise: bool | list[bool] = False, keep_weakly: bool = False
) -> np.ndarray:
    """Remove dominated points according to Pareto optimality.

    .. seealso:: For details about parameters and examples, see :func:`is_nondominated`.

    Returns
    -------
        Returns the rows of ``data`` where :func:`is_nondominated` is ``True``.

    """
    return data[is_nondominated(data, maximise, keep_weakly)]


def filter_dominated_within_sets(
    data, /, *, maximise: bool | list[bool] = False, keep_weakly: bool = False
):
    """Given a dataset with multiple sets (last column gives the set index), filter dominated points within each set.

    Executes the :func:`filter_dominated` function within each set in a dataset \
    and returns back a dataset. This is roughly equivalent to partitioning 'data' according to the last column,
    filtering dominated solutions within each partition, and joining back the result.

    Parameters
    ----------
    data : numpy array
        Numpy array of numerical values and set numbers, containing multiple sets. For example the output \
         of the :func:`read_datasets` function
    maximise : single bool, or list of booleans
        Whether the objectives must be maximised instead of minimised. \
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective. \
        Also accepts a 1D numpy array with values 0 or 1 for each objective
    keep_weakly :
        If ``False``, return ``False`` for any duplicates of nondominated points.

    Returns
    -------
    numpy array
        A numpy array where each set only contains nondominated points with respect to the set  (last column is the set index).
        Points from one set can still dominated points from another set.

    Examples
    --------
    >>> x = moocore.get_dataset("input1.dat")
    >>> pf_per_set = moocore.filter_dominated_within_sets(x)
    >>> len(pf_per_set)
    42
    >>> pf = moocore.filter_dominated(x[:, :-1])
    >>> len(pf)
    6
    >>> pf
    array([[0.20816431, 4.62275469],
           [0.22997367, 1.11772205],
           [0.58799475, 0.73891181],
           [1.54506255, 0.38303122],
           [0.17470556, 8.89066343],
           [8.57911868, 0.35169752]])

    See Also
    --------
    filter_dominated : to be used with a single dataset.

    """
    data = np.asarray(data, dtype=float)
    ncols = data.shape[1]
    if ncols < 3:
        raise ValueError(
            "'data' must have at least 3 columns (2 objectives + set column)"
        )
    _, uniq_index = np.unique(data[:, -1], return_index=True)
    # FIXME: Is there a more efficient way to do this that just creates views and not copies?
    x_split = np.vsplit(data[:, :-1], uniq_index[1:])
    is_nondom = np.concatenate(
        [
            is_nondominated(g, maximise=maximise, keep_weakly=keep_weakly)
            for g in x_split
        ],
        dtype=bool,
        casting="no",
    )
    return data[is_nondom, :]


def pareto_rank(
    data: ArrayLike, /, *, maximise: bool | list[bool] = False
) -> np.ndarray:
    r"""Rank points according to Pareto-optimality (nondominated sorting).

    The function :func:`pareto_rank` is meant to be used like
    :func:`numpy.argsort`, but it assigns indexes according to Pareto
    dominance. Duplicated points are kept on the same front. The resulting
    ranking can be used to partition points into different lists or arrays,
    each of them being mutually nondominated :footcite:p:`Deb02nsga2`.

    With 2 dimensions, the code uses the :math:`O(n \log n)` algorithm by
    :footcite:t:`Jen03`.

    Parameters
    ----------
    data :
        Numpy array of numerical values, where each row gives the coordinates of a point in objective space.
        If the array is created from the :func:`read_datasets()` function, remove the last column.
    maximise :
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of boolean values, with one value per objective.
        Also accepts a 1d numpy array with value 0/1 for each objective.

    Returns
    -------
        An integer vector of the same length as the number of rows of ``data``, where each value gives the Pareto rank of each point (lower is better).

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> x = moocore.get_dataset("input1.dat")[:, :2]
    >>> ranks = moocore.pareto_rank(x)
    >>> ranks
    array([ 5,  9,  1, 12,  1,  4,  8,  2,  4,  1,  9,  5,  6,  5, 12,  5,  5,
            6,  8,  4,  9, 13,  9, 10,  6, 11,  7,  3,  8,  4, 11,  8,  3,  6,
            3,  8,  2,  3, 10,  1, 12,  7,  8, 11, 14,  4,  7,  4,  1, 10, 10,
            1,  3, 14,  2,  7,  8,  7,  7, 11,  5, 14,  7,  9, 13, 14,  5,  9,
            6,  2, 13, 11,  4,  9, 10,  7,  8,  7,  7, 10,  6,  3,  4,  5,  8,
            4,  9,  4,  3, 11,  2,  3, 13,  2,  3, 10,  5,  3,  6,  3],
          dtype=int32)

    We can now split the original set into a list of nondominated sets ordered by Pareto rank:

    >>> paretos = [x.compress((g == ranks), axis=0) for g in np.unique(ranks)]
    >>> len(paretos)
    14

    The first element is the set of points not dominated by anything else:

    >>> np.array_equal(paretos[0], moocore.filter_dominated(x))
    True

    """
    data, data_copied = asarray_maybe_copy(data)
    nrows, nobj = data.shape
    maximise = _parse_maximise(maximise, nobj)
    if maximise.any():
        # FIXME: Do this in C.
        if not data_copied:
            data = data.copy()
        data[:, maximise] = -data[:, maximise]

    data_p, npoints, nobj = np2d_to_double_array(data)
    ranks = lib.pareto_rank(data_p, nobj, npoints)
    ranks = ffi.buffer(ranks, nrows * ffi.sizeof("int"))
    ranks = np.frombuffer(ranks, dtype=np.intc())
    assert len(ranks) == nrows
    return ranks


# def filter_dominated_sets(dataset, maximise=False, keep_weakly=False):
#     """Filter dominated sets for multiple sets

#     Executes the :func:`filter_dominated` function for every set in a dataset \
#     and returns back a dataset, preserving set

#     Examples
#     --------
#     >>> dataset = moocore.read_datasets("./doc/data/input1.dat")
#     >>> subset = moocore.subset(dataset, range = [3,5])
#     >>> moocore.filter_dominated_sets(subset)
#     array([[2.60764118, 6.31309852, 3.        ],
#            [3.22509709, 6.1522834 , 3.        ],
#            [0.37731545, 9.02211752, 3.        ],
#            [4.61023932, 2.29231998, 3.        ],
#            [0.2901393 , 8.32259412, 4.        ],
#            [1.54506255, 0.38303122, 4.        ],
#            [4.43498452, 4.13150648, 5.        ],
#            [9.78758589, 1.41238277, 5.        ],
#            [7.85344142, 3.02219054, 5.        ],
#            [0.9017068 , 7.49376946, 5.        ],
#            [0.17470556, 8.89066343, 5.        ]])

#     The above returns sets 3,4,5 with dominated points within each set removed.

#     See Also
#     --------
#     This function for data without set numbers - :func:`filter_dominated`
#     """
#     # FIXME: it will be faster to stack filter_set, then do:
#     # dataset[filter_set, :]
#     # to filter in one go.
#     new_sets = []
#     for set in np.unique(dataset[:, -1]):
#         set_data = dataset[dataset[:, -1] == set, :-1]
#         filter_set = filter_dominated(set_data, maximise, keep_weakly)
#         set_nums = np.full(filter_set.shape[0], set).reshape(-1, 1)
#         new_set = np.hstack((filter_set, set_nums))
#         new_sets.append(new_set)
#     return np.vstack(new_sets)


def normalise(
    data: ArrayLike,
    /,
    to_range: ArrayLike = [0.0, 1.0],
    *,
    lower: ArrayLike = np.nan,
    upper: ArrayLike = np.nan,
    maximise=False,
) -> np.ndarray:
    """Normalise points per coordinate to a range, e.g., ``to_range = [1,2]``, where the minimum value will correspond to 1 and the maximum to 2.

    Parameters
    ----------
    data :
        Numpy array of numerical values, where each row gives the coordinates of a point in objective space.
        See :func:`normalise_sets` to normalise data that includes set numbers (Multiple sets)

    to_range :
        Range composed of two numerical values. Normalise values to this range. If the objective is maximised, it is normalised to ``(to_range[1], to_range[0])`` instead.

    upper, lower:
        Bounds on the values. If :data:`numpy.nan`, the maximum and minimum values of each coordinate are used.

    maximise : single bool, or list of booleans
        Whether the objectives must be maximised instead of minimised. \
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective. \
        Also accepts a 1D numpy array with values 0 or 1 for each objective

    Returns
    -------
        Returns the data normalised as requested.

    Examples
    --------
    >>> dat = np.array([[3.5,5.5], [3.6,4.1], [4.1,3.2], [5.5,1.5]])
    >>> moocore.normalise(dat)
    array([[0.   , 1.   ],
           [0.05 , 0.65 ],
           [0.3  , 0.425],
           [1.   , 0.   ]])

    >>> moocore.normalise(dat, to_range = [1,2], lower = [3.5, 3.5], upper = 5.5)
    array([[1.  , 2.  ],
           [1.05, 1.3 ],
           [1.3 , 0.85],
           [2.  , 0.  ]])

    """
    # normalise() modifies the data, so we need to create a copy.
    data = np.array(data, dtype=float)
    npoints, nobj = data.shape
    if nobj == 1:
        raise ValueError("'data' must have at least two columns")
    to_range = np.asarray(to_range, dtype=float)
    if to_range.shape[0] != 2:
        raise ValueError("'to_range' must have length 2")
    lower = atleast_1d_of_length_n(np.asarray(lower, dtype=float), nobj)
    upper = atleast_1d_of_length_n(np.asarray(upper, dtype=float), nobj)
    if np.any(np.isnan(lower)):
        lower = np.where(np.isnan(lower), data.min(axis=0), lower)
    if np.any(np.isnan(upper)):
        upper = np.where(np.isnan(upper), data.max(axis=0), upper)

    maximise = _parse_maximise(maximise, data.shape[1])
    data_p, npoints, nobj = np2d_to_double_array(data)
    maximise_p = ffi.from_buffer("bool []", maximise)
    lbound_p = ffi.from_buffer("double []", lower)
    ubound_p = ffi.from_buffer("double []", upper)
    lib.agree_normalise(
        data_p,
        nobj,
        npoints,
        maximise_p,
        to_range[0],
        to_range[1],
        lbound_p,
        ubound_p,
    )
    # We can return data directly because we only changed the data, not the
    # shape.
    return data


# def normalise_sets(dataset, range=[0, 1], lower="na", upper="na", maximise=False):
#     """Normalise dataset with multiple sets

#     Executes the :func:`normalise` function for every set in a dataset (Performs normalise on every set separately)

#     Examples
#     --------
#     >>> dataset = moocore.read_datasets("./doc/data/input1.dat")
#     >>> subset = moocore.subset(dataset, range = [4,5])
#     >>> moocore.normalise_sets(subset)
#     array([[1.        , 0.38191742, 4.        ],
#            [0.70069111, 0.5114669 , 4.        ],
#            [0.12957487, 0.29411141, 4.        ],
#            [0.28059067, 0.53580626, 4.        ],
#            [0.32210885, 0.21797067, 4.        ],
#            [0.39161668, 0.92106178, 4.        ],
#            [0.        , 1.        , 4.        ],
#            [0.62293227, 0.11315216, 4.        ],
#            [0.76936124, 0.58159784, 4.        ],
#            [0.12957384, 0.        , 4.        ],
#            [0.82581672, 0.66566917, 5.        ],
#            [0.44318444, 0.35888982, 5.        ],
#            [0.80036477, 0.23242446, 5.        ],
#            [0.88550836, 0.51482968, 5.        ],
#            [0.89293026, 1.        , 5.        ],
#            [1.        , 0.        , 5.        ],
#            [0.79879657, 0.21247419, 5.        ],
#            [0.07562783, 0.80266586, 5.        ],
#            [0.        , 0.98703813, 5.        ],
#            [0.6229605 , 0.8613516 , 5.        ]])

#     See Also
#     --------
#     This function for data without set numbers - :func:`normalise`
#     """
#     for set in np.unique(dataset[:, -1]):
#         setdata = dataset[dataset[:, -1] == set, :-1]
#         dataset[dataset[:, -1] == set, :-1] = normalise(
#             setdata, to_range=range, lower=np.nan, upper=np.nan, maximise=False
#         )
#     return dataset


def eaf(data, /, percentiles: list = []):
    """Compute the Empirical attainment function (EAF) in 2D or 3D.

    Parameters
    ----------
    data : numpy array
        Numpy array of numerical values and set numbers, containing multiple sets. For example the output \
         of the :func:`read_datasets` function
    percentiles :
        List indicating which percentiles are computed. By default, all possible percentiles are calculated.

    Returns
    -------
    numpy array
        Returns a numpy array containing the EAF data points, with the same number of columns as the input argument, \
        but a different number of rows. The last column represents the EAF percentile for that data point

    Examples
    --------
    >>> x = moocore.get_dataset("input1.dat")
    >>> moocore.eaf(x)                                     # doctest: +ELLIPSIS
    array([[  0.17470556,   8.89066343,  10.        ],
           [  0.20816431,   4.62275469,  10.        ],
           [  0.22997367,   1.11772205,  10.        ],
           [  0.58799475,   0.73891181,  10.        ],
           [  1.54506255,   0.38303122,  10.        ],
           [  8.57911868,   0.35169752,  10.        ],
           [  0.20816431,   8.89066343,  20.        ],
           [  0.2901393 ,   8.32259412,  20.        ],
           ...
           [  9.78758589,   2.8124162 ,  90.        ],
           [  1.13096306,   9.72645436, 100.        ],
           [  2.71891214,   8.84691923, 100.        ],
           [  3.34035397,   7.49376946, 100.        ],
           [  4.43498452,   6.94327481, 100.        ],
           [  4.96525837,   6.20957074, 100.        ],
           [  7.92511295,   3.92669598, 100.        ]])

    >>> moocore.eaf(x, percentiles = [0, 50, 100])         # doctest: +ELLIPSIS
    array([[  0.17470556,   8.89066343,   0.        ],
           [  0.20816431,   4.62275469,   0.        ],
           [  0.22997367,   1.11772205,   0.        ],
           [  0.58799475,   0.73891181,   0.        ],
           [  1.54506255,   0.38303122,   0.        ],
           [  8.57911868,   0.35169752,   0.        ],
           [  0.53173087,   9.73244829,  50.        ],
           [  0.62230271,   9.02211752,  50.        ],
           [  0.79293574,   8.89066343,  50.        ],
           [  0.9017068 ,   8.32259412,  50.        ],
           [  0.97468676,   7.65893644,  50.        ],
           [  1.06855707,   7.49376946,  50.        ],
           [  1.54506255,   6.7102429 ,  50.        ],
           [  1.5964888 ,   5.98825094,  50.        ],
           [  2.16315952,   4.7394435 ,  50.        ],
           [  2.85891341,   4.49240941,  50.        ],
           [  3.34035397,   2.89377444,  50.        ],
           [  4.61023932,   2.87955367,  50.        ],
           [  4.96525837,   2.29231998,  50.        ],
           [  7.04694467,   1.83484358,  50.        ],
           [  9.7398055 ,   1.00153569,  50.        ],
           [  1.13096306,   9.72645436, 100.        ],
           [  2.71891214,   8.84691923, 100.        ],
           [  3.34035397,   7.49376946, 100.        ],
           [  4.43498452,   6.94327481, 100.        ],
           [  4.96525837,   6.20957074, 100.        ],
           [  7.92511295,   3.92669598, 100.        ]])

    """
    data = np.asarray(data, dtype=float)
    ncols = data.shape[1]
    if ncols < 3:
        raise ValueError(
            "'data' must have at least 3 columns (2 objectives + set column)"
        )
    if ncols > 4:
        raise NotImplementedError(
            "Only 2D or 3D datasets are currently supported for computing the EAF"
        )

    _, cumsizes = np.unique(data[:, -1], return_counts=True)
    nsets = len(cumsizes)
    cumsizes = np.cumsum(cumsizes)
    cumsizes_p, ncumsizes = np1d_to_int_array(cumsizes)
    percentiles = np.atleast_1d(percentiles)
    if len(percentiles) == 0:
        percentiles = np.arange(1.0, nsets + 1) * (100.0 / nsets)
    else:
        percentiles = np.unique(np.asarray(percentiles, dtype=float))
    percentile_p, npercentiles = np1d_to_double_array(percentiles)

    # Get C pointers + matrix size for calling CFFI generated extension module
    data_p, npoints, nobj = np2d_to_double_array(data[:, :-1])
    eaf_npoints = ffi.new("int *")
    eaf_data_p = lib.eaf_compute_matrix(
        eaf_npoints,
        data_p,
        nobj,
        cumsizes_p,
        ncumsizes,
        percentile_p,
        npercentiles,
    )
    eaf_npoints = eaf_npoints[0]
    eaf_buf = ffi.buffer(
        eaf_data_p, ffi.sizeof("double") * eaf_npoints * ncols
    )
    return np.frombuffer(eaf_buf).reshape((eaf_npoints, -1))


def vorobT(data: ArrayLike, /, ref: ArrayLike) -> dict:
    """Compute Vorob'ev threshold and expectation.

    Parameters
    ----------
    data :
        Numpy array of numerical values and set numbers, containing multiple sets. For example the output \
         of the :func:`read_datasets` function
    ref :
        Reference point set as a numpy array or list. Must be same length as a single point in the \
        dataset

    Returns
    -------
        A dictionary with elements `threshold`, `VE`, and `avg_hyp` (average hypervolume).

    See Also
    --------
    vorobDev : Compute Vorob'ev deviation.

    Notes
    -----
    For more background, see :footcite:t:`BinGinRou2015gaupar,Molchanov2005theory,CheGinBecMol2013moda`.

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> CPFs = moocore.get_dataset("CPFs.txt")
    >>> res = moocore.vorobT(CPFs, ref = (2, 200))
    >>> res['threshold']
    44.140625
    >>> res['avg_hyp']
    8943.333191728081

    """
    data = np.asarray(data, dtype=float)
    ncols = data.shape[1]
    if ncols < 3:
        raise ValueError(
            "'data' must have at least 3 columns (2 objectives + set column)"
        )
    nobj = ncols - 1
    sets = data[:, -1]
    uniq_sets = np.unique(sets)
    avg_hyp = np.mean(
        [hypervolume(data[sets == k, :-1], ref=ref) for k in uniq_sets]
    )
    prev_hyp = diff = np.inf  # hypervolume of quantile at previous step
    a = 0
    b = 100
    while diff != 0:
        c = (a + b) / 2.0
        eaf_res = eaf(data, percentiles=c)[:, :nobj]
        tmp = hypervolume(eaf_res, ref=ref)
        if tmp > avg_hyp:
            a = c
        else:
            b = c
        diff = prev_hyp - tmp
        prev_hyp = tmp

    return dict(threshold=c, VE=eaf_res, avg_hyp=float(avg_hyp))


def vorobDev(
    x: ArrayLike, /, ref: ArrayLike, *, VE: ArrayLike = None
) -> float:
    r"""Compute Vorob'ev deviation.

    Parameters
    ----------
    x :
       Numpy array of numerical values and set numbers, containing multiple sets.
       For example the output of the :func:`read_datasets` function.
    ref : ArrayLike
       Reference point set as a numpy array or list. Must be same length as a single point in the dataset.
    VE :
       Vorob'ev expectation, e.g., as returned by :func:`vorobT`.
       If not provided, it is calculated as ``vorobT(x, ref)``.

    Returns
    -------
        Vorob'ev deviation.

    See Also
    --------
    vorobT : Compute Vorob'ev threshold and expectation.

    Notes
    -----
    For more background, see :footcite:t:`BinGinRou2015gaupar,Molchanov2005theory,CheGinBecMol2013moda`.

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> CPFs = moocore.get_dataset("CPFs.txt")
    >>> VD = moocore.vorobDev(CPFs, ref=(2, 200))
    >>> VD
    3017.12989402326

    """
    if VE is None:
        VE = vorobT(x, ref)["VE"]

    x = np.asarray(x, dtype=float)
    ncols = x.shape[1]
    if ncols < 3:
        raise ValueError(
            "'x' must have at least 3 columns (2 objectives + set column)"
        )

    # Hypervolume of the symmetric difference between A and B:
    # 2 * H(AUB) - H(A) - H(B)
    H2 = hypervolume(VE, ref=ref)
    _, uniq_index = np.unique(x[:, -1], return_index=True)
    x_split = np.vsplit(x[:, :-1], uniq_index[1:])
    H1 = np.fromiter(
        (hypervolume(g, ref=ref) for g in x_split),
        dtype=float,
        count=len(x_split),
    ).mean()
    VD = (
        np.fromiter(
            (hypervolume(np.vstack((g, VE)), ref=ref) for g in x_split),
            dtype=float,
            count=len(x_split),
        ).mean()
        * 2.0
    )
    return float(VD - H1 - H2)


def eafdiff(
    x, y, /, *, intervals: int = None, maximise=False, rectangles: bool = False
) -> np.ndarray:
    """Compute empirical attainment function (EAF) differences.

    Calculate the differences between the empirical attainment functions of two
    data sets.

    Parameters
    ----------
    x, y : ArrayLike
       Numpy matrices corresponding to the input data of left and right sides,
       respectively. Each data frame has at least three columns, the third one
       being the set of each point. See also :func:`read_datasets`.

    intervals :
       The absolute range of the differences :math:`[0, 1]` is partitioned into the number of intervals provided.

    maximise : bool or list of bool
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
        Also accepts a 1D numpy array with value 0/1 for each objective.

    rectangles :
       If ``True``, the output is in the form of rectangles of the same color.


    Returns
    -------
       With ``rectangles=False``, a matrix with three columns, The first two columns describe the points where there
       is a transition in the value of the EAF differences.  With
       ``rectangles=True``, a matrix with five columns, where the first 4 columns give the
       coordinates of two corners of each rectangle. In both
       cases, the last column gives the difference in terms of sets in ``x`` minus
       sets in ``y`` that attain each point (i.e., negative values are differences
       in favour ``y``).

    See Also
    --------
    moocore.read_datasets, mooplot.eafdiffplot


    Examples
    --------
    >>> from io import StringIO
    >>> A1 = moocore.read_datasets(
    ...     StringIO('''
    ... 3 2
    ... 2 3
    ...
    ... 2.5 1
    ... 1 2
    ...
    ... 1 2''')
    ... )
    >>> A2 = moocore.read_datasets(
    ...     StringIO('''
    ... 4 2.5
    ... 3 3
    ... 2.5 3.5
    ...
    ... 3 3
    ... 2.5 3.5
    ...
    ... 2 1''')
    ... )
    >>> moocore.eafdiff(A1, A2)
    array([[ 1. ,  2. ,  2. ],
           [ 2. ,  1. , -1. ],
           [ 2.5,  1. ,  0. ],
           [ 2. ,  2. ,  1. ],
           [ 2. ,  3. ,  2. ],
           [ 3. ,  2. ,  2. ],
           [ 2.5,  3.5,  0. ],
           [ 3. ,  3. ,  0. ],
           [ 4. ,  2.5,  1. ]])
    >>> moocore.eafdiff(A1, A2, rectangles=True)
    array([[ 2. ,  1. ,  2.5,  2. , -1. ],
           [ 1. ,  2. ,  2. ,  inf,  2. ],
           [ 2.5,  1. ,  inf,  2. ,  0. ],
           [ 2. ,  2. ,  3. ,  3. ,  1. ],
           [ 2. ,  3.5,  2.5,  inf,  2. ],
           [ 2. ,  3. ,  3. ,  3.5,  2. ],
           [ 3. ,  2.5,  4. ,  3. ,  2. ],
           [ 3. ,  2. ,  inf,  2.5,  2. ],
           [ 4. ,  2.5,  inf,  3. ,  1. ]])

    """
    x = np.asarray(x, dtype=float)
    y = np.asarray(y, dtype=float)
    assert (
        x.shape[1] == y.shape[1]
    ), "'x' and 'y' must have the same number of columns"
    nobj = x.shape[1] - 1
    assert nobj == 2
    maximise = _parse_maximise(maximise, nobj=nobj)
    # The C code expects points within a set to be contiguous.
    x = x[x[:, -1].argsort(), :]
    y = y[y[:, -1].argsort(), :]
    _, cumsizes_x = np.unique(x[:, -1], return_counts=True)
    _, cumsizes_y = np.unique(y[:, -1], return_counts=True)
    cumsizes_x = np.cumsum(cumsizes_x)
    cumsizes_y = np.cumsum(cumsizes_y)
    cumsizes = np.concatenate((cumsizes_x, cumsizes_x[-1] + cumsizes_y))
    nsets = len(cumsizes)

    data = np.vstack((x[:, :-1], y[:, :-1]))
    if maximise.any():
        data[:, maximise] = -data[:, maximise]

    if intervals is None:
        intervals = int(nsets / 2.0)
    else:
        assert is_integer_value(intervals)
        intervals = min(intervals, int(nsets / 2.0))

    data_p, _, nobj_int = np2d_to_double_array(data)
    cumsizes_p, nsets = np1d_to_int_array(cumsizes)
    eaf_npoints = ffi.new("int *")
    intervals = ffi.cast("int", intervals)

    if rectangles:
        eaf_data = lib.eafdiff_compute_rectangles(
            eaf_npoints, data_p, nobj_int, cumsizes_p, nsets, intervals
        )
        ncols = 2 * nobj + 1  # 2x2D points + color
    else:
        eaf_data = lib.eafdiff_compute_matrix(
            eaf_npoints, data_p, nobj_int, cumsizes_p, nsets, intervals
        )
        ncols = nobj + 1  # 2D points + color

    eaf_npoints = eaf_npoints[0]
    eaf_data = ffi.buffer(eaf_data, ffi.sizeof("double") * eaf_npoints * ncols)
    eaf_data = np.frombuffer(eaf_data).reshape((eaf_npoints, -1))
    # FIXME: We should remove duplicated rows in C code.
    eaf_data = unique_nosort(eaf_data, axis=0)
    # FIXME: Check that we do not generate duplicated nor overlapping
    # rectangles with different colors. That would be a bug.

    if maximise.any():
        if rectangles:
            maximise = np.concatenate((maximise, maximise))
        maximise = np.flatnonzero(
            maximise
        )  # Using bool directly misses the color column.
        eaf_data[:, maximise] = -eaf_data[:, maximise]

    return eaf_data


# def get_diff_eaf(x, y, intervals=None, debug=False):

#     if np.min(x[:, -1]) != 1 or np.min(y[:, -1]) != 1:
#         raise ValueError("x and y should contain set numbers starting from 1")
#     ycopy = np.copy(
#         y
#     )  # Do hard copy so that the matrix is not corrupted. This could be optimised
#     ycopy[:, -1] = ycopy[:, -1] + np.max(
#         x[:, -1]
#     )  # Make Y sets start from end of X sets

#     data = np.vstack((x, ycopy))  # Combine X and Y datasets to one matrix
#     nsets = len(np.unique(data[:, -1]))
#     if intervals is None:
#         intervals = nsets / 2.0
#     else:
#         intervals = min(intervals, nsets / 2.0)
#     intervals = int(intervals)

#     data = np.ascontiguousarray(
#         np.asarray(data, dtype=float)
#     )  # C function requires contiguous data
#     num_data_columns = data.shape[1]
#     data_p, npoints, ncols = np2d_to_double_array(data)
#     eaf_npoints = ffi.new("int *", 0)
#     sizeof_eaf = ffi.new("int *", 0)
#     nsets = ffi.cast("int", nsets)  # Get num of sets from data
#     intervals = ffi.cast("int", intervals)
#     debug = ffi.cast("bool", debug)
#     eaf_diff_data = lib.compute_eafdiff_(
#         data_p, ncols, npoints, nsets, intervals, eaf_npoints, sizeof_eaf, debug
#     )

#     eaf_buf = ffi.buffer(eaf_diff_data, sizeof_eaf[0])
#     eaf_arr = np.frombuffer(eaf_buf)
#     # The C code gets diff EAF in Column Major order so I return it in column major order than transpose to fix into row major order
#     return np.reshape(eaf_arr, (num_data_columns, -1)).T


def whv_rect(x, /, rectangles, *, ref, maximise=False) -> float:
    """Compute weighted hypervolume given a set of rectangles.

    .. seealso:: For details about parameters, return value and examples, see :func:`total_whv_rect`.

    .. warning::
        The current implementation only supports 2 objectives.

    Returns
    -------
        A single numerical value, the weighted hypervolume

    See Also
    --------
    total_whv_rect, whv_hype

    """
    x = np.asarray(x, dtype=float)
    nobj = x.shape[1]
    if nobj != 2:
        raise NotImplementedError("Only 2D datasets are currently supported")
    if rectangles.shape[1] != 5:
        raise ValueError(
            "Invalid number of columns in 'rectangles' (should be 5)"
        )

    ref = atleast_1d_of_length_n(np.asarray(ref, dtype=float), nobj)
    x, npoints, _ = np2d_to_double_array(x)
    ref = ffi.from_buffer("double []", ref)
    rectangles, rectangles_nrow, _ = np2d_to_double_array(rectangles)
    assert not maximise
    # FIXME: Move to C code.
    # maximise = _parse_maximise(maximise, nobj)
    # if maximise.any():
    #     x[:, maximise] = -x[:, maximise]
    #     ref[maximise] = -ref[maximise]
    #     if maximise.all():
    #         rectangles[:, :4] = -rectangles[:, :4]
    #     else:
    #         pos = np.flatnonzero(maximise) + [0,2]
    #         rectangles[:, pos] = -rectangles[:, pos]
    hv = lib.rect_weighted_hv2d(x, npoints, rectangles, rectangles_nrow, ref)
    return hv


def get_ideal(x, maximise):
    # FIXME: Is there a better way to do this?
    lower = x.min(axis=0)
    upper = x.max(axis=0)
    return np.where(maximise, upper, lower)


def total_whv_rect(
    x,
    /,
    rectangles,
    *,
    ref,
    maximise=False,
    ideal=None,
    scalefactor: float = 0.1,
) -> float:
    r"""Compute total weighted hypervolume given a set of rectangles.

    Calculates the hypervolume weighted by a set of rectangles (with zero
    weight outside the rectangles). The function :func:`total_whv_rect()`
    calculates the total weighted hypervolume as :func:`hypervolume()` ``+
    scalefactor * abs(prod(ref - ideal)) *`` :func:`whv_rect()`. The
    details of the computation are given by :footcite:t:`DiaLop2020ejor`.

    .. warning::
        The current implementation only supports 2 objectives.

    Parameters
    ----------
    x :
        Numpy array of numerical values, where each row gives the coordinates of a point.
        If the array is created from the :func:`read_datasets` function, remove the last column.

    rectangles :
        Weighted rectangles that will bias the computation of the hypervolume. Maybe generated by :func:`eafdiff()` with  ``rectangles=True`` or by :func:`choose_eafdiff()`.

    ref :
        Reference point as a 1D vector. Must be same length as a single row in  ``x``.

    maximise : bool or or list of bool
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
        Also accepts a 1D numpy array with values 0 or 1 for each objective.

    ideal :
        Ideal point as a vector of numerical values.  If ``None``, it is calculated as minimum (or maximum if maximising that objective) of each objective in the input data.

    scalefactor :
        Real value within :math:`(0,1]` that scales the overall weight of the differences. This is parameter psi (:math:`\psi`) in :footcite:t:`DiaLop2020ejor`.

    Returns
    -------
        A single numerical value, the weighted hypervolume

    See Also
    --------
    hypervolume, whv_rect, whv_hype

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> rectangles = np.array(
    ...     [
    ...         [1.0, 3.0, 2.0, np.inf, 1],
    ...         [2.0, 3.5, 2.5, np.inf, 2],
    ...         [2.0, 3.0, 3.0, 3.5, 3],
    ...     ]
    ... )
    >>> whv_rect([[2, 2]], rectangles, ref=6)
    4.0
    >>> whv_rect([[2, 1]], rectangles, ref=6)
    4.0
    >>> whv_rect([[1, 2]], rectangles, ref=6)
    7.0
    >>> total_whv_rect([[2, 2]], rectangles, ref=6, ideal=1)
    26.0
    >>> total_whv_rect([[2, 1]], rectangles, ref=6, ideal=1)
    30.0
    >>> total_whv_rect([[1, 2]], rectangles, ref=6, ideal=1)
    37.5

    """
    x = np.asarray(x, dtype=float)
    nobj = x.shape[1]
    if nobj != 2:
        raise NotImplementedError("Only 2D datasets are currently supported")
    if rectangles.shape[1] != 5:
        raise ValueError(
            "Invalid number of columns in 'rectangles' (should be 5)"
        )
    if scalefactor <= 0 or scalefactor > 1:
        raise ValueError("'scalefactor' must be within (0,1]")

    ref = atleast_1d_of_length_n(np.asarray(ref, dtype=float), nobj)
    whv = whv_rect(x, rectangles, ref=ref, maximise=maximise)
    hv = hypervolume(x, ref=ref)  # FIXME: maximise = maximise)
    if ideal is None:
        # FIXME: Should we include the range of the rectangles here?
        ideal = get_ideal(x, maximise=maximise)
    else:
        ideal = atleast_1d_of_length_n(np.asarray(ideal, dtype=float), nobj)

    beta = scalefactor * abs((ref - ideal).prod())
    return float(hv + beta * whv)


def whv_hype(
    data,
    /,
    *,
    ref,
    ideal,
    maximise=False,
    nsamples: int = 100000,
    dist: Literal["uniform", "point", "exponential"] = "uniform",
    seed=None,
    mu=None,
) -> float:
    r"""Approximation of the (weighted) hypervolume by Monte-Carlo sampling (2D only).

    Return an estimation of the hypervolume of the space dominated by the input
    data following the procedure described by :footcite:t:`AugBadBroZit2009gecco`. A weight
    distribution describing user preferences may be specified.

    .. warning::
        The current implementation only supports 2 objectives.

    Parameters
    ----------
    data : numpy.ndarray
        Numpy array of numerical values, where each row gives the coordinates of a point in objective space.
        If the array is created from the :func:`read_datasets` function, remove the last (set) column.

    ref : numpy.ndarray or list
        Reference point as a numpy array or list. Must have same length as the number of columns of the dataset.

    ideal : numpy.ndarray or list
        Ideal point as a numpy array or list. Must have same length as the number of columns of the dataset.

    maximise : bool or or list of bool
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
        Also accepts a 1D numpy array with values 0 or 1 for each objective.

    nsamples :
        Number of samples for Monte-Carlo sampling. Higher values give more accurate approximation of the true hypervolume but require more time.

    dist :
      Weight distribution :footcite:p:`AugBadBroZit2009gecco`. The ones currently supported are:

       - ``uniform``:  corresponds to the default hypervolume (unweighted).
       - ``point`` : describes a goal in the objective space, where `mu` gives the coordinates of the goal. The resulting weight distribution is a multivariate normal distribution centred at the goal.
       - ``exponential`` : describes an exponential distribution with rate parameter ``1/mu``, i.e., :math:`\lambda = \frac{1}{\mu}`.

    seed : int or numpy.random.Generator
        Either an integer to seed the NumPy random number generator (RNG) or an instance of Numpy-compatible RNG. ``None`` uses the default RNG of Numpy.

    mu : float or 1D numpy.array
       Parameter of ``dist``. See above for details.


    Returns
    -------
       A single numerical value, the weighted hypervolume


    See Also
    --------
    read_datasets, hypervolume

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> moocore.hypervolume([[2, 2]], ref=4)
    4.0
    >>> moocore.whv_hype([[2, 2]], ref=4, ideal=1, seed=42)
    3.99807
    >>> moocore.hypervolume([[3, 1]], ref=4)
    3.0
    >>> moocore.whv_hype([[3, 1]], ref=4, ideal=1, seed=42)
    3.00555
    >>> moocore.whv_hype(
    ...     [[2, 2]], ref=4, ideal=1, dist="exponential", mu=0.2, seed=42
    ... )
    1.14624
    >>> moocore.whv_hype(
    ...     [[3, 1]], ref=4, ideal=1, dist="exponential", mu=0.2, seed=42
    ... )
    1.66815
    >>> moocore.whv_hype(
    ...     [[2, 2]],
    ...     ref=4,
    ...     ideal=1,
    ...     dist="point",
    ...     mu=[2.9, 0.9],
    ...     seed=42,
    ... )
    0.64485
    >>> moocore.whv_hype(
    ...     [[3, 1]],
    ...     ref=4,
    ...     ideal=1,
    ...     dist="point",
    ...     mu=[2.9, 0.9],
    ...     seed=42,
    ... )
    4.03632

    """
    # Convert to numpy.array in case the user provides a list.  We use
    # np.asfarray to convert it to floating-point, otherwise if a user inputs
    # something like [10, 10] then numpy would interpret it as an int array.
    data, data_copied = asarray_maybe_copy(data)
    nobj = data.shape[1]
    if nobj != 2:
        raise NotImplementedError("Only 2D datasets are currently supported")

    ref = atleast_1d_of_length_n(np.asarray(ref, dtype=float), nobj)
    ideal = atleast_1d_of_length_n(np.asarray(ideal, dtype=float), nobj)

    maximise = _parse_maximise(maximise, nobj)
    # FIXME: Do this in C.
    if maximise.any():
        if not data_copied:
            data = data.copy()
        data[:, maximise] = -data[:, maximise]
        # These are so small that is ok to just copy them.
        ref = ref.copy()
        ideal = ideal.copy()
        ref[maximise] = -ref[maximise]
        ideal[maximise] = -ideal[maximise]

    if not is_integer_value(seed):
        seed = np.random.default_rng(seed).integers(2**32 - 2, dtype=np.uint32)

    data_p, npoints, nobj = np2d_to_double_array(data)
    ref = ffi.from_buffer("double []", ref)
    ideal = ffi.from_buffer("double []", ideal)
    # FIXME: Check ranges.
    seed = ffi.cast("uint32_t", seed)
    nsamples = ffi.cast("int", nsamples)

    if dist == "uniform":
        hv = lib.whv_hype_unif(data_p, npoints, ideal, ref, nsamples, seed)
    elif dist == "exponential":
        mu = ffi.cast("double", mu)
        hv = lib.whv_hype_expo(data_p, npoints, ideal, ref, nsamples, seed, mu)
    elif dist == "point":
        mu = atleast_1d_of_length_n(np.asarray(mu, dtype=float), nobj)
        mu, _ = np1d_to_double_array(mu)
        hv = lib.whv_hype_gaus(data_p, npoints, ideal, ref, nsamples, seed, mu)
    else:
        raise ValueError("Unknown value of dist = {dist}")

    return hv


def get_dataset_path(filename: str, /) -> str:
    """Return path to dataset within the package.

    Parameters
    ----------
    filename :
        Name of the dataset.

    Returns
    -------
        Full path to the dataset

    """
    return files("moocore.data").joinpath(filename)


def get_dataset(filename: str, /) -> np.ndarray:
    """Return path to dataset within the package.

    Parameters
    ----------
    filename :
        Name of the dataset.

    Returns
    -------
        An array containing a representation of the data in the file.
        The first :math:`n-1` columns contain the numerical data for each of the objectives.
        The last column contains an identifier for which set the data is relevant to.

    """
    return read_datasets(get_dataset_path(filename))


def groupby(x, groups, /, *, axis: int = 0):
    """Split an array into groups.

    See https://github.com/numpy/numpy/issues/7265

    Parameters
    ----------
    x : ndarray
        Array to be divided into sub-arrays.
    groups : 1-D array
        A list or ndarray of length equal to the selected `axis`. The values are used as-is to determine the groups and do not need to be sorted.
    axis :
        The axis along which to split, default is 0.

    Yields
    ------
    sub-array : ndarray
        Sub-arrays of `x`.

    """
    index = unique_nosort(groups)
    for g in index:
        yield (g, x.compress(g == groups, axis=axis))
