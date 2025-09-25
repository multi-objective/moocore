#' Identify, remove and rank dominated points according to Pareto optimality
#'
#' Identify nondominated points with `is_nondominated()` and remove dominated
#' ones with `filter_dominated()`.
#'
#' @rdname nondominated
#'
#' @param x `matrix()`|`data.frame()`\cr Matrix or data frame of numerical
#'   values, where each row gives the coordinates of a point.
#'
#' @param maximise `logical()`\cr Whether the objectives must be maximised
#'   instead of minimised. Either a single logical value that applies to all
#'   objectives or a vector of logical values, with one value per objective.
#'
#' @param keep_weakly `logical(1)`\cr If `FALSE`, return `FALSE` for any
#'   duplicates of nondominated points.  Which of the duplicates is identified
#'   as nondominated is unspecified due to the sorting not being stable in this
#'   version.
#'
#' @return [is_nondominated()] returns a logical vector of the same length
#'   as the number of rows of `data`, where `TRUE` means that the
#'   point is not dominated by any other point.
#'
#' @author Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
#'
#' @details
#'
#' Given \eqn{n} points of dimension \eqn{m}, the current implementation uses
#' the well-known \eqn{O(n \log n)} dimension-sweep algorithm
#' \citep{KunLucPre1975jacm} for \eqn{m \leq 3} and the naive \eqn{O(m n^2)}
#' algorithm for \eqn{m \geq 4}.  The best-known \eqn{O(n(\log_2 n)^{m-2})}
#' algorithm for \eqn{m \geq 4} \citep{KunLucPre1975jacm} is not implemented
#' yet.
#'
#' @doctest
#' S = matrix(c(1,1,0,1,1,0,1,0), ncol = 2, byrow = TRUE)
#' @expect equal(c(FALSE,TRUE,TRUE,FALSE))
#' is_nondominated(S)
#'
#' @expect equal(c(TRUE,FALSE,FALSE,FALSE))
#' is_nondominated(S, maximise = TRUE)
#'
#' @expect equal(matrix(c(0,1,1,0), ncol = 2, byrow = TRUE))
#' filter_dominated(S)
#'
#' @expect equal(matrix(c(0,1,1,0,1,0), ncol = 2, byrow = TRUE))
#' filter_dominated(S, keep_weakly = TRUE)
#'
#' @expect equal(TRUE)
#' any_dominated(S)
#'
#' @expect equal(TRUE)
#' any_dominated(S, keep_weakly = TRUE)
#'
#' @expect equal(FALSE)
#' any_dominated(filter_dominated(S))
#'
#' @omit
#' path_A1 <- file.path(system.file(package="moocore"),"extdata","ALG_1_dat.xz")
#' set <- read_datasets(path_A1)[,1:2]
#' is_nondom <- is_nondominated(set)
#' cat("There are ", sum(is_nondom), " nondominated points\n")
#'
#' if (requireNamespace("graphics", quietly = TRUE)) {
#'    plot(set, col = "blue", type = "p", pch = 20)
#'    ndset <- filter_dominated(set)
#'    points(ndset[order(ndset[,1]),], col = "red", pch = 21)
#' }
#'
#' ranks <- pareto_rank(set)
#' str(ranks)
#' if (requireNamespace("graphics", quietly = TRUE)) {
#'    colors <- colorRampPalette(c("red","yellow","springgreen","royalblue"))(max(ranks))
#'    plot(set, col = colors[ranks], type = "p", pch = 20)
#' }
#' @export
#' @concept dominance
is_nondominated <- function(x, maximise = FALSE, keep_weakly = FALSE)
{
  x <- as_double_matrix_1(x)
  nobjs <- ncol(x)
  if (nobjs == 1L) { # Handle single-objective
    if (keep_weakly) {
      best <- if (maximise) max(x) else min(x)
      return(as.vector(x == best))
    } else {
      nondom <- logical(nrow(x))
      nondom[if (maximise) which.max(x) else which.min(x)] <- TRUE
      return(nondom)
    }
  }
  .Call(is_nondominated_C,
    t(x),
    rep_len(as.logical(maximise), nobjs),
    as.logical(keep_weakly))
}

#' @rdname nondominated
#' @concept dominance
#' @return `filter_dominated` returns a matrix or data.frame with only mutually nondominated points.
#' @export
filter_dominated <- function(x, maximise = FALSE, keep_weakly = FALSE)
  x[is_nondominated(x, maximise = maximise, keep_weakly = keep_weakly), , drop = FALSE]

#' @rdname nondominated
#' @concept dominance
#' @return `any_dominated` returns `TRUE` if `x` contains any (weakly-)dominated points, `FALSE` otherwise.
#' @export
any_dominated <- function(x, maximise = FALSE, keep_weakly = FALSE)
{
  x <- as_double_matrix_1(x)
  if (keep_weakly)
    x <- x[!duplicated(x), , drop = FALSE]
  nrows <- nrow(x)
  if (nrows == 1L) return(FALSE)
  nobjs <- ncol(x)
  # With a single-objective, if there are more than one row, then something is
  # dominated.
  if (nobjs == 1L) return(TRUE)
  .Call(any_dominated_C,
    t(x),
    rep_len(as.logical(maximise), nobjs))
}

#' @description `pareto_rank()` ranks points according to Pareto-optimality,
#'   which is also called nondominated sorting \citep{Deb02nsga2}.
#'
#' @rdname nondominated
#' @concept dominance
#' @return `pareto_rank()` returns an integer vector of the same length as
#'   the number of rows of `data`, where each value gives the rank of each
#'   point.
#'
#' @details `pareto_rank()` is meant to be used like `rank()`, but it
#'   assigns ranks according to Pareto dominance. Duplicated points are kept on
#'   the same front. When `ncol(data) == 2`, the code uses the \eqn{O(n
#'   \log n)} algorithm by \citet{Jen03}.
#'
#' @references
#'
#' \insertAllCited{}
#'
#' @export
pareto_rank <- function(x, maximise = FALSE)
{
  x <- as_double_matrix(x)
  x <- transform_maximise(x, maximise)
  .Call(pareto_ranking_C, t(x))
}
