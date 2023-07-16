#' Identify largest EAF differences
#'
#' Given a list of datasets, return the indexes of the pair with the largest
#' EAF differences according to the method proposed by \citet{DiaLop2020ejor}.
#'
#'
#' @param x (`list()`) A list of matrices with at least 3 columns.
#'
#' @template arg_maximise
#'
#' @param intervals (`integer(1)`) \cr The absolute range of the differences
#'   \eqn{[0, 1]} is partitioned into the number of intervals provided.
#'
#' @template arg_refpoint
#'
#' @template arg_ideal_null
#'
#' @return  (`list()`) A list with two components `pair` and `value`.
#'
#'@examples
#' # FIXME: This example is too large, we need a smaller one.
#' data(tpls50x20_1_MWT)
#' nadir <- apply(tpls50x20_1_MWT[,2:3], 2L, max)
#' x <- largest_eafdiff(split.data.frame(tpls50x20_1_MWT[,2:4], tpls50x20_1_MWT[, 1L]),
#'                      reference = nadir)
#' str(x)
#'
#'@references
#' \insertAllCited{}
#'
#'@concept eaf
#'@export
largest_eafdiff <- function(x, maximise = FALSE, intervals = 5L, reference,
                            ideal = NULL)
{
  # FIXME: Check the input data makes sense
  nobjs <- 2L
  maximise <- as.logical(rep_len(maximise, nobjs))
  if (nobjs != 2L) stop("Only 2 objectives currently supported")

  n <- length(x)
  stopifnot(n > 1L)
  best_pair <- NULL
  best_value <- 0
  if (is.null(ideal)) {
    r1 <- range_df_list(x, 1L)
    r2 <- range_df_list(x, 2L)
    upper <- c(r1[2L], r2[2L])
    lower <- c(r1[1L], r2[1L])
    ideal <- ifelse(maximise, upper, lower)
  }
  # Convert to a 1-row matrix
  if (is.null(dim(ideal))) dim(ideal) <- c(1L,nobjs)

  for (a in seq_len(n-1)) {
    for (b in (a+1):n) {
      DIFF <- eafdiff(x[[a]], x[[b]], intervals = intervals,
                      maximise = maximise, rectangles = TRUE)
      # Set color to 1
      a_rectangles <- DIFF[ DIFF[, 5] >= 1L, , drop = FALSE]
      a_rectangles[, ncol(a_rectangles)] <- 1

      a_value <- whv_rect(ideal, a_rectangles, reference, maximise)

      b_rectangles <- DIFF[ DIFF[, 5] <= -1L, , drop = FALSE]
      b_rectangles[, ncol(b_rectangles)] <- 1
      b_value <- whv_rect(ideal, b_rectangles, reference, maximise)
      value <- min(a_value, b_value)
      if (value > best_value) {
        best_value <- value
        best_pair <- c(a, b)
      }
    }
  }
  list(pair=best_pair, value = best_value)
}

#' Interactively choose according to empirical attainment function differences
#'
#' @param x (`matrix()`) Matrix of rectangles representing EAF differences
#'   returned by [eafdiff()] with `rectangles=TRUE`.
#'
#' @param left (`logical(1)`) With `left=TRUE` return the rectangles with
#'   positive differences, otherwise return those with negative differences but
#'   differences are converted to positive.
#'
#' @return `matrix` where the first 4 columns give the coordinates of two
#'   corners of each rectangle and the last column. In both cases, the last
#'   column gives the positive differences in favor of the chosen side.
#'
#' @examples
#' \donttest{
#' extdata_dir <- system.file(package="moocore", "extdata")
#' A1 <- read_datasets(file.path(extdata_dir, "wrots_l100w10_dat"))
#' A2 <- read_datasets(file.path(extdata_dir, "wrots_l10w100_dat"))
#' # Choose A1
#' rectangles <- eafdiff(A1, A2, intervals = 5, rectangles = TRUE)
#' rectangles <- choose_eafdiff(rectangles, left = TRUE)
#' reference <- c(max(A1[, 1], A2[, 1]), max(A1[, 2], A2[, 2]))
#' x <- split.data.frame(A1[,1:2], A1[,3])
#' hv_A1 <- sapply(split.data.frame(A1[, 1:2], A1[, 3]),
#'                  hypervolume, reference=reference)
#' hv_A2 <- sapply(split.data.frame(A2[, 1:2], A2[, 3]),
#'                  hypervolume, reference=reference)
#' print(fivenum(hv_A1))
#' print(fivenum(hv_A2))
#' whv_A1 <- sapply(split.data.frame(A1[, 1:2], A1[, 3]),
#'                  whv_rect, rectangles=rectangles, reference=reference)
#' whv_A2 <- sapply(split.data.frame(A2[, 1:2], A2[, 3]),
#'                  whv_rect, rectangles=rectangles, reference=reference)
#' print(fivenum(whv_A1))
#' print(fivenum(whv_A2))
#' }
#'
#'@concept eaf
#'@export
choose_eafdiff <- function(x, left = stop("'left' must be either TRUE or FALSE"))
{
  if (left) return (x[ x[, ncol(x)] > 0L, , drop = FALSE])
  x <- x[ x[, ncol(x)] < 0L, , drop = FALSE]
  # We always return positive colors.
  x[, ncol(x)] <- abs(x[, ncol(x)])
  x
}
