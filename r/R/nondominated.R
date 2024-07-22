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
#' @param keep_weakly If `FALSE`, return `FALSE` for any duplicates
#'   of nondominated points.
#'
#' @return [is_nondominated()] returns a logical vector of the same length
#'   as the number of rows of `data`, where `TRUE` means that the
#'   point is not dominated by any other point.
#'
#' @author Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
#'
#' @examples
#' S = matrix(c(1,1,0,1,1,0,1,0), ncol = 2, byrow = TRUE)
#' is_nondominated(S)
#'
#' is_nondominated(S, maximise = TRUE)
#'
#' filter_dominated(S)
#'
#' filter_dominated(S, keep_weakly = TRUE)
#'
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
#' @export
#' @concept dominance
is_nondominated <- function(x, maximise = FALSE, keep_weakly = FALSE)
{
  x <- as_double_matrix(x)
  nobjs <- ncol(x)
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
#' @examples
#' ranks <- pareto_rank(set)
#' if (requireNamespace("graphics", quietly = TRUE)) {
#'    colors <- colorRampPalette(c("red","yellow","springgreen","royalblue"))(max(ranks))
#'    plot(set, col = colors[ranks], type = "p", pch = 20)
#' }
#' @export
pareto_rank <- function(x, maximise = FALSE)
{
  x <- as_double_matrix(x)
  if (any(maximise))
    x <- transform_maximise(x, maximise)
  .Call(pareto_ranking_C, t(x))
}
