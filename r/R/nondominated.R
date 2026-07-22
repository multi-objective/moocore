#' Identify and remove dominated points according to Pareto optimality
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
#' Given \eqn{n} points of dimension \eqn{m}, the current implementation always
#' uses the best-known \eqn{O(n \log n)} dimension-sweep algorithm
#' \citep{KunLucPre1975jacm} for \eqn{m \leq 3}. For \eqn{m \geq 4}, functions
#' `is_nondominated()` and `filter_dominated()` use the best-known \eqn{O(n
#' \log^{m-2} n)} algorithm \citep{KunLucPre1975jacm} when \eqn{n > `r moocore:::.libmoocore_constants[["KUNG_SMALL_THRESHOLD"]]`}, and
#' the naive \eqn{O(m n^2)} algorithm otherwise.  Function `any_dominated()`
#' always uses the naive algorithm for \eqn{m \geq 4}.
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
#' @references
#'
#' \insertAllCited{}
#'
#' @seealso [pareto_rank()]
#' @concept dominance
#' @export
is_nondominated <- function(x, maximise = FALSE, keep_weakly = FALSE)
{
  x <- as_double_matrix_1(x)
  nobjs <- ncol(x)
  keep_weakly <- as.logical(keep_weakly)
  maximise <- as.logical(maximise)
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
  check_dimension_max(nobjs, .libmoocore_constants[["MOOCORE_DIMENSION_MAX"]])
  .Call(is_nondominated_C,
    x,
    keep_weakly,
    rep_len(maximise, nobjs))
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
  if (keep_weakly) # FIXME: Do this in C.
    x <- x[!duplicated(x), , drop = FALSE]
  nobjs <- ncol(x)
  check_dimension_max(nobjs, .libmoocore_constants[["MOOCORE_DIMENSION_MAX"]])
  .Call(any_dominated_C,
    x,
    rep_len(as.logical(maximise), nobjs))
}

#' Rank points according to Pareto-optimality (nondominated sorting).
#'
#' [pareto_rank()] is meant to be used like `rank()`, but it assigns ranks
#' according to Pareto dominance, where rank 1 indicates those solutions not
#' dominated by any other solution in the input set.  Duplicated points are
#' assigned the same rank.  The resulting ranking can be used to partition
#' points into a list of matrices, each matrix representing a nondominated
#' front \citep{Deb02nsga2} (see examples below).
#'
#' @inherit is_nondominated params
#'
#' @return An integer vector of the same length as the number of rows of the
#'   input `x` with values within `[1, nrow(points)]`, where each value gives
#'   the rank of each point (lower is better).
#'
#' @details
#'
#' Given \eqn{x,y \in\mathbb{R}^m}, let \eqn{x \prec y} denote that \eqn{x}
#' dominates \eqn{y} according to Pareto optimality.
#'
#' Nondominated sorting, also called the *layers-of-maxima problem* \citep{BucGoo2004maxima},
#' partitions a finite set of points \eqn{X \subset \mathbb{R}^m} into an ordered
#' sequence of *fronts*, \eqn{F_1, F_2, \dots, F_k}, where
#' \eqn{1 \leq k \leq |X|}, satisfying the following conditions:
#'
#' \enumerate{
#'   \item All points are allocated to a front:
#'   \deqn{\bigcup_{i=1}^{k} F_i = X}
#'
#'   \item Each point is allocated to only one front:
#'   \deqn{F_i \cap F_j = \emptyset,\; \forall i,j \in \{1,2,\dots,k\},\, i \neq j}
#'
#'   \item Points within a front are mutually nondominated:
#'   \deqn{\nexists x,y \in F_i,\; x \prec y,\; \forall i\in\{1,2,\dots,k\}}
#'
#'   \item The first front contains points not dominated by any other point:
#'   \deqn{\forall y \in F_1,\; \nexists x \in X,\; x \prec y}
#'
#'   \item Every point in front \eqn{i} is dominated by at least one point in front
#'   \eqn{i-1}:
#'   \deqn{\forall y \in F_i,\; \exists x \in F_{i-1},\; x \prec y,\; \forall i\in\{2,3,\dots,k\}}
#' }
#'
#' The rank returned by [pareto_rank()] is the index \eqn{i \in
#' \{1,2,\dots,k\}} of the front \eqn{F_i} allocated to each point. If all
#' points are mutually nondominated, there is only one front (\eqn{k = 1}). If
#' each front contains one point, then \eqn{k = n = |X|}.
#'
#' With \eqn{m=2}, i.e., `ncol(data)=2`, the code uses the best-known \eqn{O(n
#' \log n)} algorithm by \citet{Jen03}.  When \eqn{m \geq 3}, it uses the naive
#' algorithm that identifies one *front* at a time, which requires
#' \eqn{O(n^2\log n)} for \eqn{m=3}, and \eqn{O(n^2 \log^{m-2} n)} for \eqn{m
#' \geq 4}.
#'
#' @doctest
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
#' ranks <- pareto_rank(set)
#' str(ranks)
#' if (requireNamespace("graphics", quietly = TRUE)) {
#'    colors <- colorRampPalette(c("red","yellow","springgreen","royalblue"))(max(ranks))
#'    plot(set, col = colors[ranks], type = "p", pch = 20)
#' }
#'
#' @references
#'
#' \insertAllCited{}
#'
#' @seealso [is_nondominated()]
#' @concept dominance
#' @export
pareto_rank <- function(x, maximise = FALSE)
{
  x <- as_double_matrix_1(x)
  nobjs <- ncol(x)
  if (nobjs == 1L) { # Handle single-objective
    x <- as.vector(x)
    if (maximise)
      x <- -x
    return(match(x, sort(unique(x)))) # FIXME: Can we do this faster?
  }
  check_dimension_max(nobjs, .libmoocore_constants[["MOOCORE_DIMENSION_MAX"]])
  x <- transform_maximise(x, maximise)
  .Call(pareto_ranking_C, t(x))
}
