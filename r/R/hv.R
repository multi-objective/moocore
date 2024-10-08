#' Hypervolume metric
#'
#' Compute the hypervolume metric with respect to a given reference point
#' assuming minimization of all objectives. For 2D and 3D, the algorithm used
#' \citep{FonPaqLop06:hypervolume,BeuFonLopPaqVah09:tec} has \eqn{O(n \log n)}
#' complexity. For 4D or higher, the algorithm \citep{FonPaqLop06:hypervolume}
#' has \eqn{O(n^{d-2} \log n)} time and linear space complexity in the
#' worst-case, but experimental results show that the pruning techniques used
#' may reduce the time complexity even further.
#'
#' @inherit epsilon params return
#'
#' @param reference `numeric()`\cr Reference point as a vector of numerical
#'   values.
#'
#' @author Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
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
  .Call(hypervolume_C,
    t(x),
    as.double(reference))
}

#' Hypervolume contribution of a set of points
#'
#' Computes the hypervolume contribution of each point given a set of points
#' with respect to a given reference point assuming minimization of all
#' objectives.  Dominated points have zero contribution. Duplicated points have
#' zero contribution even if not dominated, because removing one of them does
#' not change the hypervolume dominated by the remaining set.
#'
#' @inheritParams hypervolume
#'
#' @return `numeric()`\cr A numerical vector
#'
#' @author Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
#'
#' @seealso [hypervolume()]
#'
#' @references
#'
#' \insertRef{FonPaqLop06:hypervolume}{moocore}
#'
#' \insertRef{BeuFonLopPaqVah09:tec}{moocore}
#'
#' @examples
#'
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
hv_contributions <- function(x, reference, maximise = FALSE)
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
    as.double(reference))
}
