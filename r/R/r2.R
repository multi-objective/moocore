#' Exact R2 indicator
#'
#' Computes the exact R2 indicator with respect to a given ideal/utopian
#' reference point assuming minimization of all objectives.
#'
#' @inherit epsilon params
#'
#' @param reference `numeric()`\cr Reference point as a vector of numerical
#'   values.
#'
#' @return `numeric(1)` A single numerical value.
#'
#' @author Lennart \enc{Schäpermeier}{Schapermeier}
#'
#' @details
#'
#' The current implementation exclusively supports bi-objective solution sets
#' and runs in \eqn{O(n \log n)}.  For more details on the computation of the
#' exact R2 indicator refer to \citet{SchKer2025r2v2}.
#'
#' @references
#'
#' \insertAllCited{}
#'
#' @doctest
#' dat <- matrix(c(5, 5, 4, 6, 2, 7, 7, 4), ncol = 2, byrow = TRUE)
#' @expect equal(tolerance = 1e-9, 2.5941919191919194)
#' r2_exact(dat, reference = c(0, 0))
#'
#' @expect equal(tolerance = 1e-9, 2.5196969696969695)
#' r2_exact(dat, reference = c(10, 10), maximise = TRUE)
#'
#' @omit
#' extdata_path <- system.file(package="moocore","extdata")
#' dat <- read_datasets(file.path(extdata_path, "ALG_1_dat.xz"))[, 1:2]
#' r2_exact(dat, reference = c(0, 0))
#'
#' dat <- filter_dominated(dat)
#' r2_exact(dat, reference = c(0, 0))
#'
#' @export
#' @concept metrics
r2_exact <- function(x, reference, maximise = FALSE)
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
  .Call(r2_exact_C,
    t(x),
    as.double(reference))
}
