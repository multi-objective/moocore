#' Combine datasets `x` and `y` by row taking care of making all sets unique.
#'
#' @param x,y `matrix`|`data.frame()`\cr Each dataset has at least three
#'   columns, the last one is the set of each point. See also
#'   [read_datasets()].
#'
#' @return `matrix()|`data.frame()`\cr A dataset.
#' @examples
#' x <- data.frame(f1 = 5:10, f2 = 10:5, set = 1:6)
#' y <- data.frame(f1 = 15:20, f2 = 20:15, set = 1:6)
#' rbind_datasets(x,y)
#' @export
rbind_datasets <- function(x, y)
{
  setcol <- ncol(x)
  stopifnot(setcol > 2L)
  stopifnot(ncol(x) == ncol(y))
  # FIXME: We could relax this condition by re-encoding  the column.
  stopifnot(min(x[,setcol]) == 1L)
  stopifnot(min(y[,setcol]) == 1L)
  # We have to make all sets unique.
  y[,setcol] <- y[,setcol] + max(x[,setcol])
  rbind(x, y)
}


combine_cumsizes_sets <- function(sets_x, sets_y)
{
  cumsizes <- cumsum(unique_counts(sets_x))
  c(cumsizes, cumsizes[length(cumsizes)] + cumsum(unique_counts(sets_y)))
}
