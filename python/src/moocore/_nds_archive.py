from collections.abc import Iterator, Sequence  # For type hints
from typing import Any, ClassVar, Final

import numpy as np
from numpy.typing import ArrayLike  # For type hints

from ._archive import (
    BaseArchive,
    BaseUnboundedArchive,
    NDTreeArchive,
    TreapArchive,
    _remember_handle,
)
from ._docsubstitute import DocSubstitute

## The CFFI library is used to create C bindings.
from ._libmoocore import ffi, lib  # ty: ignore[import-not-found]
from ._utils import _as_2d_float_array


class BaseNDSForest(BaseArchive):
    """Interface implemented by objects returned by :func:`NDSortedArchive`.

    This class defines the common interface for ``BaseNDSForest`` objects.  It is
    not intended to be instantiated directly; use :func:`NDSortedArchive` to
    create an NDS archive.

    """

    # Derived classes must define these.
    _ArchiveClass: ClassVar[BaseUnboundedArchive]

    _lib_archive_free = lib.nds_forest_free

    # This must be keep synchronized with enum nds_forest_insertion_strategy_t.
    # When the minimum version is Python 3.15, use a frozendict.
    _strategy_to_c_enum: Final[dict[str, int]] = {
        "linear": lib.LINEAR,
        "front-binary": lib.FRONT_BINARY,
        "previous-rank": lib.PREVIOUS_RANK,
        "zero-then-binary": lib.ZERO_THEN_BINARY,
        "dominance-guided-binary": lib.DOMINANCE_GUIDED_BINARY,
    }
    RANK_QUERY_STRATEGIES: Final[tuple[str, ...]] = tuple(
        _strategy_to_c_enum.keys()
    )
    """Names of the supported query strategies."""

    def __init__(
        self,
        dim: int,
        *,
        maximise: bool | Sequence[bool],
        query_strategy: str,
        has_x_values: bool,
        do_timings: bool,
    ):
        self.do_timings = do_timings
        self.query_strategy = query_strategy
        if query_strategy not in self.RANK_QUERY_STRATEGIES:
            raise ValueError(
                "'query_strategy' must be one of: "
                + ", ".join(sorted(self.RANK_QUERY_STRATEGIES))
            )

        # self._c_archive is setup by super().__init__().
        super().__init__(
            dim=dim,
            maximise=maximise,
            keep_duplicates=True,
            has_x_values=has_x_values,
        )

    def __len__(self) -> int:
        """Return the total number of stored points across all fronts."""
        return sum(self.fronts_total_sizes())

    @property
    def len_fronts(self) -> int:
        """Return the number of fronts."""
        return lib.nds_forest_len(self._c_archive)

    def _last_rank(self) -> int:
        return lib.nds_forest_last_rank(self._c_archive)

    def _check_rank(self, rank):
        if rank < 0 or rank >= self.len_fronts:
            raise ValueError(f"invalid rank: {rank}")

    def front_total_size(self, rank: int) -> int:
        """Return the total size of a front (including duplicated points)."""
        self._check_rank(rank)
        return lib.nds_forest_front_total_size(self._c_archive, rank)

    def front_unique_size(self, rank: int) -> int:
        """Return the number of unique points in a front."""
        self._check_rank(rank)
        return lib.nds_forest_front_unique_size(self._c_archive, rank)

    def fronts_total_sizes(self) -> list[int]:
        """Return the number of total points stored in each front."""
        return [self.front_total_size(rank) for rank in range(self.len_fronts)]

    def fronts_unique_sizes(self) -> list[int]:
        """Return the number of unique points stored in each front."""
        return [self.front_unique_size(rank) for rank in range(self.len_fronts)]

    def get_timings(self) -> tuple[float, float]:
        """
        Return query/update times (if timing is enabled).

        Returns
        -------
        A tuple ``(query_time, update_time)`` if the archive was created with ``do_timings=True``. Otherwise, raise ``ValueError``.

        """
        if not self.do_timings:
            raise ValueError("This archive was created with `do_timings=False`")
        query_time_p = ffi.new("double *")
        update_time_p = ffi.new("double *")
        lib.nds_forest_get_timings(self._c_archive, query_time_p, update_time_p)
        return query_time_p[0], update_time_p[0]

    # FIXME: We should also have:
    # def dominates(self, objectives):
    # def dominated_by(self, objectives):

    # FIXME: add options return_x_values, return_n to return x values and/or number of duplicates.
    def index(self, objectives) -> int:
        """Return the index of the front (rank) that contains ``objectives``.  Raises ``ValueError`` if not found, similar to ``list.index()``."""
        z_c = self._objectives_to_c(objectives)
        rank = lib.nds_forest_find_exact_vector(self._c_archive, z_c)
        if rank < self.len_fronts:
            return rank
        raise ValueError("Objective vector not found")

    def __contains__(self, z):
        """Return true if the given objective vector is archived."""
        # FIXME: This is similar to self.index(z) but we do not need to catch the exception.
        z_c = self._objectives_to_c(z)
        rank = lib.nds_forest_find_exact_vector(self._c_archive, z_c)
        return rank < self.len_fronts

    def front_dominated_by(self, rank, objectives):
        """Return ``True`` if all points in the ``rank``-th front are dominated by ``z``."""
        self._check_rank(rank)
        z_c = self._objectives_to_c(objectives)
        return lib.nds_forest_dominated_by(self._c_archive, rank, z_c)

    def front_dominates(self, rank, z) -> bool:
        """Return ``True`` if the ``rank``-th front dominates ``z``."""
        self._check_rank(rank)
        z_c = self._objectives_to_c(z)
        return lib.nds_forest_dominates(self._c_archive, rank, z_c)

    def add(self, objectives: ArrayLike, x: Any = None) -> int:
        """Insert a point in the archive and return its Pareto rank.

        This operation always succeeds if there is enough memory. Adding the
        new point may increase the number of fronts.

        This operation stores a copy of ``objectives``, so modifying it after
        adding it does not change the copy stored in the archive.  This
        operation does not copy ``x`` only a reference to it, so modifying it
        after the call does change the copy stored in the archive.

        """
        z_c, x_c = self._prepare_to_insert(objectives, x)
        if lib.nds_forest_add(self._c_archive, z_c, x_c) < 0:
            raise MemoryError
        return self._last_rank()

    def add_many(
        self, z: ArrayLike, x: Sequence[Any] | None = None
    ) -> list[int]:
        """Add several points sequentially and return the rank of each point when it was inserted.

        Since the points are added sequentially, later points may change the
        insertion rank of earlier ones.
        """
        # FIXME: We could call pareto_rank on the input sequence to insert the
        # points in an order that ensures no changes of rank, then resort the
        # ranks returned to match the original order of the points.
        z = _as_2d_float_array(z, dim=self.dim)
        x = [None] * len(z) if x is None else list(x)
        if len(x) != len(z):
            raise ValueError(
                "'x' must be either None or a list with one value per input vector"
            )
        return [self.add(point, x[index]) for index, point in enumerate(z)]

    def __repr__(self) -> str:
        z, x = self.get_contents()
        if x is None:
            return str(z)
        return f"{z}\nx = {x}"

    def get_contents(self) -> tuple[list, list | None]:
        """Return the full contents of the archive.

        Returns
        -------
            Tuple with two elements. The first element is a list of Numpy arrays, each element are the objective vectors of each front. These are copies, so they can be modified without affecting the archive. The second element is a list (or ``None`` if ``not self.has_x_values()``), one per front, where each element is a list of x values. These are references, so modifying them does modify the objects stored in the archive.

        """
        z_list = [None] * self.len_fronts
        if self.has_x_values():
            x_list = [None] * self.len_fronts
            for rank in range(self.len_fronts):
                z_list[rank], x_list[rank] = self._c_get_contents(
                    lib.nds_forest_front_at(self._c_archive, rank)
                )
            return z_list, x_list

        for rank in range(self.len_fronts):
            z_list[rank], _ = self._c_get_contents(
                lib.nds_forest_front_at(self._c_archive, rank)
            )
        return z_list, None

    def get_unique_vectors(self) -> list[np.ndarray]:
        """Return the unique objective vectors stored in the archive.

        This function returns copies that can be safely modified without affecting the archive.
        """
        return [
            self._c_get_unique_vectors(
                lib.nds_forest_front_at(self._c_archive, rank)
            )
            for rank in range(self.len_fronts)
        ]

    def get_x_values(self) -> list[list[Any]]:
        """Return the x-values stored in the archive."""
        return [
            self._ArchiveClass._c_get_x_values(
                lib.nds_forest_front_at(self._c_archive, rank)
            )
            for rank in range(self.len_fronts)
        ]

    def _c_get_contents(self, c_front):
        return self._ArchiveClass._c_get_contents(
            c_front, dim=self.dim, minmax=self._minmax
        )

    def _c_get_unique_vectors(self, c_front):
        return self._ArchiveClass._c_get_unique_vectors(
            c_front, dim=self.dim, minmax=self._minmax
        )

    def _prepare_to_insert(self, z: ArrayLike, x: Any):
        """Prepare objectives and x for insertion in the C structure."""
        # The C code will copy the z vectors but not the x objects.
        z_c = self._objectives_to_c(z)

        if x is None:
            x_c = ffi.NULL
        else:
            x_c = ffi.new_handle(x)
            _remember_handle(self._x_handles, x_c)

        return z_c, x_c

    def __iter__(self) -> Iterator[tuple[int, np.ndarray, Any]]:
        """Return front indexes, z vectors and, if available, their associated x object one at a time.

        The z vectors are copies so modifying them does not modify the archive.
        However, for efficiency, iterating does not make copies of the objects.

        Modifying the archive while iterating raises a :exc:`RuntimeError`.
        """
        if self.len_fronts == 0:
            return

        it = lib.nds_forest_iter_new(self._c_archive)
        if it == ffi.NULL:
            raise MemoryError("failed to create iterator")
        it = ffi.gc(it, lib.nds_forest_iter_free)
        rank = ffi.new("size_t *")
        z = np.empty(self._dim)
        z_c = ffi.from_buffer("double []", z)
        x = ffi.new("const void **") if self.has_x_values() else ffi.NULL
        while True:
            rc = lib.nds_forest_iter_next(it, rank, z_c, x)
            if rc == 1:
                x_handle = None if x == ffi.NULL else ffi.from_handle(x[0])
                yield rank[0], z, x_handle
            elif rc == 0:
                break
            else:
                raise RuntimeError(
                    "invalid 'BaseNDSForest' iterator, did you modify the archive?"
                )

    def has_x_values(self) -> bool:
        """Return true if the archive stores 'x' values."""
        if self.len_fronts == 0:
            return False
        return self._ArchiveClass._lib_archive_has_x_values(
            lib.nds_forest_front_at(self._c_archive, 0)
        )


class NDSForestNDTree(BaseNDSForest):
    """Ranked Pareto fronts backed by the C specialised 2D treap front."""

    _ArchiveClass: Final = NDTreeArchive

    def __init__(
        self,
        dim: int,
        *,
        maximise: bool | Sequence[bool],
        query_strategy: str,
        max_children: int = 0,
        max_bucket_size: int = 0,
        has_x_values: bool,
        do_timings: bool,
    ):
        """Create a NDS-Forest archive using ND-Trees."""
        self.max_children, self.max_bucket_size = (
            NDTreeArchive._get_default_config(
                dim, max_children, max_bucket_size
            )
        )
        super().__init__(
            dim=dim,
            maximise=maximise,
            query_strategy=query_strategy,
            has_x_values=has_x_values,
            do_timings=do_timings,
        )

    def _setup_c_archive(self, keep_duplicates: bool, has_x_values: bool):
        strategy = self._strategy_to_c_enum[self.query_strategy]
        x_is_null = not has_x_values
        return lib.nds_forest_ndtree_new(
            self.dim,
            strategy,
            self.max_children,
            self.max_bucket_size,
            x_is_null,
            self.do_timings,
        )


class NDSForestTreap(BaseNDSForest):
    """Ranked Pareto fronts backed by the C specialised 2D treap front."""

    _ArchiveClass: Final = TreapArchive

    def __init__(
        self,
        maximise: bool | Sequence[bool],
        query_strategy: str,
        has_x_values: bool,
        do_timings: bool,
    ):
        """Create an empty C-accelerated 2D ranked-front archive."""
        super().__init__(
            dim=2,
            maximise=maximise,
            query_strategy=query_strategy,
            has_x_values=has_x_values,
            do_timings=do_timings,
        )

    def _setup_c_archive(self, keep_duplicates: bool, has_x_values: bool):
        strategy = self._strategy_to_c_enum[self.query_strategy]
        x_is_null = not has_x_values
        return lib.nds_forest_treap_new(strategy, x_is_null, self.do_timings)


@DocSubstitute()
def NDSortedArchive(  # noqa: N802
    dim: int,
    *,
    maximise: bool | Sequence[bool] = False,
    query_strategy: str = "zero-then-binary",
    max_children: int = 0,
    max_bucket_size: int = 0,
    has_x_values: bool = False,
    do_timings: bool = False,
) -> BaseNDSForest:
    """Build an unbounded-size nondominated sorted archive. The type of archive built depends on dim.

    A nondominated sorted archive is a data structure that stores multiple nondominated sets (i.e., `fronts`) and allows efficient updates with new points.  The sets are sorted according to Pareto rank (see :func:`~moocore.pareto_rank`), that is, each point in the archive is associated to the set whose 0-based index is given by nondominating sorting the whole archive. When a new point is added, the point is assigned to the set with the smallest index that does not contain any point dominating the new point. If such set does not exist, the point is added to a new set with index equal to the number of sets in the archive, thus increasing the number of sets in the archive. Points are never rejected by this archive. Duplicated points are always accepted.

    These archives are unbounded, that is, there is no limit in the number of nondominated points that can be stored in the archive.

    The archive can optionally store an arbitrary Python object associated to each point. Duplicated points may be associated to different Python objects.

    When :code:`dim = 2`, the archive is based on a treap data structure :footcite:p:`AraSei1989rst`.
    When :code:`dim > 2`, the archive is based on a ND-Tree :footcite:p:`JasLus2018ndtree`.

    Parameters
    ----------
    dim :
        ${dim}
    maximise :
        ${maximise}
    query_strategy :
       Strategy when searching for the insertion front. Valid values are given by :py:const:`BaseNDSForest.RANK_QUERY_STRATEGIES`: |RANK_QUERY_STRATEGIES|.
    max_children :
        Maximum number of children used in the ND-Tree (only when :code:`dim > 2`).
    max_bucket_size :
        Maximum number of solutions stored in each leaf of the ND-Tree (only when :code:`dim > 2`).
    has_x_values :
        Whether the archive will store a Python object (`x`-value) for each input point (i.e., objective vector).
    do_timings :
        Measure query and update time for NDS-Forest. The values can be retrieved using :func:`~moocore.BaseNDSForest.get_timings`.
        Timing adds some overhead, so it must be explicitly enabled.


    Returns
    -------
        Abstract base class that documents the methods available.


    See Also
    --------
    BaseNDSForest: documents the methods available.


    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> arch = NDSortedArchive(dim=2, has_x_values=True)
    >>> arch.add_many(
    ...     [
    ...         [0, 0],
    ...         [2, 0],
    ...         [
    ...             3,
    ...             3,
    ...         ],
    ...         [2, 0],
    ...     ],
    ...     x=["a", "b", "c", "d"],
    ... )
    [0, 1, 2, 1]
    >>> len(arch)
    4
    >>> arch.len_fronts
    3
    >>> arch.fronts_total_sizes()
    [1, 2, 1]
    >>> arch.fronts_unique_sizes()
    [1, 1, 1]
    >>> arch.add([5, 5], x="e")
    3
    >>> arch
    [array([[0., 0.]]), array([[2., 0.],
           [2., 0.]]), array([[3., 3.]]), array([[5., 5.]])]
    x = [['a'], ['b', 'd'], ['c'], ['e']]

    We can test whether an objective vector has been archived using the ``in`` operator or use ``index()`` to find its rank.

    >>> [3, 3] in arch
    True
    >>> arch.index([3, 3])
    2
    >>> [1, 1] in arch
    False
    >>> arch.index([1, 1])  # doctest: +IGNORE_EXCEPTION_DETAIL +ELLIPSIS
    Traceback (most recent call last):
    ValueError: ...

    We can also dump the contents of the archive:

    >>> arch.get_contents()
    ([array([[0., 0.]]), array([[2., 0.],
           [2., 0.]]), array([[3., 3.]]), array([[5., 5.]])], [['a'], ['b', 'd'], ['c'], ['e']])
    >>> arch.get_unique_vectors()
    [array([[0., 0.]]), array([[2., 0.]]), array([[3., 3.]]), array([[5., 5.]])]
    >>> arch.get_x_values()
    [['a'], ['b', 'd'], ['c'], ['e']]
    >>> for rank, z, x in arch:
    ...     print(f"front {rank}: {x} -> {z}")
    front 0: a -> [0. 0.]
    front 1: b -> [2. 0.]
    front 1: d -> [2. 0.]
    front 2: c -> [3. 3.]
    front 3: e -> [5. 5.]

    The archive can handle maximisation and minimisation objectives:

    >>> arch = NDSortedArchive(dim=3, maximise=[False, True, False])
    >>> arch.add_many([[10, 10, 10], [9, 9, 10], [0, 0, 0]])
    [0, 0, 0]
    >>> arch.add([10, 9, 10])
    1
    >>> arch
    [array([[10., 10., 10.],
           [ 9.,  9., 10.],
           [ 0.,  0.,  0.]]), array([[10.,  9., 10.]])]


    Enable timing of the archive operations:

    >>> arch = NDSortedArchive(dim=2, do_timings=True)
    >>> x = np.random.uniform(size=(5000, 2))
    >>> res = arch.add_many(x)
    >>> query_time, update_time = arch.get_timings()
    >>> print(
    ...     f"query_time = {query_time}, update_time = {update_time}"
    ... )  # doctest: +ELLIPSIS
    query_time = 0.0..., update_time = 0.0...
    """
    if dim > 2:
        return NDSForestNDTree(
            dim,
            maximise=maximise,
            query_strategy=query_strategy,
            max_children=max_children,
            max_bucket_size=max_bucket_size,
            has_x_values=has_x_values,
            do_timings=do_timings,
        )
    if dim == 2:
        return NDSForestTreap(
            maximise=maximise,
            query_strategy=query_strategy,
            has_x_values=has_x_values,
            do_timings=do_timings,
        )
    if dim < 2:
        raise ValueError("'dim' cannot be smaller than 2")

    raise NotImplementedError("This type of archive is not implemented yet")
