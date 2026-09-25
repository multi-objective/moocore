from abc import ABC, abstractmethod
from collections.abc import Callable, Sequence, Iterator
from numpy.typing import ArrayLike  # For type hints
from typing import Any, ClassVar

import numpy as np

from ._docsubstitute import DocSubstitute
from ._utils import _as_1d_float_array, _as_2d_float_array, _parse_maximise

## The CFFI library is used to create C bindings.
from ._libmoocore import lib, ffi


def _remember_handle(handles, what):
    address = int(ffi.cast("uintptr_t", what))
    handles[address] = what


class BaseArchive(ABC):
    _lib_archive_free: ClassVar[Callable[..., Any]]

    def __init__(self, dim: int, maximise: bool | Sequence[bool]):
        if dim < 2 or dim > lib.MOOCORE_DIMENSION_MAX:
            raise ValueError(
                f"'dim' cannot be smaller than 2 or larger than {lib.MOOCORE_DIMENSION_MAX}"
            )

        self._dim = dim
        self._c_archive = self._setup_c_archive()
        if self._c_archive == ffi.NULL:
            raise MemoryError
        # ffi.gc() ensures that the pointer is registered with the GC and the C
        # destructor called.
        self._c_archive = ffi.gc(self._c_archive, self._lib_archive_free)

        self.maximise = np.asarray(
            _parse_maximise(maximise, self._dim), dtype=bool
        )
        # -1: maximize, 1: minimize
        self._minmax = np.where(self.maximise, -1.0, 1.0)

        # Python objects stored in C will be freed by Python unless we keep a
        # reference to them in Python. This dictionary keeps those
        # objects alive until we decide to free or move them to another front.
        self._x_handles = {}
        # FIXME: Using a set() instead of a dictionary is probably faster. We
        # can store the output of ffi.new_handle() directly in the set.

    @property
    def dim(self) -> int:
        """Return the dimension of the points (objective vectors) stored in the archive."""
        return self._dim

    @abstractmethod
    def _setup_c_archive(self):
        pass

    def _objectives_to_c(self, z: ArrayLike):
        z = _as_1d_float_array(z, dim=self.dim, name="z")
        z *= self._minmax
        return ffi.from_buffer("double []", z)

    def _c_to_objectives(self, z) -> np.ndarray:
        a = np.frombuffer(ffi.buffer(z, ffi.sizeof("double") * self.dim))
        a *= self._minmax
        a.setflags(write=False)
        return a


class BaseUnboundedArchive(BaseArchive):
    """Interface implemented by objects returned by :func:`UnboundedArchive`.

    This class defines the common interface for ``BaseUnboundedArchive``
    objects.  It is not intended to be instantiated directly.  See Examples in
    :func:`UnboundedArchive`.

    """

    # Derived classes must define these:
    _lib_archive_add_and_get_displaced: ClassVar[Callable[..., Any]]
    _lib_archive_dominated_by: ClassVar[Callable[..., Any]]
    _lib_archive_dominates: ClassVar[Callable[..., Any]]
    _lib_archive_find_exact_vector: ClassVar[Callable[..., Any]]
    _lib_archive_get_contents: ClassVar[Callable[..., Any]]
    _lib_archive_has_x_values: ClassVar[Callable[..., Any]]
    _lib_archive_new: ClassVar[Callable[..., Any]]
    _lib_archive_total_size: ClassVar[Callable[..., Any]]
    _lib_displaced_free: ClassVar[Callable[..., Any]]
    _lib_displaced_total_len: ClassVar[Callable[..., Any]]
    _lib_displaced_unique_len: ClassVar[Callable[..., Any]]

    def __init__(self, dim: int, maximise: bool | Sequence[bool]):
        super().__init__(dim=dim, maximise=maximise)
        self.all_displaced_p = ffi.new("bool *")

    def __contains__(self, z: ArrayLike) -> bool:
        """Return true if the given objective vector is archived."""
        z = self._objectives_to_c(z)
        return ffi.NULL != self._lib_archive_find_exact_vector(
            self._c_archive, z
        )

    def __iter__(self) -> Iterator[tuple[np.ndarray, Any]]:
        """Return z vectors and, if available, their associated x object one at a time.

        For efficiency, iterating does not make copies of the vectors nor the
        objects. However, the vectors are marked as read-only, thus
        modifying them requires that the user makes a copy.

        Modifying the tree while iterating raises a :exc:`RuntimeError`.
        """
        if len(self) == 0:
            return

        it = self._lib_archive_iter_new(self._c_archive)
        if it == ffi.NULL:
            raise MemoryError("failed to create iterator")
        it = ffi.gc(it, self._lib_archive_iter_free)
        z = ffi.new("const double **")
        rc = 1
        if self.has_x_values():
            x = ffi.new("const void **")
            while True:
                rc = self._lib_archive_iter_next(it, z, x)
                if rc != 1:
                    break
                yield self._c_to_objectives(z[0]), ffi.from_handle(x[0])
        else:
            while True:
                rc = self._lib_archive_iter_next(it, z, ffi.NULL)
                if rc != 1:
                    break
                yield self._c_to_objectives(z[0]), None

        if rc < 0:
            raise RuntimeError(
                "invalid 'BaseUnboundedArchive' iterator, did you modify the tree?"
            )

    def __len__(self) -> int:
        """Return the total number of points stored in the archive (including duplicates)."""
        return self._lib_archive_total_size(self._c_archive)

    def __repr__(self) -> str:
        z, x = self.get_contents()
        if x is None:
            return str(z)
        return f"{z}\nx = {x}"

    def unique_len(self) -> int:
        """Return the unique number of points stored in the archive."""
        return self._lib_archive_unique_size(self._c_archive)

    def has_x_values(self) -> bool:
        """Return true if the archive stores 'x' values."""
        return self._lib_archive_has_x_values(self._c_archive)

    def dominates(self, z: ArrayLike) -> bool:
        """Return true if a point in the archive dominates :code:`z`."""
        z = self._objectives_to_c(z)
        return self._lib_archive_dominates(self._c_archive, z)

    # FIXME: Should this be called is_dominated_by, so you write arch.is_dominated_by(z)?
    def dominated_by(self, z: ArrayLike) -> bool:
        """Return true if all points in the archive are dominated by :code:`z`."""
        z = self._objectives_to_c(z)
        return self._lib_archive_dominated_by(self._c_archive, z)

    def add(self, z: ArrayLike, x: Any = None) -> bool:
        """Add a point if it is not (weakly-)dominated by the archive.

        Points dominated by the added point are discarded. With
        :code:`allow_duplicates=True`, weakly-dominated points are
        accepted. Otherwise, they are rejected.

        Returns ``True`` if added or ``False`` if rejected.
        """
        accepted, _ = self._add_with_displaced_raw(
            z, x, check=True, discard=True
        )
        return accepted

    def add_many(
        self, z: ArrayLike, x: Sequence[Any] | None = None
    ) -> list[bool]:
        """Add several points sequentially and return acceptance as a boolean list."""
        # FIXME: call is_nondominated(points) first. It is much faster than adding
        # them to the archive one by one.
        z = _as_2d_float_array(z, dim=self.dim)
        if x is None:
            return [self.add(point) for point in z]

        x = list(x)
        if len(x) != len(z):
            raise ValueError(
                "'x' must be either None or a list with one value per input vector"
            )
        return [self.add(point, x_value) for point, x_value in zip(z, x)]

    # FIXME: This should be called _add_and_get_displaced()
    def _add_with_displaced_raw(
        self, z: ArrayLike, x: Any, check: bool, discard: bool
    ):
        """Insert an entry and return members displaced into the next Pareto front."""
        # The C code copies z but not x.
        z_c = self._objectives_to_c(z)
        x_c = ffi.NULL if x is None else ffi.new_handle(x)

        # Make sure _lib_archive_add_and_get_displaced() starts from an empty
        # list of displaced.
        self.displaced_p[0] = ffi.NULL
        # FIXME: Add option to discard removed points (so no displaced).
        accepted = self._lib_archive_add_and_get_displaced(
            self._c_archive, z_c, x_c, check, self.displaced_p
        )
        if accepted == lib.ARCHIVE_INSERT_REJECTED:
            return False, None

        if (
            accepted != lib.ARCHIVE_INSERT_ACCEPTED
            and accepted != lib.ARCHIVE_INSERT_DUPLICATED
        ):
            if accepted == lib.ARCHIVE_INSERT_X_MUST_BE_NOT_NULL:
                raise ValueError("'x' cannot be None")
            elif accepted == lib.ARCHIVE_INSERT_X_MUST_BE_NULL:
                raise ValueError("'x' must be None")
            elif accepted == lib.ARCHIVE_INSERT_MEMORY_ERROR:
                raise MemoryError
            else:
                raise NotImplementedError("unknown error")

        if x is not None:
            _remember_handle(self._x_handles, x_c)

        # FIXME: if x is None and discard: return True, None
        displaced = self.displaced_p[0]
        if displaced == ffi.NULL:
            return True, None

        displaced = ffi.gc(displaced, self._lib_displaced_free)
        # FIXME: We know that displaced is non-zero, so the checks are wasted.
        return True, self._process_displaced(displaced, discard=discard)

    def get_contents(self) -> tuple[np.ndarray, list[Any] | None]:
        """Return the full contents of the archive.

        Returns
        -------
            Tuple of objective vectors (as a Numpy array) and a list of x values (or ``None`` if ``not self.has_x_values()``)

        """
        return self._c_get_contents(self._c_archive, self.dim, self._minmax)

    def get_unique_vectors(self) -> np.ndarray:
        """Return the unique objective vectors stored in the archive."""
        return self._c_get_unique_vectors(
            self._c_archive, self.dim, self._minmax
        )

    def get_x_values(self) -> list[Any]:
        """Return the x-values stored in the archive."""
        return self._c_get_x_values(self._c_archive)

    def _get_x_values_addresses_from_displaced(self, displaced):
        if not self.has_x_values():
            return []

        x_list = ffi.new("void *[]", self._lib_displaced_total_len(displaced))
        self._lib_displaced_get_x_values(displaced, x_list)
        addresses = [int(ffi.cast("uintptr_t", x)) for x in x_list]
        return addresses

    def _process_displaced(self, displaced, discard):
        # FIXME: How can displaced be None?
        # FIXME: How can displaced have len == 0? displaced should be NULL in that case.
        if (
            displaced is None
            or displaced == ffi.NULL
            or self._lib_displaced_unique_len(displaced) == 0
        ):
            return None

        # FIXME: This creates a list that we need to traverse again below.
        # Avoid the intermediate list and process the values directly.
        addresses = self._get_x_values_addresses_from_displaced(displaced)
        if discard:
            for k in addresses:
                del self._x_handles[k]
            return None
        else:
            x_handles = {k: self._x_handles.pop(k) for k in addresses}
            return displaced, x_handles

    def _add_displaced_raw(self, displaced):
        if displaced is None:
            return None
        displaced, x_handles = displaced
        displaced, all_displaced = self._lib_archive_insert_displaced(displaced)
        if all_displaced:
            assert self._lib_displaced_unique_len(displaced) > 0
            # Swap handles
            x_handles, self._x_handles = self._x_handles, x_handles
            return displaced, x_handles

        self._x_handles.update(x_handles)
        return self._process_displaced(displaced, discard=False)

    @classmethod
    def _c_get_contents(cls, c_tree, dim, minmax):
        size = cls._lib_archive_total_size(c_tree)
        z_list = ffi.new("double []", dim * size)
        if cls._lib_archive_has_x_values(c_tree):
            x_list = ffi.new("void *[]", size)
            cls._lib_archive_get_contents(c_tree, z_list, x_list)
            x_list = [ffi.from_handle(x) for x in x_list]
        else:
            cls._lib_archive_get_contents(c_tree, z_list, ffi.NULL)
            x_list = []

        z_list = np.frombuffer(ffi.buffer(z_list)).reshape(size, dim)
        z_list *= minmax
        return z_list, x_list

    @classmethod
    def _c_get_unique_vectors(cls, c_archive, dim, minmax):
        # FIXME: It would be faster to return a single block of memory, but we
        # need a new function to get the pointer addresses.
        n = cls._lib_archive_unique_size(c_archive)
        if n == 0:
            return []
        z_list = ffi.new("double *[]", n)
        cls._lib_archive_get_unique_vectors(c_archive, z_list)
        z_array = np.vstack(
            [
                np.frombuffer(ffi.buffer(z, ffi.sizeof("double") * dim))
                for z in z_list
            ]
        )
        z_array *= minmax
        return z_array

    @abstractmethod
    def _lib_archive_insert_displaced(self, displaced) -> bool:
        pass


class NDTreeArchive(BaseUnboundedArchive):
    # C functions
    _lib_archive_add_and_get_displaced = lib.ndtree_add
    _lib_archive_dominated_by = lib.ndtree_dominated_by
    _lib_archive_dominates = lib.ndtree_dominates
    _lib_archive_find_exact_vector = lib.ndtree_find_exact_vector
    _lib_archive_free = lib.ndtree_free
    _lib_archive_get_contents = lib.ndtree_get_contents
    _lib_archive_get_unique_vectors = lib.ndtree_get_unique_vectors
    _lib_archive_has_x_values = lib.ndtree_has_x_values
    _lib_archive_new = lib.ndtree_new
    _lib_archive_total_size = lib.ndtree_total_size
    _lib_archive_unique_size = lib.ndtree_unique_size
    _lib_archive_iter_new = lib.ndtree_iter_new
    _lib_archive_iter_free = lib.ndtree_iter_free
    _lib_archive_iter_next = lib.ndtree_iter_next
    _lib_displaced_free = lib.flex_bucket_free
    _lib_displaced_get_unique_vectors = lib.flex_bucket_get_unique_vectors
    _lib_displaced_get_x_values = lib.flex_bucket_get_x_values
    _lib_displaced_total_len = lib.flex_bucket_total_len
    _lib_displaced_unique_len = lib.flex_bucket_unique_len

    def __init__(
        self,
        dim: int,
        maximise: bool | Sequence[bool] = False,
        max_children: int = 0,
        max_bucket_size: int = 0,
        allow_duplicates: bool = True,
    ):

        self.max_children, self.max_bucket_size, self.allow_duplicates = (
            self._get_default_config(
                dim, max_children, max_bucket_size, allow_duplicates
            )
        )
        super().__init__(dim=dim, maximise=maximise)
        # We cache these pointers here so we don't keep allocating and freeing memory.
        self.displaced_p = ffi.new("FlexBucket **")

    # This is a @classmethod so we can use it from NDSForestNDTree
    @classmethod
    def _get_default_config(
        cls,
        dim,
        max_children: int,
        max_bucket_size: int,
        allow_duplicates: bool,
    ):
        # FIXME: This duplicates the logic in C, it would be better to call a C
        # function.
        max_children = max(dim + 1, max_children)
        if max_children >= 255:
            raise ValueError("'max_children' must be smaller than 255")

        if max_bucket_size < max_children:
            max_bucket_size = 20
        if max_bucket_size >= 255:
            raise ValueError("'max_bucket_size' must be smaller than 255")

        return max_children, max_bucket_size, bool(allow_duplicates)

    def _setup_c_archive(self):
        # ffi.gc() ensures that the pointer is registered with the GC and the C
        # destructor called.
        return self._lib_archive_new(
            self.dim,
            self.max_children,
            self.max_bucket_size,
            self.allow_duplicates,
        )

    @staticmethod
    def _c_get_x_values(c_archive):
        if not lib.ndtree_has_x_values(c_archive):
            return []
        x_list = ffi.new("void *[]", lib.ndtree_total_size(c_archive))
        lib.ndtree_get_x_values(c_archive, x_list)
        x_list = [ffi.from_handle(x) for x in x_list]
        return x_list

    def _lib_archive_insert_displaced(self, displaced):
        # lib.ndtree_insert_displaced() will free the current displaced, so
        # Python does not need to do it.
        ffi.gc(displaced, None)
        self.displaced_p[0] = displaced
        lib.ndtree_insert_displaced(
            self._c_archive, self.displaced_p, self.all_displaced_p
        )
        assert displaced != self.displaced_p[0]
        displaced = self.displaced_p[0]
        if displaced != ffi.NULL:
            # lib.ndtree_insert_displaced() may create new memory, so we need ffi.gc().
            displaced = ffi.gc(displaced, self._lib_displaced_free)

        return displaced, self.all_displaced_p[0]


class TreapArchive(BaseUnboundedArchive):
    # C functions
    _lib_archive_add_and_get_displaced = lib.treap_archive_add
    _lib_archive_dominated_by = lib.treap_archive_dominated_by
    _lib_archive_dominates = lib.treap_archive_dominates
    _lib_archive_find_exact_vector = lib.treap_archive_find_exact_vector
    _lib_archive_free = lib.treap_archive_free
    _lib_archive_get_contents = lib.treap_archive_get_contents
    _lib_archive_get_unique_vectors = lib.treap_archive_get_unique_vectors
    _lib_archive_has_x_values = lib.treap_archive_has_x_values
    _lib_archive_new = lib.treap_archive_new
    _lib_archive_total_size = lib.treap_archive_total_size
    _lib_archive_unique_size = lib.treap_archive_unique_size
    _lib_archive_iter_new = lib.treap_archive_iter_new
    _lib_archive_iter_free = lib.treap_archive_iter_free
    _lib_archive_iter_next = lib.treap_archive_iter_next
    _lib_displaced_free = lib.treap_archive_free
    _lib_displaced_get_unique_vectors = lib.treap_archive_get_unique_vectors
    _lib_displaced_get_x_values = lib.treap_archive_get_x_values
    _lib_displaced_total_len = lib.treap_archive_total_size
    _lib_displaced_unique_len = lib.treap_archive_unique_size

    def __init__(self, maximise: bool | Sequence[bool]):
        super().__init__(dim=2, maximise=maximise)
        # We cache these pointers here so we don't keep allocating and freeing memory.
        self.displaced_p = ffi.new("TreapArchive **")

    def _setup_c_archive(self):
        return self._lib_archive_new()

    @staticmethod
    def _c_get_x_values(c_archive):
        if not lib.treap_archive_has_x_values(c_archive):
            return []
        x_list = ffi.new("void *[]", lib.treap_archive_total_size(c_archive))
        lib.treap_archive_get_x_values(c_archive, x_list)
        x_list = [ffi.from_handle(x) for x in x_list]
        return x_list

    def _lib_archive_insert_displaced(self, displaced) -> bool:
        # lib.treap_archive_insert_displaced() does not create new memory, so no ffi.gc().
        lib.treap_archive_insert_displaced(
            self._c_archive, displaced, self.all_displaced_p
        )
        return displaced, self.all_displaced_p[0]


# FIXME: implement maximise
@DocSubstitute()
def UnboundedArchive(  # noqa: N802
    dim: int,
    *,
    maximise: bool | Sequence[bool] = False,
    max_children: int = 0,
    max_bucket_size: int = 0,
    allow_duplicates: bool = True,
) -> BaseArchive:
    """Create an empty unbounded-size nondominated archive. The type of archive built depends on dim.

    A nondominated archive is a data structure that stores only nondominated points and allows efficient updates with new points.  When a new point is added, if the new point is (weakly)-dominated by an existing point in the archive, the point would be rejected. If the new point is not (weakly)-dominated, it will be accepted. Points in the archive (weakly)-dominated by the new one will be removed.  If the archive allows duplicates, then duplicated points, i.e., weakly-dominated but not dominated, are allowed in the archive.

    These archives are unbounded, that is, there is no limit in the number of nondominated points that can be stored in the archive.

    The archive can optionally store an arbitrary Python object associated to each point. When the archive allows duplicates, duplicated points may be associated to different Python objects.

    When :code:`dim = 2`, the archive is based on a treap data structure :footcite:p:`AraSei1989rst`.
    When :code:`dim > 2`, the archive is based on a ND-Tree :footcite:p:`JasLus2018ndtree`.

    Parameters
    ----------
    dim :
        ${dim}
    maximise :
        ${maximise}
    max_children :
        Maximum number of children used in the ND-Tree (only when :code:`dim > 2`).
    max_bucket_size :
        Maximum number of solutions stored in each leaf of the ND-Tree (only when :code:`dim > 2`).
    allow_duplicates :
        Whether to allow or reject duplicated input points (i.e., objective vectors).

    Returns
    -------
        Abstract base class.

    See Also
    --------
    BaseArchive: documents the methods available.

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> arch = UnboundedArchive(dim=2)
    >>> arch.add_many([[10, 10], [5, 20], [20, 5]])
    [True, True, True]
    >>> arch.add([11, 11])
    False

    The ``len()`` function applied to the archive returns the number of points:

    >>> len(arch)
    3
    >>> arch.add([5, 5])
    True
    >>> len(arch)
    1
    >>> arch.add_many([[6, 4], [4, 6]])
    [True, True]

    We can also iterate over the contents:

    >>> for z, _ in arch:
    ...     print(z)
    [4. 6.]
    [5. 5.]
    [6. 4.]


    It is possible to associate `x`-values to each point.

    >>> arch = UnboundedArchive(3)
    >>> arch.add_many([[10, 10, 10], [5, 5, 20], [20, 5, 5]], x=["a", "b", "c"])
    [True, True, True]
    >>> len(arch)
    3
    >>> arch.add([9, 9, 9], x="d")
    True
    >>> arch
    [[20.  5.  5.]
     [ 5.  5. 20.]
     [ 9.  9.  9.]]
    x = ['c', 'b', 'd']

    The default is to allow duplicates:

    >>> arch.add([9, 9, 9], x="e")
    True
    >>> arch
    [[20.  5.  5.]
     [ 5.  5. 20.]
     [ 9.  9.  9.]
     [ 9.  9.  9.]]
    x = ['c', 'b', 'd', 'e']


    The ``in`` keyword can be used to test if a point is in the archive:

    >>> [9, 9, 9] in arch
    True
    >>> [10, 10, 10] in arch
    False

    The contents of the archive can be obtained in different ways:

    >>> arch.get_contents()
    (array([[20.,  5.,  5.],
           [ 5.,  5., 20.],
           [ 9.,  9.,  9.],
           [ 9.,  9.,  9.]]), ['c', 'b', 'd', 'e'])
    >>> arch.get_unique_vectors()
    array([[20.,  5.,  5.],
           [ 5.,  5., 20.],
           [ 9.,  9.,  9.]])
    >>> arch.get_x_values()
    ['c', 'b', 'd', 'e']
    >>> for z, x in arch:
    ...     print(f"{x} -> {z}")
    c -> [20.  5.  5.]
    b -> [ 5.  5. 20.]
    d -> [9. 9. 9.]
    e -> [9. 9. 9.]

    The vectors returned by iterating are read-only:

    >>> for z, x in arch:  # doctest: +IGNORE_EXCEPTION_DETAIL +ELLIPSIS
    ...     z[0] = 1
    Traceback (most recent call last):
    ValueError: assignment destination is read-only

    We can also mix maximisation and minimisation objectives:

    >>> arch = UnboundedArchive(dim=2, maximise=[False, True])
    >>> arch.add_many([[10, 10], [9, 9], [0, 0]])
    [True, True, True]
    >>> arch.add([10, 9])
    False
    >>> arch.dominated_by([0, 11])
    True
    >>> arch.dominates([10, 11])
    False
    >>> arch
    [[ 0.  0.]
     [ 9.  9.]
     [10. 10.]]
    x = []

    """
    if dim > 2:
        return NDTreeArchive(
            dim,
            maximise=maximise,
            max_children=max_children,
            max_bucket_size=max_bucket_size,
            allow_duplicates=allow_duplicates,
        )
    if dim == 2:
        return TreapArchive(maximise=maximise)
    if dim < 2:
        raise ValueError("'dim' cannot be smaller than 2")

    raise NotImplementedError("This type of archive is not implemented yet")
