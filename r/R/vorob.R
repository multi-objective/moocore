#' Vorob'ev computations
#'
#' Compute Vorob'ev threshold, expectation and deviation. Also, displaying the
#' symmetric deviation function is possible.  The symmetric deviation
#' function is the probability for a given target in the objective space to
#' belong to the symmetric difference between the Vorob'ev expectation and a
#' realization of the (random) attained set.
#'
#' @inheritParams eaf
#' @inheritParams hypervolume
#'
#' @return `vorob_t` returns a list with elements `threshold`,
#'   `ve`, and `avg_hyp` (average hypervolume)
#' @rdname Vorob
#' @author Mickael Binois
#' @doctest
#' data(CPFs)
#' res <- vorob_t(CPFs, reference = c(2, 200))
#' @expect equal(44.140625)
#' res$threshold
#' @expect equal(8943.3332)
#' res$avg_hyp
#' # Now print Vorob'ev deviation
#' vd <- vorob_dev(CPFs, ve = res$ve, reference = c(2, 200))
#' @expect equal(3017.1299)
#' vd
#' @references
#' \insertRef{BinGinRou2015gaupar}{moocore}
#'
#' C. Chevalier (2013), Fast uncertainty reduction strategies relying on
#' Gaussian process models, University of Bern, PhD thesis.
#'
#' \insertRef{Molchanov2005theory}{moocore}
#'
#' @concept eaf
#' @export
vorob_t <- function(x, sets, reference, maximise = FALSE)
{
  if (missing(sets)) {
    sets <- x[, ncol(x)]
    x <- x[, -ncol(x), drop=FALSE]
  }
  x <- as_double_matrix(x)

  if (any(maximise)) {
    x <- transform_maximise(x, maximise)
    if (all(maximise)) {
      reference <- -reference
    } else {
      reference[maximise] <- -reference[maximise]
    }
  }

  # First step: compute average hypervolume over conditional Pareto fronts
  avg_hyp <- mean(sapply(split.data.frame(x, sets), hypervolume, reference = reference))

  prev_hyp <- diff <- Inf # hypervolume of quantile at previous step
  a <- 0
  b <- 100
  setcol <- ncol(x) + 1L
  while (diff != 0) {
    c <- (a + b) / 2
    ve <- eaf(x, sets = sets, percentiles = c)[,-setcol]
    tmp <- hypervolume(ve, reference = reference)
    if (tmp > avg_hyp) a <- c else b <- c
    diff <- prev_hyp - tmp
    prev_hyp <- tmp
  }
  ve <- transform_maximise(ve, maximise)
  list(threshold = c, ve = ve, avg_hyp = avg_hyp)
}

#' @concept eaf
#' @rdname Vorob
#' @param ve `matrix()`\cr Vorob'ev expectation, e.g., as returned by [vorob_t()].
#' @return `vorob_dev` returns the Vorob'ev deviation.
#' @export
vorob_dev <- function(x, sets, reference, ve = NULL, maximise = FALSE)
{
  # FIXME: Does it make sense to call this function with 'x' different than the
  # one used to calculate ve? If not, then we should merge them and avoid a lot
  # of redundant work.
  if (missing(sets)) {
    sets <- x[, ncol(x)]
    x <- x[, -ncol(x), drop=FALSE]
  }
  if (is.null(ve)) {
    ve <- vorob_t(x, sets, reference = reference, maximise = maximise)$ve
  } else {
    x <- as_double_matrix(x)
  }

  if (any(maximise)) {
    x <- transform_maximise(x, maximise)
    ve <- transform_maximise(ve, maximise)
    if (all(maximise)) {
      reference <- -reference
    } else {
      reference[maximise] <- -reference[maximise]
    }
  }
  setcol <- ncol(x)
  # Hypervolume of the symmetric difference between A and B:
  # 2 * H(AUB) - H(A) - H(B)
  h2 <- hypervolume(ve, reference = reference)
  x_split <- split.data.frame(x, sets)
  h1 <- mean(sapply(x_split, hypervolume, reference = reference))

  hv_union_ve <- function(y)
    hypervolume(rbind(y, ve), reference = reference)

  vd <- 2 * sum(sapply(x_split, hv_union_ve))
  nruns <- length(x_split)
  ((vd / nruns) - h1 - h2)
}
