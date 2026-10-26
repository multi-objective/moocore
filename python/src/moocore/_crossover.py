from collections.abc import Callable
from typing import Any

import numpy as np
from numpy.typing import ArrayLike

from ._docsubstitute import DocSubstitute
from ._libmoocore import ffi, lib # ty: ignore[unresolved-import]
from ._utils import _get_seed_for_c, array_1d_of_length_n, np2d_to_double_array


def _as_matrix(x: ArrayLike, name: str, min_rows: int) -> np.ndarray:
    x = np.asarray(x, dtype=float)
    if x.ndim != 2 or x.shape[1] == 0:
        raise ValueError(f"{name!r} must be a 2D array with at least 1 column")
    if x.shape[0] < min_rows:
        raise ValueError(f"{name!r} must have at least {min_rows} rows")
    return x


def _check_probability(value: float, name: str) -> float:
    if not 0 <= value <= 1:
        raise ValueError(f"{name!r} must be within [0, 1]")
    return float(value)


def _check_positive(value: float, name: str) -> float:
    if not value > 0:
        raise ValueError(f"{name!r} must be larger than 0")
    return float(value)


def _bounds(
    lower: ArrayLike | None, upper: ArrayLike | None, nvar: int
) -> tuple[np.ndarray, np.ndarray]:
    if lower is None:
        lower = np.full(nvar, -np.inf)
    else:
        lower = array_1d_of_length_n(
            np.asarray(lower, dtype=float), nvar, name="lower"
        )
    if upper is None:
        upper = np.full(nvar, np.inf)
    else:
        upper = array_1d_of_length_n(
            np.asarray(upper, dtype=float), nvar, name="upper"
        )
    if np.any(lower > upper):
        raise ValueError("'lower' must not be larger than 'upper'")
    return np.ascontiguousarray(lower), np.ascontiguousarray(upper)


def _check_within_bounds(
    parents: np.ndarray, lower: np.ndarray, upper: np.ndarray
) -> None:
    # NaN fails both comparisons, so it is rejected too.
    if not np.all((parents >= lower) & (parents <= upper)):
        raise ValueError("'parents' must be within 'lower' and 'upper'")


def _targets(targets: ArrayLike | None, n: int) -> np.ndarray:
    if targets is None:
        return np.arange(n, dtype=np.uintp)
    targets = np.atleast_1d(targets)
    if targets.ndim != 1 or (
        targets.size > 0 and not np.issubdtype(targets.dtype, np.integer)
    ):
        raise ValueError("'targets' must be a 1D vector of row indices")
    if np.any((targets < 0) | (targets >= n)):
        raise ValueError(f"'targets' must be row indices within [0, {n - 1}]")
    return np.ascontiguousarray(targets, dtype=np.uintp)


def _call(
    fun: Callable[..., None], x: np.ndarray, nrows: int, *args: Any
) -> np.ndarray:
    """Call the C function ``fun`` and return the offspring it writes."""
    out = np.empty((nrows, x.shape[1]), dtype=float)
    x_p, n, nvar = np2d_to_double_array(x, ctype_shape=("size_t", "size_t"))
    fun(ffi.from_buffer("double []", out), x_p, n, nvar, *args)
    return out


def _keep_dtype(out: np.ndarray, dtype: np.dtype) -> np.ndarray:
    if np.issubdtype(dtype, np.integer) or np.issubdtype(dtype, np.bool_):
        return out.astype(dtype)
    return out


def _call_with_targets(
    fun: Callable[..., None],
    population: np.ndarray,
    targets: np.ndarray,
    lower: np.ndarray,
    upper: np.ndarray,
    *args: Any,
) -> np.ndarray:
    return _call(
        fun,
        population,
        len(targets),
        ffi.from_buffer("size_t []", targets),
        len(targets),
        ffi.from_buffer("double []", lower),
        ffi.from_buffer("double []", upper),
        *args,
    )


@DocSubstitute()
def uniform_crossover(
    parents: ArrayLike,
    /,
    *,
    swap_prob: float = 0.5,
    pair_prob: float = 1.0,
    seed: int | np.random.Generator | None = None,
) -> np.ndarray:
    r"""Uniform crossover.

    Each crossover operation on a pair of parents produces two offspring.  With
    probability ``pair_prob``, each variable is swapped between the two
    offspring with probability ``swap_prob``.  Otherwise, the offspring are
    copies of their parents.

    The offspring only take values from their parents, so this operator works
    for binary, integer and real-valued variables alike.  Integer and Boolean
    inputs keep their dtype.

    Parameters
    ----------
    parents :
        ${parents}
    swap_prob :
        ${swap_prob}
    pair_prob :
        ${pair_prob}
    seed :
        ${random_seed}

    Returns
    -------
        ${pair_offspring}

    See Also
    --------
    single_point_crossover : Exchange the tails of two parents.

    Notes
    -----
    Uniform crossover was proposed by :footcite:t:`Syswerda89`.  See also
    :footcite:t:`EibSmi2003`.

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> parents = np.array([[0, 0, 0, 0, 0, 0], [1, 1, 1, 1, 1, 1]])
    >>> moocore.uniform_crossover(parents, seed=5)
    array([[0, 1, 0, 1, 0, 0],
           [1, 0, 1, 0, 1, 1]])

    """
    dtype = np.asarray(parents).dtype
    parents = _as_matrix(parents, "parents", min_rows=2)
    swap_prob = _check_probability(swap_prob, "swap_prob")
    pair_prob = _check_probability(pair_prob, "pair_prob")
    out = _call(
        lib.uniform_crossover,
        parents,
        len(parents),
        swap_prob,
        pair_prob,
        _get_seed_for_c(seed),
    )
    return _keep_dtype(out, dtype)


@DocSubstitute()
def single_point_crossover(
    parents: ArrayLike,
    /,
    *,
    pair_prob: float = 1.0,
    seed: int | np.random.Generator | None = None,
) -> np.ndarray:
    r"""Single-point crossover.

    Each crossover operation on a pair of parents produces two offspring.  With
    probability ``pair_prob``, a cut point :math:`k` is drawn uniformly from
    :math:`\{1, \dots, n_\text{var} - 1\}`.  The first offspring takes the
    first :math:`k` variables from the first parent and the rest from the
    second parent, and the second offspring takes the complement.  Otherwise,
    the offspring are copies of their parents.

    The offspring only take values from their parents, so this operator works
    for binary, integer and real-valued variables alike.  Integer and Boolean
    inputs keep their dtype.

    Parameters
    ----------
    parents :
        ${parents}  It must have at least 2 columns.
    pair_prob :
        ${pair_prob}
    seed :
        ${random_seed}

    Returns
    -------
        ${pair_offspring}

    See Also
    --------
    uniform_crossover : Swap each variable independently.

    Notes
    -----
    Single-point crossover is the classical crossover of genetic algorithms
    :footcite:p:`Hol75,Goldberg89`.

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> parents = np.array([[0, 0, 0, 0, 0, 0], [1, 1, 1, 1, 1, 1]])
    >>> moocore.single_point_crossover(parents, seed=4)
    array([[0, 0, 0, 1, 1, 1],
           [1, 1, 1, 0, 0, 0]])

    """
    dtype = np.asarray(parents).dtype
    parents = _as_matrix(parents, "parents", min_rows=2)
    if parents.shape[1] < 2:
        raise ValueError("'parents' must have at least 2 columns")
    pair_prob = _check_probability(pair_prob, "pair_prob")
    out = _call(
        lib.single_point_crossover,
        parents,
        len(parents),
        pair_prob,
        _get_seed_for_c(seed),
    )
    return _keep_dtype(out, dtype)


@DocSubstitute()
def blend_alpha_crossover(
    parents: ArrayLike,
    /,
    *,
    lower: ArrayLike | None = None,
    upper: ArrayLike | None = None,
    alpha: float = 0.5,
    pair_prob: float = 1.0,
    seed: int | np.random.Generator | None = None,
) -> np.ndarray:
    r"""Blend crossover (BLX-:math:`\alpha`).

    Each crossover operation on a pair of parents :math:`x, y` produces
    **one** offspring :math:`z`.  With probability ``pair_prob``, for each
    variable :math:`j`, let :math:`m_j = \min(x_j, y_j)`, :math:`M_j =
    \max(x_j, y_j)` and :math:`d_j = M_j - m_j`.  Then :math:`z_j` is sampled
    uniformly from :math:`[m_j - \alpha d_j, M_j + \alpha d_j]`, clipped to
    the bounds of the variable.  Otherwise, the offspring is a copy of the
    first parent :math:`x`.

    Parameters
    ----------
    parents :
        ${parents}  Every value must be within the bounds.
    lower :
        ${lower_bound}
    upper :
        ${upper_bound}
    alpha :
        Fraction of the distance between the parents by which the sampling
        interval extends beyond them on each side.  With ``alpha=0``, the
        offspring lie between their parents.
    pair_prob :
        ${pair_prob}
    seed :
        ${random_seed}

    Returns
    -------
        Array with :math:`\lceil n/2 \rceil` rows, where :math:`n` is the
        number of rows of ``parents``.  Row ``i`` is the offspring of rows
        ``2i`` and ``2i + 1`` of ``parents``.

    See Also
    --------
    local_crossover : Offspring inside the box spanned by the parents.

    Notes
    -----
    BLX-:math:`\alpha` was proposed by :footcite:t:`EshSch1992`, who recommend
    :math:`\alpha = 0.5` because it makes an offspring as likely to lie outside
    its parents as between them.  Each variable is sampled independently, so
    the offspring fill the whole (clipped) box around the parents.  Eshelman
    and Schaffer do not handle bounds, and they produce two offspring per pair.

    To produce more than one offspring per pair, call the function again.  If
    ``seed`` is an integer, every call returns the same offspring.  Pass the
    same :class:`numpy.random.Generator` to every call to get different
    offspring each time, as in the example below.

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> parents = np.array([[0.2, 0.2], [0.6, 0.8]])
    >>> rng = np.random.default_rng(42)
    >>> moocore.blend_alpha_crossover(parents, lower=0, upper=1, seed=rng)
    array([[0.4174605 , 0.81353203]])
    >>> moocore.blend_alpha_crossover(parents, lower=0, upper=1, seed=rng)
    array([[0.03527987, 0.73987298]])

    """
    parents = _as_matrix(parents, "parents", min_rows=2)
    lower, upper = _bounds(lower, upper, parents.shape[1])
    _check_within_bounds(parents, lower, upper)
    if not alpha >= 0:
        raise ValueError("'alpha' must be non-negative")
    pair_prob = _check_probability(pair_prob, "pair_prob")
    return _call(
        lib.blend_alpha_crossover,
        parents,
        (len(parents) + 1) // 2,
        ffi.from_buffer("double []", lower),
        ffi.from_buffer("double []", upper),
        float(alpha),
        pair_prob,
        _get_seed_for_c(seed),
    )


@DocSubstitute()
def single_arithmetic_crossover(
    parents: ArrayLike,
    /,
    *,
    pair_prob: float = 1.0,
    seed: int | np.random.Generator | None = None,
) -> np.ndarray:
    r"""Single arithmetic crossover.

    Each crossover operation on a pair of parents :math:`x, y` produces two
    offspring.  With probability ``pair_prob``, one variable :math:`k` is
    chosen uniformly at random and both offspring get :math:`(x_k + y_k) / 2`
    in that variable.  All other variables are copied from the parents.
    Otherwise, the offspring are copies of their parents.

    Parameters
    ----------
    parents :
        ${parents}
    pair_prob :
        ${pair_prob}
    seed :
        ${random_seed}

    Returns
    -------
        ${pair_offspring}

    See Also
    --------
    local_crossover : Recombine every variable with a random weight.

    Notes
    -----
    :footcite:t:`EibSmi2003` describe single arithmetic recombination with a
    weight :math:`\alpha`, where one offspring gets :math:`\alpha y_k + (1 -
    \alpha) x_k` and the other gets :math:`\alpha x_k + (1 - \alpha) y_k`.
    This function is their example with :math:`\alpha = 1/2`, where both
    offspring get the average.  Picek, Jakobovic and Golub (2013) compare it
    with other real-coded crossover operators.

    References
    ----------
    .. footbibliography::

    S. Picek, D. Jakobovic and M. Golub (2013). On the recombination operator
    in the real-coded genetic algorithms. *2013 IEEE Congress on Evolutionary
    Computation*, pages 3103--3110. https://doi.org/10.1109/CEC.2013.6557948

    Examples
    --------
    >>> parents = np.array([[0.0, 0.0, 0.0, 0.0], [1.0, 1.0, 1.0, 1.0]])
    >>> moocore.single_arithmetic_crossover(parents, seed=42)
    array([[0. , 0. , 0. , 0.5],
           [1. , 1. , 1. , 0.5]])

    """
    parents = _as_matrix(parents, "parents", min_rows=2)
    pair_prob = _check_probability(pair_prob, "pair_prob")
    return _call(
        lib.single_arithmetic_crossover,
        parents,
        len(parents),
        pair_prob,
        _get_seed_for_c(seed),
    )


@DocSubstitute()
def local_crossover(
    parents: ArrayLike,
    /,
    *,
    pair_prob: float = 1.0,
    seed: int | np.random.Generator | None = None,
) -> np.ndarray:
    r"""Local crossover.

    Each crossover operation on a pair of parents :math:`x, y` produces two
    offspring.  With probability ``pair_prob``, they are

    .. math::
        u_j = a_j x_j + (1 - a_j) y_j, \qquad v_j = (1 - a_j) x_j + a_j y_j,

    where each weight :math:`a_j` is drawn uniformly from :math:`[0, 1)`.  The
    offspring lie in the box spanned by their parents, and :math:`u + v = x +
    y`.  Otherwise, the offspring are copies of their parents.

    Parameters
    ----------
    parents :
        ${parents}
    pair_prob :
        ${pair_prob}
    seed :
        ${random_seed}

    Returns
    -------
        ${pair_offspring}

    See Also
    --------
    single_arithmetic_crossover : Average a single variable.
    blend_alpha_crossover : Sample around the parents.

    Notes
    -----
    This operator is described by Dumitrescu et al. (2000) and compared with
    other real-coded crossover operators by Picek, Jakobovic and Golub (2013).

    References
    ----------
    D. Dumitrescu, B. Lazzerini, L. C. Jain and A. Dumitrescu (2000).
    *Evolutionary Computation*. CRC Press. https://doi.org/10.1201/9781482273960

    S. Picek, D. Jakobovic and M. Golub (2013). On the recombination operator
    in the real-coded genetic algorithms. *2013 IEEE Congress on Evolutionary
    Computation*, pages 3103--3110. https://doi.org/10.1109/CEC.2013.6557948

    Examples
    --------
    >>> parents = np.array([[0.0, 0.0, 0.0], [1.0, 1.0, 1.0]])
    >>> moocore.local_crossover(parents, seed=42)
    array([[0.04928569, 0.26800606, 0.40134152],
           [0.95071431, 0.73199394, 0.59865848]])

    """
    parents = _as_matrix(parents, "parents", min_rows=2)
    pair_prob = _check_probability(pair_prob, "pair_prob")
    return _call(
        lib.local_crossover,
        parents,
        len(parents),
        pair_prob,
        _get_seed_for_c(seed),
    )


@DocSubstitute()
def bounded_exponential_crossover(
    parents: ArrayLike,
    /,
    *,
    lower: ArrayLike | None = None,
    upper: ArrayLike | None = None,
    scale: float = 0.1,
    swap_prob: float = 0.0,
    pair_prob: float = 1.0,
    seed: int | np.random.Generator | None = None,
) -> np.ndarray:
    r"""Bounded exponential crossover (BEX).

    Each crossover operation on a pair of parents :math:`x, y` produces two
    offspring :math:`u, v`.  With probability ``pair_prob``, for each
    variable :math:`j` with :math:`d_j = |x_j - y_j| > 0`,

    .. math::
        u_j = x_j + \beta_j^x d_j, \qquad v_j = y_j + \beta_j^y d_j,

    where :math:`\beta_j^x` is negative or positive with probability 1/2 each.
    On each side, it follows an exponential distribution with scale
    :math:`\lambda` (``scale``), truncated at the bound on that side, so that
    :math:`u_j` stays within the bounds of the variable.  Towards the lower
    bound :math:`l_j`, :math:`\beta = \lambda \ln(e_l + r(1 - e_l))` with
    :math:`e_l = \exp((l_j - x_j) / (\lambda d_j))`.  Towards the upper bound
    :math:`h_j`, :math:`\beta = -\lambda \ln(1 - r(1 - e_h))` with :math:`e_h =
    \exp((x_j - h_j) / (\lambda d_j))`.  The uniform random number :math:`r`
    and the direction are shared by both offspring.  Variables where the
    parents are equal are copied.  After that, each variable is swapped between
    the two offspring with probability ``swap_prob``, as in
    :func:`uniform_crossover`.

    Otherwise, with probability ``1 - pair_prob``, the offspring are copies of
    their parents.

    Parameters
    ----------
    parents :
        ${parents}  Every value must be within the bounds.
    lower :
        ${lower_bound}
    upper :
        ${upper_bound}
    scale :
        Scale :math:`\lambda > 0` of the exponential distribution.  Smaller
        values keep the offspring closer to their parents.
    swap_prob :
        ${swap_prob}
    pair_prob :
        ${pair_prob}
    seed :
        ${random_seed}

    Returns
    -------
        ${pair_offspring}

    Notes
    -----
    BEX was proposed by Thakur, Meghwani and Jalota (2014) as a bounded
    version of the Laplace crossover of Deep and Thakur (2007).  Without
    bounds, :math:`\beta` follows a Laplace distribution.  A parent that sits
    on a bound is copied with probability 1/2, as in the paper.

    The swap of variables (``swap_prob``) is not part of the original operator.
    Thakur, Meghwani and Jalota tuned :math:`\lambda = 0.65` and a crossover
    probability of 0.5 for their genetic algorithm.  The defaults here keep
    the offspring closer to their parents and cross over every pair.

    References
    ----------
    M. Thakur, S. S. Meghwani and H. Jalota (2014). A modified real coded
    genetic algorithm for constrained optimization. *Applied Mathematics and
    Computation*, 235:292--317. https://doi.org/10.1016/j.amc.2014.02.093

    K. Deep and M. Thakur (2007). A new crossover operator for real coded
    genetic algorithms. *Applied Mathematics and Computation*,
    188(1):895--911. https://doi.org/10.1016/j.amc.2006.10.047

    Examples
    --------
    >>> parents = np.array([[0.2, 0.2], [0.6, 0.8]])
    >>> moocore.bounded_exponential_crossover(parents, lower=0, upper=1, seed=42)
    array([[0.32040486, 0.14833417],
           [0.72036984, 0.74522359]])

    """
    parents = _as_matrix(parents, "parents", min_rows=2)
    lower, upper = _bounds(lower, upper, parents.shape[1])
    _check_within_bounds(parents, lower, upper)
    scale = _check_positive(scale, "scale")
    swap_prob = _check_probability(swap_prob, "swap_prob")
    pair_prob = _check_probability(pair_prob, "pair_prob")
    return _call(
        lib.bounded_exponential_crossover,
        parents,
        len(parents),
        ffi.from_buffer("double []", lower),
        ffi.from_buffer("double []", upper),
        scale,
        swap_prob,
        pair_prob,
        _get_seed_for_c(seed),
    )


@DocSubstitute()
def de_rand_1_bin_crossover(
    population: ArrayLike,
    /,
    *,
    targets: ArrayLike | None = None,
    lower: ArrayLike | None = None,
    upper: ArrayLike | None = None,
    scaling_factor: float = 0.5,
    crossover_rate: float = 0.9,
    seed: int | np.random.Generator | None = None,
) -> np.ndarray:
    r"""Differential evolution crossover (DE/rand/1/bin).

    For each target :math:`x_i`, three rows :math:`x_{r_1}, x_{r_2}, x_{r_3}`
    are drawn from ``population``, distinct from each other and from the
    target.  The offspring takes each variable :math:`j` from the mutant
    :math:`x_{r_1} + F (x_{r_2} - x_{r_3})` with probability :math:`CR`, and
    from the target otherwise.  One variable chosen at random always comes
    from the mutant.  Finally, the offspring is clipped to the bounds.

    Parameters
    ----------
    population :
        ${population}  It must have at least 4 rows.
    targets :
        ${targets}
    lower :
        ${lower_bound}
    upper :
        ${upper_bound}
    scaling_factor :
        ${scaling_factor}
    crossover_rate :
        ${crossover_rate}
    seed :
        ${random_seed}

    Returns
    -------
        Array with one row per target, where row ``i`` is the offspring of
        target ``targets[i]``.

    See Also
    --------
    de_current_1_crossover : The variant used by MOEA/D-DE.

    Notes
    -----
    This is the DE/rand/1/bin scheme of :footcite:t:`StoPri1997:de`.  They
    recommend :math:`F = 0.5` as a first choice.  They call :math:`CR = 0.1` a
    good first choice, but suggest first trying :math:`CR = 0.9` or 1.0 to see
    whether a quick solution is possible, which gives the default here.
    Multi-objective DE algorithms such as GDE3 :footcite:p:`KukLam2005:gde3`
    and DEMO :footcite:p:`RobFil05demo` use this variant
    :footcite:p:`BezLopStu2015emode`.

    Storn and Price do not specify how to handle values outside the bounds.
    Clipping them, as done here, is a common choice.

    References
    ----------
    .. footbibliography::

    Examples
    --------
    >>> population = np.array(
    ...     [[0.1, 0.1], [0.4, 0.9], [0.8, 0.3], [0.5, 0.5], [0.9, 0.9]]
    ... )
    >>> moocore.de_rand_1_bin_crossover(
    ...     population, targets=[0, 3], lower=0, upper=1, seed=42
    ... )
    array([[1. , 0.5],
           [0. , 0.1]])

    """
    population = _as_matrix(population, "population", min_rows=4)
    targets = _targets(targets, len(population))
    lower, upper = _bounds(lower, upper, population.shape[1])
    scaling_factor = _check_positive(scaling_factor, "scaling_factor")
    crossover_rate = _check_probability(crossover_rate, "crossover_rate")
    return _call_with_targets(
        lib.de_rand_1_bin_crossover,
        population,
        targets,
        lower,
        upper,
        scaling_factor,
        crossover_rate,
        _get_seed_for_c(seed),
    )


@DocSubstitute()
def de_current_1_crossover(
    population: ArrayLike,
    /,
    *,
    targets: ArrayLike | None = None,
    lower: ArrayLike | None = None,
    upper: ArrayLike | None = None,
    scaling_factor: float = 0.5,
    crossover_rate: float = 1.0,
    seed: int | np.random.Generator | None = None,
) -> np.ndarray:
    r"""Differential evolution crossover used by MOEA/D-DE (DE/current/1).

    For each target :math:`x_i`, two distinct rows :math:`x_{r_1}, x_{r_2}`
    are drawn from ``population``.  Either of them may be the target itself.
    The offspring takes each variable :math:`j` from the mutant :math:`x_i + F
    (x_{r_1} - x_{r_2})` with probability :math:`CR`, and from the target
    otherwise.  Finally, the offspring is clipped to the bounds.

    Unlike :func:`de_rand_1_bin_crossover`, the base vector is the target and
    no variable is forced to come from the mutant, so with :math:`CR = 0` the
    offspring is a copy of the target.

    Parameters
    ----------
    population :
        ${population}  In MOEA/D-DE, this is the neighbourhood of the
        subproblem or, with a small probability, the whole population, and
        ``targets`` gives the row of the subproblem's current solution.
    targets :
        ${targets}
    lower :
        ${lower_bound}
    upper :
        ${upper_bound}
    scaling_factor :
        ${scaling_factor}
    crossover_rate :
        ${crossover_rate}
    seed :
        ${random_seed}

    Returns
    -------
        Array with one row per target, where row ``i`` is the offspring of
        target ``targets[i]``.

    See Also
    --------
    de_rand_1_bin_crossover : The classical DE/rand/1/bin scheme.

    Notes
    -----
    This is the DE operator of MOEA/D-DE :footcite:p:`LiZha2009:moead-de`,
    which uses :math:`CR = 1` and :math:`F = 0.5`.  With :math:`CR = 1`, the
    operator is invariant to rotations of the decision space.  MOEA/D-DE then
    applies polynomial mutation, which is not part of this function.

    Li and Zhang do not name the mutation step.  Tanabe and Ishibuchi (2019)
    call it the current/1 strategy, a term introduced by Gong et al. (2017),
    and find it in the source code of MOEA/D-DE, MOEA/D-DRA and MOEA/D-STM.

    Li and Zhang do not require the two donors to differ.  This function draws
    two distinct donors, either of which may be the target, as their C++ code
    does according to Tanabe and Ishibuchi (2019).

    Li and Zhang reset values outside the bounds to a random value within
    them.  Their own implementation clips them instead, which is what this
    function does, and which Tanabe and Ishibuchi (2019) found to work best
    with this operator.  Tanabe and Ishibuchi also explain why some
    implementations of MOEA/D-DE use DE/rand/1 instead.

    References
    ----------
    .. footbibliography::

    R. Tanabe and H. Ishibuchi (2019). Review and analysis of three components
    of the differential evolution mutation operator in MOEA/D-DE. *Soft
    Computing*, 23(23):12843--12857.
    https://doi.org/10.1007/s00500-019-03842-6

    W. Gong, Y. Wang, Z. Cai and S. Yang (2017). A weighted biobjective
    transformation technique for locating multiple optimal solutions of
    nonlinear equation systems. *IEEE Transactions on Evolutionary
    Computation*, 21(5):697--713. https://doi.org/10.1109/TEVC.2017.2670779

    Examples
    --------
    >>> neighbourhood = np.array([[0.5, 0.5], [0.4, 0.9], [0.8, 0.3]])
    >>> moocore.de_current_1_crossover(
    ...     neighbourhood, targets=[0], lower=0, upper=1, seed=42
    ... )
    array([[0.3, 0.8]])

    """
    population = _as_matrix(population, "population", min_rows=2)
    targets = _targets(targets, len(population))
    lower, upper = _bounds(lower, upper, population.shape[1])
    scaling_factor = _check_positive(scaling_factor, "scaling_factor")
    crossover_rate = _check_probability(crossover_rate, "crossover_rate")
    return _call_with_targets(
        lib.de_current_1_crossover,
        population,
        targets,
        lower,
        upper,
        scaling_factor,
        crossover_rate,
        _get_seed_for_c(seed),
    )


@DocSubstitute()
def parent_centric_crossover(
    population: ArrayLike,
    /,
    *,
    targets: ArrayLike | None = None,
    lower: ArrayLike | None = None,
    upper: ArrayLike | None = None,
    sigma_zeta: float = 0.1,
    sigma_eta: float = 0.1,
    seed: int | np.random.Generator | None = None,
) -> np.ndarray:
    r"""Parent-centric crossover (PCX).

    Each target :math:`x` is an index parent.  Two more parents :math:`a, b`
    are drawn from ``population``, distinct from each other and from the
    target.  Let :math:`d = x - g`, where :math:`g` is the centroid of the
    three parents, and let :math:`\bar{D}` be the mean distance from :math:`a`
    and :math:`b` to the line through :math:`x` along :math:`d`.  The
    offspring is

    .. math::
        x + \zeta d + \bar{D} \eta,

    where :math:`\zeta \sim N(0, \sigma_\zeta^2)` and :math:`\eta` is a
    vector of independent :math:`N(0, \sigma_\eta^2)` values projected onto
    the subspace orthogonal to :math:`d`.  Finally, the offspring is clipped to
    the bounds.

    Parameters
    ----------
    population :
        ${population}  It must have at least 3 rows.
    targets :
        ${targets}
    lower :
        ${lower_bound}
    upper :
        ${upper_bound}
    sigma_zeta :
        Standard deviation :math:`\sigma_\zeta > 0` of the step along
        :math:`d`.
    sigma_eta :
        Standard deviation :math:`\sigma_\eta > 0` of the steps orthogonal to
        :math:`d`.
    seed :
        ${random_seed}

    Returns
    -------
        Array with one row per target, where row ``i`` is the offspring of
        index parent ``targets[i]``.

    Notes
    -----
    PCX was proposed by Deb, Anand and Joshi (2002), who use three parents and
    :math:`\sigma_\zeta = \sigma_\eta = 0.1`.  Their equation (2) writes the
    step orthogonal to :math:`d` as a sum of normal steps along :math:`\mu - 1`
    orthonormal vectors, where :math:`\mu` is the number of parents.  Like
    pymoo, which bases its implementation on the authors' C code, this
    function projects an isotropic normal vector onto the whole subspace
    orthogonal to :math:`d`, so the step covers every direction orthogonal to
    :math:`d`, not only :math:`\mu - 1` of them.  If :math:`x` is (almost) the
    centroid, the projection is skipped.

    References
    ----------
    K. Deb, A. Anand and D. Joshi (2002). A computationally efficient
    evolutionary algorithm for real-parameter optimization. *Evolutionary
    Computation*, 10(4):371--395. https://doi.org/10.1162/106365602760972767

    Examples
    --------
    >>> population = np.array([[0.2, 0.2], [0.4, 0.9], [0.8, 0.3], [0.5, 0.5]])
    >>> moocore.parent_centric_crossover(
    ...     population, targets=[0, 3], lower=0, upper=1, seed=42
    ... )
    array([[0.18832072, 0.19642879],
           [0.50136187, 0.44711562]])

    """
    population = _as_matrix(population, "population", min_rows=3)
    targets = _targets(targets, len(population))
    lower, upper = _bounds(lower, upper, population.shape[1])
    sigma_zeta = _check_positive(sigma_zeta, "sigma_zeta")
    sigma_eta = _check_positive(sigma_eta, "sigma_eta")
    return _call_with_targets(
        lib.parent_centric_crossover,
        population,
        targets,
        lower,
        upper,
        sigma_zeta,
        sigma_eta,
        _get_seed_for_c(seed),
    )
