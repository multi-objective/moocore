#' Hypervolume metric
#'
#' Compute the hypervolume metric with respect to a given reference point
#' assuming minimization of all objectives.
#'
#'
#' @inherit epsilon params return
#'
#' @param reference `numeric()`\cr Reference point as a vector of numerical
#'   values.
#'
#' @author Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
#'
#' @details
#'
#' The hypervolume of a set of multidimensional points \eqn{A \subset
#' \mathbb{R}^d}{A in R^m} with respect to a reference point \eqn{\vec{r} \in
#' \mathbb{R}^d}{r in R^m} is the volume of the region dominated by the set and
#' bounded by the reference point \citep{ZitThi1998ppsn}.  Points in \eqn{A}
#' that do not strictly dominate \eqn{\vec{r}} do not contribute to the
#' hypervolume value, thus, ideally, the reference point must be strictly
#' dominated by all points in the true Pareto front.
#'
#' More precisely, the hypervolume is the [Lebesgue measure](https://en.wikipedia.org/wiki/Lebesgue_measure) of the union of
#' axis-aligned hyperrectangles
#' ([orthotopes](https://en.wikipedia.org/wiki/Hyperrectangle)), where each
#' hyperrectangle is defined by one point from \eqn{\vec{a} \in A} and the
#' reference point.  The union of axis-aligned hyperrectangles is also called
#' an _orthogonal polytope_.
#'
#' The hypervolume is compatible with Pareto-optimality
#' \citep{KnoCor2002cec,ZitThiLauFon2003:tec}, that is, \eqn{\nexists A,B
#' \subset \mathbb{R}^m}{it does not exist A,B subsets of R^d}, such that
#' \eqn{A} is better than \eqn{B} in terms of Pareto-optimality and
#' \eqn{\text{hyp}(A) \leq \text{hyp}(B)}{hyp(A) <= hyp(B)}. In other words, if
#' a set is better than another in terms of Pareto-optimality, the hypervolume
#' of the former must be strictly larger than the hypervolume of the latter.
#' Conversely, if the hypervolume of a set is larger than the hypervolume of
#' another, then we know for sure than the latter set cannot be better than the
#' former in terms of Pareto-optimality.
#'
#' Like most measures of unions of high-dimensional geometric objects,
#' computing the hypervolume is #P-hard \citep{BriFri2010approx}.
#'
#' For 2D and 3D, the algorithms used
#' \citep{FonPaqLop06:hypervolume,BeuFonLopPaqVah09:tec} have \eqn{O(n \log n)}
#' complexity, where \eqn{n} is the number of input points. The 3D case uses
#' the \eqn{\text{HV3D}^{+}} algorithm \citep{GueFon2017hv4d}, which has the
#' sample complexity as the HV3D algorithm
#' \citep{FonPaqLop06:hypervolume,BeuFonLopPaqVah09:tec}, but it is faster in
#' practice.
#'
#' For 4D, the algorithm used is \eqn{\text{HV4D}^{+}} \citep{GueFon2017hv4d},
#' which has \eqn{O(n^2)} complexity.  Compared to the [original
#' implementation](https://github.com/apguerreiro/HVC/), this implementation
#' correctly handles weakly dominated points and has been further optimized for
#' speed.
#'
#' For 5D or higher, it uses a recursive algorithm
#' \citep{FonPaqLop06:hypervolume} with \eqn{\text{HV4D}^{+}} as the base case,
#' resulting in a \eqn{O(n^{d-2})} time complexity and \eqn{O(n)} space
#' complexity in the worst-case, where \eqn{d} is the dimension of the points.
#' Experimental results show that the pruning techniques used may reduce the
#' time complexity even further.  The original proposal
#' \citep{FonPaqLop06:hypervolume} had the HV3D algorithm as the base case,
#' giving a time complexity of \eqn{O(n^{d-2} \log n)}.  Andreia P. Guerreiro
#' enhanced the numerical stability of the algorithm by avoiding floating-point
#' comparisons of partial hypervolumes.
#'
#'
#' @references
#'
#' \insertAllCited{}
#'
#' @examples
#'
#' data(SPEA2minstoptimeRichmond)
#' # The second objective must be maximized
#' # We calculate the hypervolume of the union of all sets.
#' hypervolume(SPEA2minstoptimeRichmond[, 1:2], reference = c(250, 0),
#'             maximise = c(FALSE, TRUE))
#'
#' @export
#' @concept metrics
hypervolume <- function(x, reference, maximise = FALSE)
{
  x <- as_double_matrix(x)
  nobjs <- ncol(x)
  if (!is.numeric(reference))
    stop("a numerical reference vector must be provided")
  if (length(reference) == 1L) reference <- rep_len(reference, nobjs)

  if (any(maximise)) {
    x <- transform_maximise(x, maximise)
    if (all(maximise)) {
      reference <- -reference
    } else {
      reference[maximise] <- -reference[maximise]
    }
  }
  .Call(hypervolume_C,
    t(x),
    as.double(reference))
}

#' Hypervolume contribution of a set of points
#'
#' Computes the hypervolume contribution of each point of a set of points with
#' respect to a given reference point. Duplicated and dominated points have
#' zero contribution.  By default, dominated points are ignored, that is, they
#' do not affect the contribution of other points.  See the Notes below for
#' more details.  For details about the hypervolume, see [hypervolume()].
#'
#' @inheritParams hypervolume
#'
#' @param ignore_dominated `logical(1)`\cr Whether dominated points are ignored
#'   when computing the contribution of nondominated points.  The value of this
#'   parameter has an effect on the return values only if the input contains
#'   dominated points. Setting this to `FALSE` slows down the computation
#'   significantly.  See the Notes below for a detailed explanation.
#'
#' @return `numeric()`\cr A numerical vector
#'
#' @author Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
#'
#' @details
#'
#' The hypervolume contribution of point \eqn{\vec{p} \in X} is defined as
#' \eqn{\text{hvc}(\vec{p}) = \text{hyp}(X) - \text{hyp}(X \setminus
#' \{\vec{p}\})}.  This definition implies that duplicated points have zero
#' contribution even if not dominated, because removing one of the duplicates
#' does not change the hypervolume of the remaining set.  Moreover, dominated
#' points also have zero contribution. However, a point that is dominated by a
#' single (dominating) nondominated point reduces the contribution of the
#' latter, because removing the dominating point makes the dominated one become
#' nondominated.
#'
#' Handling this special case is non-trivial and makes the computation more
#' expensive, thus the default (`ignore_dominated=TRUE`) ignores all dominated
#' points in the input, that is, their contribution is set to zero and their
#' presence does not affect the contribution of any other point.  Setting
#' `ignore_dominated=FALSE` will consider dominated points according to the
#' mathematical definition given above, but the computation will be slower.
#'
#' When the input only consists of mutually nondominated points, the value of
#' `ignore_dominated` does not change the result, but the default value is
#' significantly faster.
#'
#' The current implementation uses a \eqn{O(n\log n)} dimension-sweep
#' algorithm for 2D.  With `ignore_dominated=TRUE`, the 3D case uses the HVC3D
#' algorithm \citep{GueFon2017hv4d}, which has \eqn{O(n\log n)} complexity.
#' Otherwise, the implementation uses the naive algorithm that requires
#' calculating the hypervolume \eqn{|X|+1} times.
#'
#' @seealso [hypervolume()]
#'
#' @references
#'
#' \insertAllCited{}
#'
#' @doctest
#'
#' x <- matrix(c(5,1, 1,5, 4,2, 4,4, 5,1), ncol=2, byrow=TRUE)
#' @expect equal(c(0,3,3,0,0))
#' hv_contributions(x, reference=c(6,6))
#' # hvc[(5,1)] = 0 = duplicated
#' # hvc[(1,5)] = 3 = (4 - 1) * (6 - 5)
#' # hvc[(4,2)] = 3 = (5 - 4) * (5 - 2)
#' # hvc[(4,4)] = 0 = dominated
#' # hvc[(5,1)] = 0 = duplicated
#'
#' @expect equal(c(0,3,2,0,0))
#' hv_contributions(x, reference=c(6,6), ignore_dominated = FALSE)
#' # hvc[(5,1)] = 0 = duplicated
#' # hvc[(1,5)] = 3 = (4 - 1) * (6 - 5)
#' # hvc[(4,2)] = 2 = (5 - 4) * (4 - 2)
#' # hvc[(4,4)] = 0 = dominated
#' # hvc[(5,1)] = 0 = duplicated
#'
#' @omit
#' data(SPEA2minstoptimeRichmond)
#' # The second objective must be maximized
#' # We calculate the hypervolume contribution of each point of the union of all sets.
#' hv_contributions(SPEA2minstoptimeRichmond[, 1:2], reference = c(250, 0),
#'             maximise = c(FALSE, TRUE))
#'
#' # Duplicated points show zero contribution above, even if not
#' # dominated. However, filter_dominated removes all duplicates except
#' # one. Hence, there are more points below with nonzero contribution.
#' hv_contributions(filter_dominated(SPEA2minstoptimeRichmond[, 1:2], maximise = c(FALSE, TRUE)),
#'                  reference = c(250, 0), maximise = c(FALSE, TRUE))
#'
#' @export
#' @concept metrics
hv_contributions <- function(x, reference, maximise = FALSE, ignore_dominated = TRUE)
{
  x <- as_double_matrix(x)
  nobjs <- ncol(x)
  if (is.null(reference)) stop("reference cannot be NULL")
  if (length(reference) == 1L) reference <- rep_len(reference, nobjs)

  if (any(maximise)) {
    x <- transform_maximise(x, maximise)
    if (all(maximise)) {
      reference <- -reference
    } else {
      reference[maximise] <- -reference[maximise]
    }
  }
  .Call(hv_contributions_C,
    t(x),
    as.double(reference),
    as.logical(ignore_dominated))
}
