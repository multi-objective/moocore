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
#' @return `vorobT` returns a list with elements `threshold`,
#'   `VE`, and `avg_hyp` (average hypervolume)
#' @rdname Vorob
#' @author Mickael Binois
#' @doctest
#' data(CPFs)
#' res <- vorobT(CPFs, reference = c(2, 200))
#' @expect equal(44.140625)
#' res$threshold
#' @expect equal(8943.3332)
#' res$avg_hyp
#' @omit
#' \donttest{
#' # Display Vorob'ev expectation and attainment function
#' if (require("mooplot", quietly=TRUE)) {
#'    # First style
#'    eafplot(CPFs[,1:2], sets = CPFs[,3], percentiles = c(0, 25, 50, 75, 100, res$threshold),
#'            main = substitute(paste("Empirical attainment function, ",beta,"* = ", a, "%"),
#'                              list(a = formatC(res$threshold, digits = 2, format = "f"))))
#'    # Second style
#'    eafplot(CPFs[,1:2], sets = CPFs[,3], percentiles = c(0, 20, 40, 60, 80, 100),
#'            col = gray(seq(0.8, 0.1, length.out = 6)^0.5), type = "area",
#'            legend.pos = "bottomleft", extra.points = res$VE, extra.col = "cyan",
#'            extra.legend = "VE", extra.lty = "solid", extra.pch = NA, extra.lwd = 2,
#'            main = substitute(paste("Empirical attainment function, ",beta,"* = ", a, "%"),
#'                              list(a = formatC(res$threshold, digits = 2, format = "f"))))
#' }}
#' @resume
#' # Now print Vorob'ev deviation
#' VD <- vorobDev(CPFs, VE = res$VE, reference = c(2, 200))
#' @expect equal(3017.1299)
#' VD
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
vorobT <- function(x, sets, reference, maximise = FALSE)
{
  if (missing(sets)) {
    sets <- x[, ncol(x)]
    x <- x[, -ncol(x), drop=FALSE]
  }
  x <- as_double_matrix(x)

  if (any(maximise)) {
    x <- transform_maximise(x, maximise)
    if (length(maximise) == 1L) {
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
    VE <- eaf(x, sets = sets, percentiles = c)[,-setcol]
    tmp <- hypervolume(VE, reference = reference)
    if (tmp > avg_hyp) a <- c else b <- c
    diff <- prev_hyp - tmp
    prev_hyp <- tmp
  }
  if (any(maximise))
    VE <- transform_maximise(VE, maximise)
  list(threshold = c, VE = VE, avg_hyp = avg_hyp)
}

#' @concept eaf
#' @rdname Vorob
#' @param VE `matrix()`\cr Vorob'ev expectation, e.g., as returned by [vorobT()].
#' @return `vorobDev` returns the Vorob'ev deviation.
#' @export
vorobDev <- function(x, sets, reference, VE = NULL, maximise = FALSE)
{
  # FIXME: Does it make sense to call this function with 'x' different than the
  # one used to calculate VE? If not, then we should merge them and avoid a lot
  # of redundant work.
  if (missing(sets)) {
    sets <- x[, ncol(x)]
    x <- x[, -ncol(x), drop=FALSE]
  }
  if (is.null(VE)) {
    VE <- vorobT(x, sets, reference = reference, maximise = maximise)$VE
  } else {
    x <- as_double_matrix(x)
  }

  if (any(maximise)) {
    x <- transform_maximise(x, maximise)
    VE <- transform_maximise(VE, maximise)
    if (length(maximise) == 1L) {
      reference <- -reference
    } else {
      reference[maximise] <- -reference[maximise]
    }
  }
  setcol <- ncol(x)
  # Hypervolume of the symmetric difference between A and B:
  # 2 * H(AUB) - H(A) - H(B)
  H2 <- hypervolume(VE, reference = reference)
  x_split <- split.data.frame(x, sets)
  H1 <- mean(sapply(x_split, hypervolume, reference = reference))

  hv_union_VE <- function(y)
    hypervolume(rbind(y, VE), reference = reference)

  VD <- 2 * sum(sapply(x_split, hv_union_VE))
  nruns <- length(x_split)
  ((VD / nruns) - H1 - H2)
}
