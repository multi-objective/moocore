#' Vorob'ev threshold, expectation and deviation
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
#'
#' @details
#'
#' Let \eqn{\mathcal{A} = \{A_1, \dots, A_n\}} be a multi-set of \eqn{n} sets
#' \eqn{A_i \subset \mathbb{R}^d} of mutually nondominated vectors, with finite
#' (but not necessarily equal) cardinality.  If bounded by a reference point
#' \eqn{\vec{r}} that is strictly dominated by any point in any set, then these
#' sets can be seen a samples from a random closed set
#' \citep{Molchanov2005theory}.
#'
#' Let the \eqn{\beta}-quantile be the subset of the empirical attainment
#' function \eqn{\mathcal{Q}_\beta = \{\vec{z}\in \mathbb{R}^d :
#' \hat{\alpha}_{\mathcal{A}}(\vec{z}) \geq \beta\}}.
#'
#' The Vorob'ev *expectation* is the \eqn{\beta^{*}}-quantile set
#' \eqn{\mathcal{Q}_{\beta^{*}}} such that the mean value hypervolume of the
#' sets is equal (or as close as possible) to the hypervolume of
#' \eqn{\mathcal{Q}_{\beta^{*}}}, that is, \eqn{\text{hyp}(\mathcal{Q}_\beta)
#' \leq \mathbb{E}[\text{hyp}(\mathcal{A})] \leq
#' \text{hyp}(\mathcal{Q}_{\beta^{*}})}, \eqn{\forall \beta > \beta^{*}}. Thus,
#' the Vorob'ev expectation provides a definition of the notion of *mean*
#' nondominated set.
#'
#' The value \eqn{\beta^{*} \in [0,1]} is called the Vorob'ev
#' *threshold*. Large differences from the median quantile (0.5) indicate a
#' skewed distribution of \eqn{\mathcal{A}}.
#'
#' The Vorob'ev *deviation* is the mean hypervolume of the symmetric difference
#' between the Vorob'ev expectation and any set in \eqn{\mathcal{A}}, that is,
#' \eqn{\mathbb{E}[\text{hyp}(\mathcal{Q}_{\beta^{*}} \ominus \mathcal{A})]},
#' where the symmetric difference is defined as \eqn{A \ominus B = (A \setminus
#' B) \cup (B \setminus A)}.  Low deviation values indicate that the sets are
#' very similar, in terms of the location of the weakly dominated space, to the
#' Vorob'ev expectation.
#'
#' For more background, see
#' \citet{BinGinRou2015gaupar,Molchanov2005theory,CheGinBecMol2013moda}.
#'
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
#' \insertAllCited{}
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
