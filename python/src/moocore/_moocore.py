from __future__ import annotations

import os
from io import StringIO
from collections.abc import Callable
from numpy.typing import ArrayLike  # For type hints
from typing import Literal, Any

from math import gamma as gamma_function
from math import ldexp
# NOTE: if we ever start using SciPy, we can use
# from scipy.special import gamma_function

import lzma
import shutil
import tempfile

import numpy as np

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
        Error code returned by ``read_datasets()`` C function.

    """

    _error_strings = (
        "NO_ERROR",
        "READ_INPUT_FILE_EMPTY",
        "READ_INPUT_WRONG_INITIAL_DIM",
        "ERROR_FOPEN",
        "ERROR_CONVERSION",
        "ERROR_COLUMNS",
    )

    def __init__(self, error_code):
        self.error = error_code
        self.message = self._error_strings[abs(error_code)]
        super().__init__(self.message)


def read_datasets(filename: str | os.PathLike | StringIO) -> np.ndarray:
    """Read an input dataset file, parsing the file and returning a numpy array.

    Parameters
    ----------
    filename:
        Filename of the dataset file or :class:`io.StringIO` directly containing the file contents.
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
    | 8.07559653  | 2.40702554  | 1.         |
    +-------------+-------------+------------+
    | 8.66094446  | 3.64050144  | 1.         |
    +-------------+-------------+------------+
    | ...         | ...         | ...        |
    +-------------+-------------+------------+
    | 7.99466959  | 2.81122537  | 10.        |
    +-------------+-------------+------------+
    | 2.12700289  | 2.43114174  | 10.        |
    +-------------+-------------+------------+

    It is also possible to read datasets from a string:

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


def _all_positive(x: ArrayLike):
    return x.min() > 0


def _unary_refset_common(data, ref, maximise, check_all_positive=False):
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
    if check_all_positive:
        if not _all_positive(data):
            raise ValueError(
                "All values must be larger than 0 in the input data"
            )
        if not _all_positive(ref):
            raise ValueError(
                "All values must be larger than 0 in the reference set"
            )

    maximise = _parse_maximise(maximise, nobj)
    data_p, npoints, nobj = np2d_to_double_array(data)
    ref_p, ref_size = np1d_to_double_array(ref)
    maximise_p = ffi.from_buffer("bool []", maximise)
    return data_p, nobj, npoints, ref_p, ref_size, maximise_p


def igd(
    data: ArrayLike, /, ref: ArrayLike, *, maximise: bool | list[bool] = False
) -> float:
    """Inverted Generational Distance (IGD).

    .. seealso:: For details about parameters, return value and examples, see :func:`igd_plus`.  For details of the calculation, see :ref:`igd_hausdorf`.

    Returns
    -------
        A single numerical value.

    """
    data_p, nobj, npoints, ref_p, ref_size, maximise_p = _unary_refset_common(
        data, ref, maximise
    )
    return lib.IGD(data_p, nobj, npoints, ref_p, ref_size, maximise_p)


def igd_plus(
    data: ArrayLike, /, ref: ArrayLike, *, maximise: bool | list[bool] = False
) -> float:
    r"""Modified IGD (IGD+).

    IGD+ is a Pareto-compliant version of :func:`igd` proposed by :cite:t:`IshMasTanNoj2015igd`.

    .. seealso:: For details of the calculation, see :ref:`igd_hausdorf`.

    Parameters
    ----------
    data :
        Matrix of numerical values, where each row gives the coordinates of a point in objective space.
        If the array is created from the :func:`read_datasets` function, remove the last (set) column.

    ref :
        Reference set as a matrix of numerical values. Must have the same number of columns as ``data``.

    maximise :
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
        Also accepts a 1D numpy array with value 0/1 for each objective.

    Returns
    -------
        A single numerical value.

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

    Example 4 by :cite:t:`IshMasTanNoj2015igd` shows a case where IGD gives the
    wrong answer. In this case A is better than B in terms of Pareto optimality.

    >>> ref = np.array([10, 0, 6, 1, 2, 2, 1, 6, 0, 10]).reshape(-1, 2)
    >>> A = np.array([4, 2, 3, 3, 2, 4]).reshape(-1, 2)
    >>> B = np.array([8, 2, 4, 4, 2, 8]).reshape(-1, 2)
    >>> print(
    ...     f"IGD(A)={moocore.igd(A, ref)} > IGD(B)={moocore.igd(B, ref)}\n"
    ...     + f"and AvgHausdorff(A)={moocore.avg_hausdorff_dist(A, ref)} > "
    ...     + f"AvgHausdorff(B)={moocore.avg_hausdorff_dist(B, ref)},\n"
    ...     + f"which both contradict Pareto optimality. By contrast,\n"
    ...     + f"IGD+(A)={moocore.igd_plus(A, ref)} < IGD+(B)={moocore.igd_plus(B, ref)},"
    ...     + " which is correct."
    ... )
    IGD(A)=3.707092031609239 > IGD(B)=2.59148346584763
    and AvgHausdorff(A)=3.707092031609239 > AvgHausdorff(B)=2.59148346584763,
    which both contradict Pareto optimality. By contrast,
    IGD+(A)=1.482842712474619 < IGD+(B)=2.260112615949154, which is correct.


    """  # noqa: D401
    data_p, nobj, npoints, ref_p, ref_size, maximise_p = _unary_refset_common(
        data, ref, maximise
    )
    return lib.IGD_plus(data_p, nobj, npoints, ref_p, ref_size, maximise_p)


def avg_hausdorff_dist(  # noqa: D417
    data: ArrayLike,
    /,
    ref: ArrayLike,
    *,
    maximise: bool | list[bool] = False,
    p: float = 1,
) -> float:
    """Average Hausdorff distance.

    .. seealso:: For details about parameters, return value and examples, see :func:`igd_plus`.  For details of the calculation, see :ref:`igd_hausdorf`.

    Parameters
    ----------
    p :
        Hausdorff distance parameter. Must be larger than 0.

    Returns
    -------
        A single numerical value.

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
    r"""Additive epsilon metric.

    The current implementation uses the naive algorithm that requires
    :math:`O(n \cdot |A| \cdot |R|)`, where :math:`n` is the number of
    objectives (dimension of vectors), :math:`A` is the input set and :math:`R`
    is the reference set.

    .. seealso:: For details of the calculation, see :ref:`epsilon_metric`.

    Parameters
    ----------
    data :
        Numpy array of numerical values, where each row gives the coordinates of a point in objective space.
        If the array is created from the :func:`read_datasets` function, remove the last (set) column
    ref :
        Reference set as a matrix of numerical values. Must have the same number of columns as ``data``.
    maximise :
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
        Also accepts a 1d numpy array with value 0/1 for each objective

    Returns
    -------
        A single numerical value.

    Examples
    --------
    >>> import numpy as np
    >>> dat = np.array([[3.5, 5.5], [3.6, 4.1], [4.1, 3.2], [5.5, 1.5]])
    >>> ref = np.array([[1, 6], [2, 5], [3, 4], [4, 3], [5, 2], [6, 1]])
    >>> moocore.epsilon_additive(dat, ref=ref)
    2.5
    >>> moocore.epsilon_mult(dat, ref=ref)
    3.5
    >>> float(np.exp(moocore.epsilon_additive(np.log(dat), ref=np.log(ref))))
    3.5

    """
    data_p, nobj, npoints, ref_p, ref_size, maximise_p = _unary_refset_common(
        data, ref, maximise
    )
    return lib.epsilon_additive(
        data_p, nobj, npoints, ref_p, ref_size, maximise_p
    )


def epsilon_mult(data, /, ref, *, maximise: bool | list[bool] = False) -> float:
    """Multiplicative epsilon metric.

    .. warning::
       ``data`` and ``ref`` must all be larger than 0.

    .. seealso:: For details about parameters, return value and examples, see :func:`epsilon_additive`.  For details of the calculation, see :ref:`epsilon_metric`.

    Returns
    -------
        A single numerical value.

    """
    data_p, nobj, npoints, ref_p, ref_size, maximise_p = _unary_refset_common(
        data, ref, maximise, check_all_positive=True
    )
    return lib.epsilon_mult(data_p, nobj, npoints, ref_p, ref_size, maximise_p)


def _hypervolume(data: ArrayLike, ref: ArrayLike):
    data_p, npoints, nobj = np2d_to_double_array(data)
    ref_buf = ffi.from_buffer("double []", ref)
    hv = lib.fpli_hv(data_p, nobj, npoints, ref_buf)
    if hv < 0:
        raise MemoryError("memory allocation failed")
    return hv


def hypervolume(
    data: ArrayLike, /, ref: ArrayLike, *, maximise: bool | list[bool] = False
) -> float:
    r"""Hypervolume indicator.

    Compute the hypervolume metric with respect to a given reference point
    assuming minimization of all objectives.

    .. seealso:: For details about the hypervolume, see :ref:`hypervolume_metric`.

    Parameters
    ----------
    data :
        Numpy array of numerical values, where each row gives the coordinates of a point.
        If the array is created from the :func:`read_datasets` function, remove the last column.
    ref :
        Reference point as a 1D vector. Must be same length as a single point in the ``data``.
    maximise :
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
        Also accepts a 1D numpy array with value 0/1 for each objective

    Returns
    -------
        A single numerical value, the hypervolume indicator.

    See Also
    --------
    Hypervolume : object-oriented interface.
    RelativeHypervolume : Compute hypervolume relative to a reference set.


    Notes
    -----
    For 2D and 3D, the algorithms used
    :footcite:p:`FonPaqLop06:hypervolume,BeuFonLopPaqVah09:tec` have :math:`O(n
    \log n)` complexity, where :math:`n` is the number of input points.  The 3D case uses the HV3D\ :sup:`+` algorithm :footcite:p:`GueFon2017hv4d`, which has the same complexity as the HV3D algorithm :footcite:p:`FonPaqLop06:hypervolume,BeuFonLopPaqVah09:tec`, but it is faster in practice.

    For 4D, the algorithm used is HV4D\ :sup:`+` :footcite:p:`GueFon2017hv4d`,
    which has :math:`O(n^2)` complexity.  Compared to the `original
    implementation <https://github.com/apguerreiro/HVC/>`_, this implementation
    correctly handles weakly dominated points and has been further optimized
    for speed.

    For 5D or higher, it uses a recursive algorithm that has the 3D algorithm
    as a base case :footcite:p:`FonPaqLop06:hypervolume`, which has
    :math:`O(n^{m-2} \log n)` time and linear space complexity in the
    worst-case, where :math:`m` is the dimension of the points, but
    experimental results show that the pruning techniques used may reduce the
    time complexity even further.  Andreia P. Guerreiro improved the
    integration of the 3D case with the recursive algorithm, which leads to
    significant reduction of computation time. She has also enhanced the
    numerical stability of the algorithm by avoiding floating-point comparisons
    of partial hypervolumes.


    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> dat = np.array([[5, 5], [4, 6], [2, 7], [7, 4]])
    >>> moocore.hypervolume(dat, ref=[10, 10])
    38.0

    This function assumes that objectives must be minimized by default. We can
    easily specify maximization:

    >>> dat = np.array([[5, 5], [4, 6], [2, 7], [7, 4]])
    >>> moocore.hypervolume(dat, ref=0, maximise=True)
    39.0

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

    return _hypervolume(data, ref)


class Hypervolume:
    """Object-oriented interface for the hypervolume indicator.

    .. seealso:: For details about parameters, return value and examples, see :func:`hypervolume`.

    Parameters
    ----------
    ref :
       Reference point as a 1D vector. Must be same length as a single point in the ``data``.
    maximise :
       Whether the objectives must be maximised instead of minimised.
       Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
       Also accepts a 1D numpy array with value 0/1 for each objective

    Examples
    --------
    This function assumes that objectives must be minimized by default. We can
    easily specify maximization:

    >>> hv_ind = moocore.Hypervolume(ref=0, maximise=True)
    >>> hv_ind([[5, 5], [4, 6], [2, 7], [7, 4]])
    39.0
    >>> hv_ind([[5, 5], [4, 6], [7, 4]])
    37.0

    See Also
    --------
    RelativeHypervolume : Compute hypervolume relative to a reference set.

    """

    def __init__(
        self, ref: ArrayLike, maximise: bool | list[bool] = False
    ) -> None:
        self._ref = np.array(ref, dtype=float, ndmin=1)
        self._maximise = np.array(maximise, dtype=bool, ndmin=1)
        n = len(self._maximise)
        if n == 1:
            n = len(self._ref)
            if n > 1:
                self._maximise = np.full((n), self._maximise[0])

        elif len(self._ref) == 1:
            self._ref = np.full((n), self._ref[0])
        elif n != len(self._ref):
            raise ValueError(
                f"ref and maximise need to have the same length ({len(self._ref)} != {n})"
            )

        if self._maximise.any():
            self._ref[self._maximise] = -self._ref[self._maximise]
        self._nobj = n

    def __call__(self, data: ArrayLike) -> float:
        r"""Compute hypervolume indicator.

        Parameters
        ----------
        data :
            Numpy array of numerical values, where each row gives the coordinates of a point.
            If the array is created from the :func:`read_datasets` function, remove the last column.

        Returns
        -------
           A single numerical value, the hypervolume indicator.

        """
        data, data_copied = asarray_maybe_copy(data)
        nobj = self._nobj
        ref = self._ref
        maximise = self._maximise

        if nobj == 1:
            nobj = data.shape[1]
            ref = np.full((nobj), ref[0])
            maximise = np.full((nobj), maximise[0])
        elif nobj != data.shape[1]:
            raise ValueError(
                f"data and ref need to have the same number of objectives ({data.shape[1]} != {nobj})"
            )

        # FIXME: Do this in C.
        if maximise.any():
            if not data_copied:
                data = data.copy()
            data[:, maximise] = -data[:, maximise]

        return _hypervolume(data, ref)


class RelativeHypervolume(Hypervolume):
    r"""Computes the hypervolume value of fronts relative to the hypervolume of a reference front.

    The value is computed as :math:`1 - \frac{\text{hyp}(X)}{\text{hyp}(R)}`,
    where :math:`X` is an input set (front), :math:`R` is the reference front
    and :math:`\text{hyp}()` is calculated by :func:`hypervolume`.  Thus, lower
    values are better, in contrast to the usual hypervolume.  The metric has
    also been called *hypervolume relative deviation*
    :footcite:p:`BezLopStu2017assessment`.

    Parameters
    ----------
    ref :
       Reference point as a 1D vector. Must be same length as a single point in ``ref_set``.
    ref_set :
       Reference set (front) as 2D array. The reference front does not need to
       weakly dominate the input sets. If this is required, the user must ensure it.
    maximise :
       Whether the objectives must be maximised instead of minimised.
       Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
       Also accepts a 1D numpy array with value 0/1 for each objective.


    References
    ----------
    .. footbibliography::


    Examples
    --------
    This function assumes that objectives must be minimized by default. We can
    easily specify maximization:

    >>> ref_set = [[6, 6], [2, 7], [7, 2]]
    >>> hv_ind = moocore.RelativeHypervolume(ref=0, ref_set=ref_set, maximise=True)
    >>> hv_ind([[5, 5], [4, 6], [2, 7], [7, 4]])
    0.0250000
    >>> hv_ind([[5, 5], [4, 6], [7, 4]])
    0.0749999
    >>> hv_ind([[0, 0]])
    1.0
    >>> hv_ind(ref_set)
    0.0

    """

    def __init__(
        self,
        ref: ArrayLike,
        ref_set: ArrayLike,
        maximise: bool | list[bool] = False,
    ) -> None:
        super().__init__(ref=ref, maximise=maximise)
        self._ref_set_hv = super().__call__(ref_set)
        if self._ref_set_hv == 0:
            raise ValueError("hypervolume of 'ref_set' is zero")

    def __call__(self, data: ArrayLike) -> float:
        r"""Compute relative hypervolume indicator as ``1 - (hypervolume(data, ref=ref) / hypervolume(ref_set, ref=ref))``.

        Parameters
        ----------
        data :
            Numpy array of numerical values, where each row gives the
            coordinates of a point.  If the array is created from the
            :func:`read_datasets` function, remove the last column.

        Returns
        -------
           A single numerical value, the relative hypervolume indicator, which must be minimized.

        """
        data_hv = super().__call__(data)
        return 1.0 - data_hv / self._ref_set_hv


def hv_contributions(
    x: ArrayLike,
    /,
    ref: ArrayLike,
    maximise: bool | list[bool] = False,
) -> np.array:
    r"""Hypervolume contributions of a set of points.

    Computes the hypervolume contribution of each point of a set of points with
    respect to a given reference point. The hypervolume contribution of point
    :math:`\vec{p} \in X` is :math:`\text{hvc}(\vec{p}) = \text{hyp}(X) -
    \text{hyp}(X \setminus \{\vec{p}\})`. Dominated points have zero
    contribution. Duplicated points have zero contribution even if not
    dominated, because removing one of the duplicates does not change the
    hypervolume of the remaining set.

    The current implementation uses the naive algorithm that requires
    calculating the hypervolume :math:`|X|+1` times.

    .. seealso:: For details about the hypervolume, see :ref:`hypervolume_metric`.

    Parameters
    ----------
    x :
        Numpy array of numerical values, where each row gives the coordinates of a point.
        If the array is created from the :func:`read_datasets` function, remove the last column.
    ref :
        Reference point as a 1D vector. Must be same length as a single point in ``x``.
    maximise :
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
        Also accepts a 1D numpy array with value 0/1 for each objective

    Returns
    -------
        An array of floating-point values as long as the number of rows in ``x``.
        Each value is the contribution of the corresponding point in ``x``.

    Examples
    --------
    >>> dat = np.array([[5, 5], [4, 6], [2, 7], [7, 4]])
    >>> moocore.hv_contributions(dat, ref=[10, 10])
    array([2., 1., 6., 3.])

    """
    # Convert to numpy.array in case the user provides a list.  We use
    # np.asarray to convert it to floating-point, otherwise if a user inputs
    # something like ref = np.array([10, 10]) then numpy would interpret it as
    # an int array.
    x, x_copied = asarray_maybe_copy(x)
    nobj = x.shape[1]
    ref = atleast_1d_of_length_n(np.array(ref, dtype=float), nobj)
    if nobj != ref.shape[0]:
        raise ValueError(
            f"data and ref need to have the same number of objectives ({nobj} != {ref.shape[0]})"
        )

    maximise = _parse_maximise(maximise, nobj)
    # FIXME: Do this in C.
    if maximise.any():
        if not x_copied:
            x = x.copy()
        x[:, maximise] = -x[:, maximise]
        ref = ref.copy()
        ref[maximise] = -ref[maximise]

    hvc = np.empty(len(x), dtype=float)
    hvc_p, _ = np1d_to_double_array(hvc)
    x_p, npoints, nobj = np2d_to_double_array(x)
    ref_buf = ffi.from_buffer("double []", ref)
    lib.hv_contributions(hvc_p, x_p, nobj, npoints, ref_buf)
    return hvc


def hv_approx(
    data: ArrayLike,
    /,
    ref: ArrayLike,
    maximise: bool | list[bool] = False,
    nsamples: int = 100000,
    seed: int | np.random.Generator | None = None,
    method: Literal["DZ2019"] = "DZ2019",
) -> float:
    r"""Approximate the hypervolume indicator.

    Approximate the value of the hypervolume metric with respect to a given
    reference point assuming minimization of all objectives. The default
    ``method="DZ2019"`` relies on Monte-Carlo sampling
    :footcite:p:`DenZha2019approxhv` and, thus, it gets more accurate, but
    slower, for higher values of ``nsamples``.

    .. seealso:: For details of the calculation, see :ref:`hv_approximation`.


    Parameters
    ----------
    data :
        Numpy array of numerical values, where each row gives the coordinates of a point.
        If the array is created from the :func:`read_datasets` function, remove the last column.
    ref :
        Reference point as a 1D vector. Must be same length as a single point in the ``data``.
    maximise :
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
        Also accepts a 1d numpy array with value 0/1 for each objective
    nsamples :
        Number of samples for Monte-Carlo sampling. Higher values give more
        accurate approximation of the true hypervolume but require more time.
    seed :
        Either an integer to seed :func:`numpy.random.default_rng`, Numpy
        default random number generator (RNG) or an instance of a
        Numpy-compatible RNG. ``None`` uses the equivalent of a random seed
        (see :func:`numpy.random.default_rng`).

    method :
        Method to approximate the hypervolume.

    Returns
    -------
        A single numerical value, the approximate hypervolume indicator.

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> x = np.array([[5, 5], [4, 6], [2, 7], [7, 4]])
    >>> moocore.hv_approx(x, ref=[10, 10], seed=42)
    37.95471
    >>> moocore.hypervolume(x, ref=[10, 10])
    38.0

    Merge all the sets of a dataset by removing the set number column:

    >>> x = moocore.get_dataset("input1.dat")[:, :-1]

    Dominated points are ignored, so this:

    >>> moocore.hv_approx(x, ref=10, seed=42)
    93.348976559100

    gives the same hypervolume approximation as this:

    >>> x = moocore.filter_dominated(x)
    >>> moocore.hv_approx(x, ref=10, seed=42)
    93.348976559100

    The approximation is far from perfect for large sets:

    >>> x = moocore.get_dataset("CPFs.txt")[:, :-1]
    >>> x = moocore.filter_dominated(-x, maximise=True)
    >>> x = moocore.normalise(x, to_range=[1, 2])
    >>> reference = 0.9
    >>> moocore.hypervolume(x, ref=reference, maximise=True)
    1.0570447464301551
    >>> moocore.hv_approx(x, ref=reference, maximise=True, seed=42)
    1.056312559097445

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

    expected = np.empty(nsamples, dtype=float)
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
    # ldexp(x, n) = x * 2^n
    c_m = (np.pi ** (nobj / 2)) / ldexp(gamma_function(nobj / 2 + 1), nobj)
    return float(c_m * expected)


def is_nondominated(
    data: ArrayLike,
    maximise: bool | list[bool] = False,
    keep_weakly: bool = False,
) -> np.ndarray:
    r"""Identify dominated points according to Pareto optimality.

    Given :math:`n` points of dimension :math:`m`, the current implementation
    uses the well-known :math:`O(n \log n)` dimension-sweep algorithm
    :footcite:p:`KunLucPre1975jacm` for :math:`m \leq 3` and the naive
    :math:`O(m n^2)` algorithm for :math:`m \geq 4`.


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
        Which of the duplicates is identified as nondominated is unspecified
        due to the sorting not being stable in this version.

    Returns
    -------
        Returns a boolean array of the same length as the number of rows of data,
        where ``True`` means that the point is not dominated by any other point.

    See Also
    --------
    filter_dominated : to filter out dominated points.

    pareto_rank : to rank points according to Pareto dominance.


    References
    ----------
    .. footbibliography::


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
    nondom = lib.is_nondominated(data_p, nobj, npoints, maximise_p, keep_weakly)
    nondom = ffi.buffer(nondom, nrows)
    return np.frombuffer(nondom, dtype=bool)


def is_nondominated_within_sets(
    data: ArrayLike,
    /,
    sets: ArrayLike,
    *,
    maximise: bool | list[bool] = False,
    keep_weakly: bool = False,
) -> np.ndarray:
    r"""Identify dominated points according to Pareto optimality within each set.

    Executes the :func:`is_nondominated` function within each set in a dataset
    and returns back a 1D array of booleans. This is equivalent to
    ``apply_within_sets(data, sets, is_nondominated, ...)`` but slightly
    faster.

    Parameters
    ----------
    data :
        Array of numerical values, where each row gives the coordinates of a point in objective space.
        If the array is created by the :func:`read_datasets()` function, remove the last column.
    sets :
        1D vector or list of values that define the sets to which each row of ``data`` belongs.
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
    filter_dominated_within_sets : filter out dominated points.
    apply_within_sets : a more general way to apply any function to each set.

    Examples
    --------
    >>> x = np.array([[1, 2, 1], [1, 3, 1], [2, 1, 1], [2, 2, 2]])
    >>> moocore.is_nondominated_within_sets(x[:, :-1], x[:, -1])
    array([ True, False,  True,  True])
    >>> x = moocore.get_dataset("input1.dat")
    >>> nondom_per_set = moocore.is_nondominated_within_sets(x[:, :-1], x[:, -1])
    >>> len(nondom_per_set)
    100
    >>> nondom_per_set  # doctest: +ELLIPSIS
    array([False, False,  True, False,  True, False, False, False, False,
            True, False,  True,  True,  True, False,  True,  True,  True,
           False,  True, False, False, False, False,  True, False,  True,
           ...
            True,  True,  True, False,  True, False,  True,  True, False,
            True, False, False,  True,  True, False, False, False, False,
           False])
    >>> x[nondom_per_set, :]  # doctest: +ELLIPSIS
    array([[ 0.20816431,  4.62275469,  1.        ],
           [ 0.22997367,  1.11772205,  1.        ],
           [ 0.58799475,  0.73891181,  1.        ],
           [ 1.5964888 ,  5.98825094,  2.        ],
           [ 5.2812367 ,  3.47800969,  2.        ],
           [ 2.16315952,  4.7394435 ,  2.        ],
           ...
           [ 0.6510164 ,  9.42381213,  9.        ],
           [ 1.30291449,  4.50417698,  9.        ],
           [ 0.62230271,  3.56945324, 10.        ],
           [ 0.86723965,  1.58599089, 10.        ],
           [ 6.43135537,  1.00153569, 10.        ]])

    """
    data = np.asarray(data, dtype=float)
    ncols = data.shape[1]
    if ncols < 2:
        raise ValueError("'data' must have at least 2 columns (2 objectives)")

    # FIXME: How can we make this faster?
    _, idx, inv = np.unique(sets, return_index=True, return_inverse=True)
    # Remember the original position of each element of each set.
    idx = [np.flatnonzero(inv == i) for i in idx.argsort()]
    data = np.concatenate(
        [
            is_nondominated(
                data.take(g_idx, axis=0),
                maximise=maximise,
                keep_weakly=keep_weakly,
            )
            for g_idx in idx
        ]
    )
    idx = np.concatenate(idx).argsort()
    return data.take(idx, axis=0)


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
    data: ArrayLike,
    /,
    *,
    maximise: bool | list[bool] = False,
    keep_weakly: bool = False,
) -> np.ndarray:
    """Given a dataset with multiple sets (last column gives the set index), filter dominated points within each set.

    Executes the :func:`filter_dominated` function within each set in a dataset
    and returns back a dataset. This is roughly equivalent to partitioning ``data`` according to the last column,
    filtering dominated solutions within each partition, and joining back the result.

    Parameters
    ----------
    data :
        Numpy array of numerical values and set numbers, containing multiple datasets. For example the output of the :func:`read_datasets` function.
    maximise :
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
        Also accepts a 1D numpy array with values 0 or 1 for each objective
    keep_weakly :
        If ``False``, do not delete duplicates of nondominated points.

    Returns
    -------
        A numpy array where each set only contains nondominated points with respect to the set (last column is the set index).
        Points from one set can still dominate points from another set.

    Examples
    --------
    >>> x = moocore.get_dataset("input1.dat")
    >>> pf_per_set = moocore.filter_dominated_within_sets(x)
    >>> len(pf_per_set)
    42
    >>> pf_per_set  # doctest: +ELLIPSIS
    array([[ 0.20816431,  4.62275469,  1.        ],
           [ 0.22997367,  1.11772205,  1.        ],
           [ 0.58799475,  0.73891181,  1.        ],
           [ 1.5964888 ,  5.98825094,  2.        ],
           [ 5.2812367 ,  3.47800969,  2.        ],
           [ 2.16315952,  4.7394435 ,  2.        ],
           ...
           [ 0.6510164 ,  9.42381213,  9.        ],
           [ 1.30291449,  4.50417698,  9.        ],
           [ 0.62230271,  3.56945324, 10.        ],
           [ 0.86723965,  1.58599089, 10.        ],
           [ 6.43135537,  1.00153569, 10.        ]])
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
    >>> moocore.filter_dominated_within_sets(x[(x[:, 2] >= 3) & (x[:, 2] <= 5), :])
    array([[2.60764118, 6.31309852, 3.        ],
           [3.22509709, 6.1522834 , 3.        ],
           [0.37731545, 9.02211752, 3.        ],
           [4.61023932, 2.29231998, 3.        ],
           [0.2901393 , 8.32259412, 4.        ],
           [1.54506255, 0.38303122, 4.        ],
           [4.43498452, 4.13150648, 5.        ],
           [9.78758589, 1.41238277, 5.        ],
           [7.85344142, 3.02219054, 5.        ],
           [0.9017068 , 7.49376946, 5.        ],
           [0.17470556, 8.89066343, 5.        ]])

    The above returns sets 3,4,5 with dominated points within each set removed.


    See Also
    --------
    read_datasets : read datasets from a file.
    filter_dominated : to be used with a single dataset.

    """
    data = np.asarray(data, dtype=float)
    if data.shape[1] < 3:
        raise ValueError(
            "'data' must have at least 3 columns (2 objectives + set column)"
        )

    is_nondom = is_nondominated_within_sets(
        data[:, :-1],
        sets=data[:, -1],
        maximise=maximise,
        keep_weakly=keep_weakly,
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


def normalise(
    data: ArrayLike,
    /,
    to_range: ArrayLike = [0.0, 1.0],
    *,
    lower: ArrayLike = np.nan,
    upper: ArrayLike = np.nan,
    maximise: bool | list[bool] = False,
) -> np.ndarray:
    """Normalise points per coordinate to a range, e.g., ``to_range = [1,2]``, where the minimum value will correspond to 1 and the maximum to 2.

    Parameters
    ----------
    data :
        Numpy array of numerical values, where each row gives the coordinates of a point in objective space.

    to_range :
        Range composed of two numerical values. Normalise values to this range. If the objective is maximised, it is normalised to ``(to_range[1], to_range[0])`` instead.

    upper, lower:
        Bounds on the values. If :data:`numpy.nan`, the maximum and minimum values of each coordinate are used.

    maximise :
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
        Also accepts a 1D numpy array with values 0 or 1 for each objective

    Returns
    -------
        Returns the data normalised as requested.

    Examples
    --------
    >>> dat = np.array([[3.5, 5.5], [3.6, 4.1], [4.1, 3.2], [5.5, 1.5]])
    >>> moocore.normalise(dat)
    array([[0.   , 1.   ],
           [0.05 , 0.65 ],
           [0.3  , 0.425],
           [1.   , 0.   ]])

    >>> moocore.normalise(dat, to_range=[1, 2], lower=[3.5, 3.5], upper=5.5)
    array([[1.  , 2.  ],
           [1.05, 1.3 ],
           [1.3 , 0.85],
           [2.  , 0.  ]])

    """
    # normalise() modifies the data, so we need to create a copy.
    # order='C' is needed for np2d_to_double_array()
    data = np.array(data, dtype=float, order="C")
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
    # We can return data directly because we only changed the data, not the shape.
    return data


def eaf(data: ArrayLike, /, percentiles: list = []) -> np.ndarray:
    """Compute the Empirical attainment function (EAF) in 2D or 3D.

    Parameters
    ----------
    data :
        Numpy array of numerical values and set numbers, containing multiple sets. For example the output of the :func:`read_datasets` function
    percentiles :
        List indicating which percentiles are computed. By default, all possible percentiles are calculated.

    Returns
    -------
        EAF data points, with the same number of columns as the input argument, but a different number of rows. The last column represents the EAF percentile for that data point.

    See Also
    --------
    mooplot.plot_eaf: Plotting the EAF.



    Examples
    --------
    >>> x = moocore.get_dataset("input1.dat")
    >>> moocore.eaf(x)  # doctest: +ELLIPSIS
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

    >>> moocore.eaf(x, percentiles=[0, 50, 100])  # doctest: +ELLIPSIS
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
    eaf_buf = ffi.buffer(eaf_data_p, ffi.sizeof("double") * eaf_npoints * ncols)
    return np.frombuffer(eaf_buf).reshape((eaf_npoints, -1))


def vorob_t(data: ArrayLike, /, ref: ArrayLike) -> dict:
    """Compute Vorob'ev threshold and expectation.

    Parameters
    ----------
    data :
        Numpy array of numerical values and set numbers, containing multiple
        sets. For example the output of the :func:`read_datasets` function.
    ref :
       Reference point as a 1D vector. Must be same length as a single point in the ``data``.

    Returns
    -------
        A dictionary with elements `threshold`, `ve`, and `avg_hyp` (average hypervolume).

    See Also
    --------
    vorob_dev : Compute Vorob'ev deviation.

    Notes
    -----
    For more background, see :footcite:t:`BinGinRou2015gaupar,Molchanov2005theory,CheGinBecMol2013moda`.

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> CPFs = moocore.get_dataset("CPFs.txt")
    >>> res = moocore.vorob_t(CPFs, ref=(2, 200))
    >>> res["threshold"]
    44.140625
    >>> res["avg_hyp"]
    8943.333191728081
    >>> res["ve"].shape
    (213, 2)
    """
    data = np.asarray(data, dtype=float)
    ncols = data.shape[1]
    if ncols < 3:
        raise ValueError(
            "'data' must have at least 3 columns (2 objectives + set column)"
        )
    nobj = ncols - 1
    sets = data[:, -1]
    avg_hyp = np.mean(
        apply_within_sets(data[:, :-1], sets, hypervolume, ref=ref)
    )
    prev_hyp = diff = np.inf  # hypervolume of quantile at previous step
    a = 0.0
    b = 100.0
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

    return dict(threshold=c, ve=eaf_res, avg_hyp=float(avg_hyp))


def vorob_dev(
    data: ArrayLike, /, ref: ArrayLike, *, ve: ArrayLike = None
) -> float:
    r"""Compute Vorob'ev deviation.

    Parameters
    ----------
    data :
       Numpy array of numerical values and set numbers, containing multiple sets.
       For example the output of the :func:`read_datasets` function.
    ref :
       Reference point as a 1D vector. Must be same length as a single point in the ``data``.
    ve :
       Vorob'ev expectation, e.g., as returned by :func:`vorob_t`.
       If not provided, it is calculated as ``vorob_t(x, ref)``.

    Returns
    -------
        Vorob'ev deviation.

    See Also
    --------
    vorob_t : Compute Vorob'ev threshold and expectation.

    Notes
    -----
    For more background, see :footcite:t:`BinGinRou2015gaupar,Molchanov2005theory,CheGinBecMol2013moda`.

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> CPFs = moocore.get_dataset("CPFs.txt")
    >>> vd = moocore.vorob_dev(CPFs, ref=(2, 200))
    >>> vd
    3017.12989402326

    """
    if ve is None:
        ve = vorob_t(data, ref=ref)["ve"]

    data = np.asarray(data, dtype=float)
    ncols = data.shape[1]
    if ncols < 3:
        raise ValueError(
            "'data' must have at least 3 columns (2 objectives + set column)"
        )

    # Hypervolume of the symmetric difference between A and B:
    # 2 * H(AUB) - H(A) - H(B)
    hv_ind = Hypervolume(ref=ref)
    h2 = hv_ind(ve)
    sets = data[:, -1]
    data = data[:, :-1]
    h1 = np.mean(apply_within_sets(data, sets, hv_ind))
    vd = 2 * np.mean(
        apply_within_sets(data, sets, lambda g: hv_ind(np.vstack((g, ve))))
    )
    return float(vd - h1 - h2)


def eafdiff(
    x: ArrayLike,
    y: ArrayLike,
    /,
    *,
    intervals: int | None = None,
    maximise: bool | list[bool] = False,
    rectangles: bool = False,
) -> np.ndarray:
    """Compute empirical attainment function (EAF) differences.

    Calculate the differences between the empirical attainment functions of two
    data sets.

    .. warning::
        The current implementation only supports 2 objectives.


    Parameters
    ----------
    x, y : ArrayLike
       Numpy matrices corresponding to the input data of left and right sides,
       respectively. Each data frame has at least three columns, the third one
       being the set of each point. See also :func:`read_datasets`.

    intervals :
       The absolute range of the differences :math:`[0, 1]` is partitioned into the number of intervals provided.

    maximise :
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
        Also accepts a 1D numpy array with values 0 or 1 for each objective

    rectangles :
       If ``True``, the output is in the form of rectangles of the same color.


    Returns
    -------
       With ``rectangles=False``, a matrix with three columns, The first two
       columns describe the points where there is a transition in the value of
       the EAF differences.

       With ``rectangles=True``, a matrix with five columns, where the first 4
       columns give the coordinates of two corners of each rectangle. In both
       cases, the last column gives the difference in terms of sets in ``x``
       minus sets in ``y`` that attain each point (i.e., negative values are
       differences in favour ``y``).


    See Also
    --------
    mooplot.eafdiffplot: Plotting EAF differences.


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
    assert x.shape[1] == y.shape[1], (
        "'x' and 'y' must have the same number of columns"
    )
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


def whv_rect(
    x: ArrayLike,
    /,
    rectangles: ArrayLike,
    *,
    ref: ArrayLike,
    maximise: bool | list[bool] = False,
) -> float:
    """Compute weighted hypervolume given a set of rectangles.

    .. seealso:: For details about parameters, return value and examples, see :func:`total_whv_rect`.

    .. warning::
        The current implementation only supports 2 objectives.

    Returns
    -------
        A single numerical value, the weighted hypervolume.

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
    maximise = _parse_maximise(maximise, nobj)
    if maximise.any():
        raise NotImplementedError("Only minimization is currently supported")
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
    x: ArrayLike,
    /,
    rectangles: ArrayLike,
    *,
    ref: ArrayLike,
    maximise: bool | list[bool] = False,
    ideal: ArrayLike = None,
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
        A single numerical value, the weighted hypervolume.

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


def largest_eafdiff(
    x: list,
    /,
    ref: ArrayLike,
    *,
    maximise: bool | list[bool] = False,
    intervals: int = 5,
    ideal: ArrayLike = None,
) -> tuple[tuple[int, int], float]:
    """Identify largest EAF differences.

    Given a list of datasets, return the indexes of the pair with the largest
    EAF differences according to the method proposed by :footcite:t:`DiaLop2020ejor`.

    .. warning::
        The current implementation only supports 2 objectives.

    Parameters
    ----------
    x :
       A list of matrices with at least 3 columns (last column indicates the set).

    ref :
        Reference point as a 1D vector. Must be same length as a single point in the input data.

    maximise :
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
        Also accepts a 1D numpy array with value 0/1 for each objective

    intervals :
       The absolute range of the differences :math:`[0, 1]` is partitioned into the number of intervals provided.

    ideal :
        Ideal point as a vector of numerical values.  If ``None``, it is calculated as minimum (or maximum if maximising that objective) of each objective in the input data.


    Returns
    -------
    pair : tuple[int,int]
        Pair of indexes into the list that give the largest EAF difference.

    value : float
        Value of the largest difference.


    See Also
    --------
    eafdiff, whv_rect

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> import pandas as pd
    >>> df = pd.read_csv(moocore.get_dataset_path("tpls50x20_1_MWT.csv"))
    >>> df
         algorithm  Makespan  WeightedTardiness   run
    0         1to2    4280.0            10231.0   1.0
    1         1to2    4238.0            10999.0   1.0
    2         1to2    4137.0            11737.0   1.0
    3         1to2    4024.0            14871.0   1.0
    4         1to2    4014.0            17825.0   1.0
    ...        ...       ...                ...   ...
    1506    double    4048.0            14755.0  15.0
    1507    double    3923.0            25507.0  15.0
    1508    double    3890.0            29567.0  15.0
    1509    double    3862.0            31148.0  15.0
    1510    double    4413.0             9894.0  15.0
    <BLANKLINE>
    [1511 rows x 4 columns]
    >>> nadir = df.iloc[:, 1:3].max().to_numpy()
    >>> # Split by column 'algorithm'
    >>> x = [
    ...     g.drop("algorithm", axis=1) for i, g in df.groupby("algorithm", sort=False)
    ... ]
    >>> moocore.largest_eafdiff(x, ref=nadir)
    ((2, 5), 777017.0)

    """
    n = len(x)
    if n == 0:
        raise ValueError("Empty list")
    x = [np.asarray(z, dtype=float) for z in x]
    nobj = x[0].shape[1] - 1
    if nobj != 2:
        raise NotImplementedError("Only 2D datasets are currently supported")

    maximise = _parse_maximise(maximise, nobj)

    if ideal is None:
        ideal = get_ideal(
            np.concatenate(
                [[z[:, :-1].min(axis=0), z[:, :-1].max(axis=0)] for z in x],
                axis=0,
            ),
            maximise=maximise,
        )

    ideal = ideal.reshape(1, -1)

    best_value = 0
    for a in range(n - 1):
        for b in range(a + 1, n):
            diff = eafdiff(
                x[a],
                x[b],
                intervals=intervals,
                maximise=maximise,
                rectangles=True,
            )
            # Set color to 1
            a_rectangles = diff[diff[:, -1] >= 1, :]
            a_rectangles[:, -1] = 1
            a_value = whv_rect(
                ideal, rectangles=a_rectangles, ref=ref, maximise=maximise
            )

            b_rectangles = diff[diff[:, -1] <= -1, :]
            b_rectangles[:, -1] = 1
            b_value = whv_rect(
                ideal, rectangles=b_rectangles, ref=ref, maximise=maximise
            )

            value = min(a_value, b_value)
            if value > best_value:
                best_value = value
                best_pair = (a, b)

    return best_pair, best_value


def whv_hype(
    data: ArrayLike,
    /,
    *,
    ref: ArrayLike,
    ideal: ArrayLike,
    maximise: bool | list[bool] = False,
    nsamples: int = 100000,
    dist: Literal["uniform", "point", "exponential"] = "uniform",
    seed: int | np.random.Generator | None = None,
    mu: float | ArrayLike | None = None,
) -> float:
    r"""Approximation of the (weighted) hypervolume by Monte-Carlo sampling (2D only).

    Return an estimation of the hypervolume of the space dominated by the input
    data following the procedure described by :footcite:t:`AugBadBroZit2009gecco`. A weight
    distribution describing user preferences may be specified.

    .. warning::
        The current implementation only supports 2 objectives.

    Parameters
    ----------
    data :
        Numpy array of numerical values, where each row gives the coordinates of a point in objective space.
        If the array is created from the :func:`read_datasets` function, remove the last (set) column.

    ref :
        Reference point as a numpy array or list. Must have same length as the number of columns of the dataset.

    ideal :
        Ideal point as a numpy array or list. Must have same length as the number of columns of the dataset.

    maximise :
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of booleans, with one value per objective.
        Also accepts a 1D numpy array with values 0 or 1 for each objective.

    nsamples :
        Number of samples for Monte-Carlo sampling. Higher values give more
        accurate approximation of the true hypervolume but require more time.

    seed :
        Either an integer to seed :func:`numpy.random.default_rng`, Numpy
        default random number generator (RNG) or an instance of a
        Numpy-compatible RNG. ``None`` uses the equivalent of a random seed
        (see :func:`numpy.random.default_rng`).

    dist :
        Weight distribution :footcite:p:`AugBadBroZit2009gecco`. The ones currently supported are:

       ``'uniform'``
         corresponds to the default hypervolume (unweighted). ``mu`` should be ``None``.

       ``'point'``
         describes a goal in the objective space, where ``mu`` gives the
         coordinates of the goal (1d Numpy array). The resulting weight distribution is a
         multivariate normal distribution centred at the goal.

       ``'exponential'``
         describes an exponential distribution with rate parameter ``1/mu``, i.e., :math:`\lambda = \frac{1}{\mu}`.

    mu :
       Parameter of ``dist``. See above for details.


    Returns
    -------
       A single numerical value, the weighted hypervolume.


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
    >>> moocore.whv_hype([[2, 2]], ref=4, ideal=1, dist="exponential", mu=0.2, seed=42)
    1.14624
    >>> moocore.whv_hype([[3, 1]], ref=4, ideal=1, dist="exponential", mu=0.2, seed=42)
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
    # np.asarray to convert it to floating-point, otherwise if a user inputs
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


def apply_within_sets(
    x: ArrayLike, sets: ArrayLike, func: Callable[..., Any], **kwargs
) -> np.ndarray:
    """Split ``x`` by row according to ``sets`` and apply ``func`` to each sub-array.

    Parameters
    ----------
    x :
        2D array to be divided into sub-arrays.
    sets :
        A list or 1D array of length equal to the number of rows of ``x``. The values are used as-is to determine the groups and do not need to be sorted.
    func :
        A function that can take a 2D array as input. This function may return (1) a 2D array with the same number of rows as the input,
        (2) a 1D array as long as the number of input rows,
        (3) a scalar value,  or
        (4) a 2D array with a single row.

    kwargs :
        Additional keyword arguments to ``func``.

    Returns
    -------
        An array whose shape depends on the output of ``func``. See Examples below.

    See Also
    --------
    is_nondominated_within_sets, filter_dominated_within_sets

    Examples
    --------
    >>> sets = np.array([3, 1, 2, 4, 2, 3, 1])
    >>> x = np.arange(len(sets) * 2).reshape(-1, 2)
    >>> x = np.hstack((x, sets.reshape(-1, 1)))

    If ``func`` returns an array with the same number of rows as the input (case 1),
    then the output is ordered in exactly the same way as the input.

    >>> moocore.apply_within_sets(x, sets, lambda x: x)
    array([[ 0,  1,  3],
           [ 2,  3,  1],
           [ 4,  5,  2],
           [ 6,  7,  4],
           [ 8,  9,  2],
           [10, 11,  3],
           [12, 13,  1]])

    This is also the behavior if ``func`` returns a 1D array with one value per input row (case 2).

    >>> moocore.apply_within_sets(x, sets, lambda x: x.sum(axis=1))
    array([ 4,  6, 11, 17, 19, 24, 26])

    If ``func`` returns a single scalar (case 3) or a 2D array with a single row (case 4),
    then the order of the output is the order of the unique values as found in
    ``sets``, without sorting the unique values, which is what
    :meth:`pandas.Series.unique` returns and NOT what :func:`numpy.unique`
    returns.

    >>> moocore.apply_within_sets(x, sets, lambda x: x.max())
    array([11, 13,  9,  7])

    >>> moocore.apply_within_sets(x, sets, lambda x: [x.max(axis=0)])
    array([[10, 11,  3],
           [12, 13,  1],
           [ 8,  9,  2],
           [ 6,  7,  4]])

    In the previous example, ``func`` returns a 2D array with a single row. The
    following will produce an error because it returns a 1D array, which is
    interpreted as case 2, but the number of values does not match the number
    of input rows.

    >>> moocore.apply_within_sets(
    ...     x, sets, lambda x: x.max(axis=0)
    ... )  # doctest: +ELLIPSIS
    Traceback (most recent call last):
        ...
    ValueError: `func` returned an array of length 3 but the input has length 2 for rows [0 5]

    """
    x = np.asarray(x)
    _, idx, inv = np.unique(sets, return_index=True, return_inverse=True)
    # Remember the original position of each element of each set.
    idx = [np.flatnonzero(inv == i) for i in idx.argsort()]
    res = []
    shorter = False
    for g_idx in idx:
        z = func(x.take(g_idx, axis=0), **kwargs)
        z = np.atleast_1d(z)
        if len(z) != len(g_idx):
            if len(z) != 1:
                raise ValueError(
                    f"`func` returned an array of length {len(z)} but the input has length {len(g_idx)} for rows {g_idx}"
                )
            shorter = True
        res.append(z)

    res = np.concatenate(res)
    if not shorter:
        res = res.take(np.concatenate(idx).argsort(), axis=0)
    return res
