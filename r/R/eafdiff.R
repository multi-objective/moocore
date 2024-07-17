#' Compute empirical attainment function differences
#'
#' Calculate the differences between the empirical attainment functions of two
#' data sets.
#'
#' @param x,y Data frames corresponding to the input data of
#'   left and right sides, respectively. Each data frame has at least three
#'   columns, the third one being the set of each point. See also
#'   [read_datasets()].
#'
#' @param intervals (`integer(1)`) \cr The absolute range of the differences
#'   \eqn{[0, 1]} is partitioned into the number of intervals provided.
#'
#' @template arg_maximise
#'
#' @param rectangles If TRUE, the output is in the form of rectangles of the same color.
#'
#' @details
#'   This function calculates the differences between the EAFs of two
#'   data sets.
#'
#' @return With `rectangle=FALSE`, a `data.frame` containing points where there
#'   is a transition in the value of the EAF differences.  With
#'   `rectangle=TRUE`, a `matrix` where the first 4 columns give the
#'   coordinates of two corners of each rectangle. In both cases, the last
#'   column gives the difference in terms of sets in `x` minus sets in `y` that
#'   attain each point (i.e., negative values are differences in favour `y`).
#'
#' @seealso    [read_datasets()]
# , [mooplot::eafdiffplot()]
#'
#' @doctest
#' A1 <- read_datasets(text='
#'  3 2
#'  2 3
#'
#'  2.5 1
#'  1 2
#'
#'  1 2
#' ')
#'
#' A2 <- read_datasets(text='
#'  4 2.5
#'  3 3
#'  2.5 3.5
#'
#'  3 3
#'  2.5 3.5
#'
#'  2 1
#' ')
#' d <- eafdiff(A1, A2)
#' str(d)
#' @testRaw  expect_equal(d,matrix(byrow=TRUE, ncol=3, scan(quiet = TRUE, text='1.0  2.0    2
#' @testRaw   2.0  1.0   -1
#' @testRaw   2.5  1.0    0
#' @testRaw   2.0  2.0    1
#' @testRaw   2.0  3.0    2
#' @testRaw   3.0  2.0    2
#' @testRaw   2.5  3.5    0
#' @testRaw   3.0  3.0    0
#' @testRaw   4.0  2.5    1')))
#' @expect true(is.matrix(.))
#' d
#'
#' d <- eafdiff(A1, A2, rectangles = TRUE)
#' str(d)
# @expect value(as.matrix(read.table(header=TRUE, text='
#       xmin ymin xmax ymax diff
#       2.0  1.0  2.5  2.0   -1
#       1.0  2.0  2.0  Inf    2
#       2.5  1.0  Inf  2.0    0
#       2.0  2.0  3.0  3.0    1
#       2.0  3.5  2.5  Inf    2
#       2.0  3.0  3.0  3.5    2
#       3.0  2.5  4.0  3.0    2
#       3.0  2.0  Inf  2.5    2
#       4.0  2.5  Inf  3.0    1')))
#' d
#'@concept eaf
#'@export
eafdiff <- function(x, y, intervals = NULL, maximise = FALSE, rectangles = FALSE)
{
  stopifnot(ncol(x) == ncol(y))

  sets_x <- x[, ncol(x)]
  if (anyNA(sets_x)) stop("'sets' must have only non-NA numerical values")
  cumsizes_x <- cumsum(unique_counts(sets_x))
  # The C code expects points within a set to be contiguous.
  x <- as_double_matrix(x[order(sets_x) , -ncol(x), drop=FALSE])

  sets_y <- y[, ncol(y)]
  if (anyNA(sets_y)) stop("'sets' must have only non-NA numerical values")
  cumsizes_y <- cumsum(unique_counts(sets_y))
  # The C code expects points within a set to be contiguous.
  y <- as_double_matrix(y[order(sets_y), -ncol(y), drop=FALSE])

  nsets <- length(cumsizes_x) + length(cumsizes_y)
  if (is.null(intervals)) {
    # Default is nsets / 2
    intervals <- nsets / 2.0
  } else {
    stopifnot(length(intervals) == 1L)
    intervals <- min(intervals, nsets / 2.0)
  }

  maximise <- as.logical(maximise)
  if (any(maximise)) {
    x <- transform_maximise(x, maximise)
    y <- transform_maximise(y, maximise)
  }

  DIFF <- compute_eafdiff_call(x, y, cumsizes_x, cumsizes_y,
    intervals = intervals, ret = if (rectangles) "rectangles" else "points")
  # FIXME: We should remove duplicated rows in C code.
  # FIXME: Check that we do not generate duplicated nor overlapping rectangles
  # with different colors. That would be a bug.
  DIFF <- DIFF[!duplicated(DIFF),]
  if (any(maximise)) {
    if (rectangles && length(maximise) != 1L)
      maximise <- c(maximise, maximise)
    DIFF[, -ncol(DIFF)] <- transform_maximise(DIFF[, -ncol(DIFF), drop=FALSE], maximise)
  }
  # Undo previous transformation
  ## if (any(maximise)) {
  ##   DIFF[,-ncol(x)] <- transform_maximise(x[, -ncol(x), drop=FALSE], maximise)
  ## }
  DIFF
}

#' Same as [eafdiff()] but performs no checks and does not transform the input or
#' the output. This function should be used by other packages that want to
#' avoid redundant checks and transformations.
#'
#' @seealso [as_double_matrix()] [transform_maximise()]
#' @inheritParams eafdiff
#' @param cumsizes_x,cumsizes_y Cumulative size of the different sets of points in `x`.
#' @param ret (`"points"|"rectangles"|"polygons"`)\cr The format of the returned EAF differences.
#' @concept eaf
#' @export
compute_eafdiff_call <- function(x, y, cumsizes_x, cumsizes_y, intervals, ret)
{
  x <- cbind(t(x), t(y))
  cumsizes <- c(cumsizes_x, cumsizes_x[length(cumsizes_x)] + cumsizes_y)
  intervals <- as.integer(intervals)
  switch(ret,
    rectangles = .Call(compute_eafdiff_rectangles_C, x, cumsizes, intervals),
    polygons = .Call(compute_eafdiff_polygon_C, x, cumsizes, intervals),
    points = .Call(compute_eafdiff_C, x, cumsizes, intervals))
}

compute_eafdiff <- function(x, y, sets_x, sets_y, intervals)
  compute_eafdiff_call(x, y, cumsum(unique_counts(sets_x)), cumsum(unique_counts(sets_y)), intervals, ret = "points")

# FIXME: The default intervals should be nsets / 2
compute_eafdiff_rectangles <- function(x, y, sets_x, sets_y, intervals)
  compute_eafdiff_call(x, y, cumsum(unique_counts(sets_x)), cumsum(unique_counts(sets_y)), intervals, ret = "rectangles")

# FIXME: The default intervals should be nsets / 2
compute_eafdiff_polygon <- function(x, y, sets_x, sets_y, intervals)
  compute_eafdiff_call(x, y, cumsum(unique_counts(sets_x)), cumsum(unique_counts(sets_y)), intervals, ret = "polygons")



## compute_eafdiff <- function(data, intervals)
## {
##   DIFF <- compute_eafdiff_call(data[,c(1L,2L), drop=FALSE], sets = data[,3L], intervals = intervals)
##   #print(DIFF)
##   # FIXME: Do this computation in C code. See compute_eafdiff_area_C
##   setcol <- ncol(data)
##   eafval <- DIFF[, setcol]
##   eafdiff <- list(left  = unique(DIFF[ eafval >= 1L, , drop=FALSE]),
##                   right = unique(DIFF[ eafval <= -1L, , drop=FALSE]))
##   eafdiff$right[, setcol] <- -eafdiff$right[, setcol]
##   eafdiff
## }
