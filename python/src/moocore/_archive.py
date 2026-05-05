from collections.abc import Sequence
from numpy.typing import ArrayLike  # For type hints
from typing import Any

import numpy as np

from ._moocore import (
    hypervolume,
    hv_contributions,
    _parse_maximise,
)

from ._utils import (
    array_1d_of_length_n,
)


class UnboundedArchive:
    r"""Archive that accepts an unlimited number of solutions.

    Parameters
    ----------
    f :
         Matrix of numerical values, where each row gives the coordinates of a point, typically in objective space.
    x :
         List of objects associated to points. These are not copied for efficiency, so modifying the original may modify the copy stored in the archive.
    m :
       Number of dimensions of the input points to be stored. Only needed if 'f' is not given.
    maximise :
        Whether the objectives must be maximised instead of minimised.
        Either a single boolean value that applies to all objectives or a list of boolean values, with one value per objective.
        Also accepts a 1D numpy array with value 0/1 for each objective.
    online_metrics :
        List of unary indicators that will be updated online when adding/removing points. Supported values are TODO.
    ref_point :
        Reference point for hypervolume computation. Required if ``"hypervolume"`` appears in ``online_metrics``.


    Examples
    --------
    The archive can be initialized with a set of points.

    >>> moa2obj = moocore.UnboundedArchive(
    ...     [[1, 5], [2, 3], [4, 5], [5, 0]], xvals=["a", "b", "c", "d"]
    ... )
    >>> moa3obj = moocore.UnboundedArchive(
    ...     [[1, 2, 3], [3, 2, 1], [3, 3, 0], [2, 2, 1]],
    ...     xvals=["a", "b", "c", "d"],
    ... )
    >>> moa4obj = moocore.UnboundedArchive(
    ...     [[1, 2, 3, 4], [1, 3, 4, 5], [4, 3, 2, 1], [1, 3, 0, 1]],
    ...     xvals=["a", "b", "c", "d"],
    ... )
    >>> print("points in the 2 objective archive:", list(moa2obj))
    points in the 2 objective archive: [array([1, 5]), array([2, 3]), array([5, 0])]
    >>> print("points in the 3 objective archive:", list(moa3obj))
    points in the 3 objective archive: [array([1, 2, 3]), array([3, 3, 0]), array([2, 2, 1])]
    >>> print("points in the 4 objective archive:", list(moa4obj))
    points in the 4 objective archive: [array([1, 2, 3, 4]), array([1, 3, 0, 1])]

    Archive objects can also be initialized empty, but you need to specify `m`, the dimension of the points.

    >>> moa = UnboundedArchive(m=3)
    >>> print("points in the empty archive:", list(moa))
    points in the empty archive: []

    Points can be added to the archive one at a time or several at once as a
    list or a 2D numpy array).  The data associated with each input points is
    stored in `archive.xvals` as a list.

    >>> moa.add([1, 2, 3], "a")
    >>> print(f"points: {list(moa)}\nxvals : {moa.xvals}")
    points: [array([1, 2, 3])]
    xvals : ['a']
    >>> moa.add([[3, 2, 1], [2, 3, 2], [2, 2, 2]], ["b", "c", "d"])
    >>> print(f"points: {list(moa)}\nxvals : {moa.xvals}")
    points: [array([1, 2, 3]), array([3, 2, 1]), array([2, 2, 2])]
    xvals : ['a', 'b', 'd']

    The `len()` function applied to the archive returns the number of points:

    >>> len(moa)
    3

    The `in` keyword can be used to test if a point is in the archive:

    >>> [2, 2, 2] in moa
    True
    >>> [3, 2, 0] in moa
    False

    """

    def __init__(
        self,
        f: ArrayLike = [],
        /,
        x: Sequence[Any] | None = None,
        *,
        m: int = 0,
        maximise: bool | Sequence[bool] = False,
        online_metrics=None,
        ref_point: ArrayLike | None = None,
    ):
        self.fvals = np.atleast_2d(f)
        # np.atleast_2d([]).shape is (1,0), so we may need to correct it.
        self._dim = self.fvals.shape[1]
        if self._dim < 2:
            if m < 2:
                raise ValueError("either 'f' or 'm' must be provided")
            self.fvals = []
            self._dim = m

        self.xvals = [] if x is None else x
        self.maximise = _parse_maximise(maximise, self._dim())
        nrows = len(self.fvals)
        if nrows > 0:
            if len(self.xvals):
                assert len(self.fvals) == len(self.xvals)
        self.is_nondom = nrows == 0
        self._filter_dominated()
        if "hypervolume" in online_metrics:
            # Make sure it is a 1D array of length nobj.
            self._ref_point = array_1d_of_length_n(
                np.asarray(ref_point, dtype=float), self._dim, name="ref_point"
            )

    def add(
        self,
        f: ArrayLike,
        /,
        *,
        x: Sequence[Any] | None = None,
        check: bool = True,
    ) -> bool | Sequence[bool]:
        """Add one or more points to the archive.

        Parameters
        ----------
        f :
             One or more input points.
        x :
            Optional objects (e.g., decision vectors) associated to the input points.
        check :
             If ``False``, assume that points are mutually nondominated.


        """
        self._push_back(f, x)
        self._filter_dominated()

    def to_numpy(self) -> np.ndarray:
        """Return the contents of the archive as a NumPy array.

        If the archive stores associated ``x`` objects, they are returned as a second argument.
        """
        if len(self.xvals):
            return np.array(self.fvals), self.xvals
        return np.array(self.fvals)

    def __len__(self):
        return len(self.fvals)

    def max_len(self):
        return np.inf

    def __iter__(self):
        return iter(self.fvals)

    # TODO: deepcopy?
    def copy(self):
        pass

    def dim(self):
        return self._dim

    def hypervolume(self):
        """Return the hypervolume of the archive."""
        if self._hypervolume is None:
            if self.ref is None:
                raise ValueError(
                    "Computing the hypervolume requires a reference point"
                )
            self._hypervolume = hypervolume(
                self.fvals, ref=self.ref, maximise=self.maximise
            )
        return self._hypervolume

    def hv_contributions(self):
        """Return the hypervolume contribution of the points in the archive."""
        if self._hvc is None:
            if self.ref is None:
                raise ValueError(
                    "Computing the hypervolume requires a reference point"
                )
            self._hvc = hv_contributions(
                self.fvals, ref=self.ref, maximise=self.maximise
            )
        return self._hvc

    def dominates(self, f: ArrayLike) -> Sequence[bool]:
        pass

    def weakly_dominates(self, f: ArrayLike) -> Sequence[bool]:
        pass

    def one_hv_contribution(self, f: ArrayLike) -> Sequence[float]:
        """Return the hypervolume contribution of the points given as input with respect to the archive."""
        pass

    def optimal_for_chebychev(self, w: ArrayLike, ref: ArrayLike) -> np.ndarray:
        """Return point in the archive that is optimal with respect the given weight vector."""
        pass

    def optimal_for_weighted_sum(self, w: ArrayLike) -> np.ndarray:
        """Return point in the archive that is optimal with respect the given weight vector."""
        pass
