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
#' The unary R2 indicator is a quality indicator for a set \eqn{A \subset
#' \mathbb{R}^m}{A in R^m} w.r.t. an ideal or utopian reference point
#' \eqn{\vec{r} \in \mathbb{R}^m}{r in R^m}.  It was originally proposed by
#' \citet{HanJas1998} and is defined as the expected Tchebycheff utility under a
#' uniform distribution of weight vectors (w.l.o.g. assuming minimization):
#'
#' \deqn{R2(A) := \int_{w \in W} \min_{a \in A} \left\{ \max_{i=1,\dots,m} w_i
#' (a_i - r_i) \right\} \, dw,}{R2(A) := integral over w in W of min over a in
#' A of max over i of w_i (a_i - r_i) dw,}
#'
#' where \eqn{W} denotes the uniform distribution across weights:
#'
#' \deqn{W = \{w \in \mathbb{R}^m \mid w_i \geq 0, \sum_{i=1}^m w_i = 1\}.}{W
#' = \{w in R^m | w_i >= 0, sum_i w_i = 1\}.}
#'
#' The R2 indicator is to be minimized and has an optimal value of 0 when
#' \eqn{\vec{r} \in A}{r in A}.
#'
#' The exact R2 indicator is strongly Pareto-compliant, i.e., compatible with
#' Pareto-optimality:
#'
#' \deqn{\forall A, B \subset \mathbb{R}^m: A \prec B \Rightarrow R2(A) <
#' R2(B).}{for all A, B in R^m: A dominates B implies R2(A) < R2(B).}
#'
#' Given an ideal or utopian reference point, which is available in most
#' scenarios, all non-dominated solutions always contribute to the value of
#' the exact R2 indicator.  However, it is scale-dependent and care should be
#' taken such that all objectives contribute approximately equally to the
#' indicator, e.g., by normalizing the Pareto front to the unit hypercube.
#'
#' The current implementation exclusively supports bi-objective solution sets
#' and runs in \eqn{O(n \log n)} following \citet{SchKer2025r2v2}.
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
#' # This function assumes minimisation by default. We can easily specify maximisation:
#' @expect equal(tolerance = 1e-9, 2.5196969696969695)
#' r2_exact(dat, reference = c(10, 10), maximise = TRUE)
#'
#' # Merge all the sets of a dataset by removing the set number column:
#' extdata_path <- system.file(package="moocore","extdata")
#' dat <- read_datasets(file.path(extdata_path, "example1_dat"))[, 1:2]
#' @expect equal(65L)
#' nrow(dat)
#'
#' # Dominated points are ignored, so this:
#' @expect equal(tolerance = 1e-9, 3865393.493470812)
#' r2_exact(dat, reference = 0)
#'
#' # gives the same exact R2 value as this:
#' dat <- filter_dominated(dat)
#' @expect equal(9L)
#' nrow(dat)
#' @expect equal(tolerance = 1e-9, 3865393.493470812)
#' r2_exact(dat, reference = 0)
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
