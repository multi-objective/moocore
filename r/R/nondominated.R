#' Identify, remove and rank dominated points according to Pareto optimality
#'
#' Identify nondominated points with [is_nondominated()] and remove dominated
#' ones with [filter_dominated()].
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
#'   duplicates of nondominated points, except the last one.
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
#' three_fronts = matrix(c(1, 2, 3,
#'                         3, 1, 2,
#'                         2, 3, 1,
#'                         10, 20, 30,
#'                         30, 10, 20,
#'                         20, 30, 10,
#'                         100, 200, 300,
#'                         300, 100, 200,
#'                         200, 300, 100), ncol=3, byrow=TRUE)
#' @expect equal(c(1,1,1,2,2,2,3,3,3))
#' pareto_rank(three_fronts)
#'
#' split.data.frame(three_fronts, pareto_rank(three_fronts))
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
  # FIXME: Implement this in is_nondominated_C
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
    x,
    rep_len(as.logical(maximise), nobjs),
    as.logical(keep_weakly))
}

#' @rdname nondominated
#' @concept dominance
#' @return [filter_dominated()] returns a matrix or data.frame with only mutually nondominated points.
#' @export
filter_dominated <- function(x, maximise = FALSE, keep_weakly = FALSE)
  x[is_nondominated(x, maximise = maximise, keep_weakly = keep_weakly), , drop = FALSE]

#' @rdname nondominated
#' @concept dominance
#' @description  [any_dominated()] quickly detects if a set contains any dominated point.
#' @return [any_dominated()] returns `TRUE` if `x` contains any (weakly-)dominated points, `FALSE` otherwise.
#' @export
any_dominated <- function(x, maximise = FALSE, keep_weakly = FALSE)
{
  x <- as_double_matrix_1(x)
  if (keep_weakly)
    x <- x[!duplicated(x), , drop = FALSE]
  nobjs <- ncol(x)
  .Call(any_dominated_C,
    x,
    rep_len(as.logical(maximise), nobjs))
}

#' @description [pareto_rank()] ranks points according to Pareto-optimality,
#'   which is also called nondominated sorting \citep{Deb02nsga2}.
#'
#' @rdname nondominated
#' @concept dominance
#' @return [pareto_rank()] returns an integer vector of the same length as
#'   the number of rows of `data`, where each value gives the rank of each
#'   point.
#'
#' @details [pareto_rank()] is meant to be used like `rank()`, but it assigns
#'   ranks according to Pareto dominance, where rank 1 indicates those
#'   solutions not dominated by any other solution in the input set.
#'   Duplicated points are kept on the same front. The resulting ranking can be
#'   used to partition points into a list of matrices, each matrix representing
#'   a nondominated front (see examples below)
#'
#'   More formally, given a finite set of points \eqn{X \subset \mathbb{R}^m},
#'   the rank of a point \eqn{x \in X} is defined as:
#'
#'   \deqn{\operatorname{rank}(x) = r \iff x \in F^c_{r} \land \nexists y \in F^c_{r}, y \prec x}
#'
#'   where \eqn{y \prec x} means that \eqn{y} dominates \eqn{x} according to
#'   Pareto optimality, \eqn{F^c_r = X \setminus \bigcup_{i=1}^{r-1} F_i} and
#'   \eqn{F_r = \{x \in X \land \operatorname{rank}(x) = r\}}.  The sets
#'   \eqn{F_c}, with \eqn{c=1,\dots,k}, partition \eqn{X} into \eqn{k}
#'   *fronts*, that is, mutually nondominated subsets of \eqn{X}.
#'
#'   Let \eqn{m} be the number of dimensions. With \eqn{m=2}, i.e.,
#'   `ncol(data)=2`, the code uses the \eqn{O(n \log n)} algorithm by
#'   \citet{Jen03}.  When \eqn{m=3}, it uses a \eqn{O(k\cdot n \log n)}
#'   algorithm, where \eqn{k} is the number of fronts in the output.  With
#'   higher dimensions, it uses the naive \eqn{O(k m n^2)} algorithm.
#'
#' @references
#'
#' \insertAllCited{}
#'
#' @export
pareto_rank <- function(x, maximise = FALSE)
{
  x <- as_double_matrix_1(x)
  if (ncol(x) == 1L) { # Handle single-objective
    x <- as.vector(x)
    if (maximise)
      x <- -x
    # FIXME: Can we do this faster?
    return(match(x, sort(unique(x))))
  }
  x <- transform_maximise(x, maximise)
  .Call(pareto_ranking_C, t(x))
}
