range_df_list <- function(x, col)
{
  # FIXME: How to make this faster?
  do.call(range, lapply(x, `[`, , col))
}

get_ideal <- function(x, maximise)
{
  # FIXME: Is there a better way to do this?
  minmax <- colRanges(as.matrix(x))
  lower <- minmax[,1L]
  upper <- minmax[,2L]
  ifelse(maximise, upper, lower)
}

nunique <- function(x) length(unique.default(x))

# FIXME: There must be something faster than table
unique_counts <- function(x) as.vector(table(x))

#' Transform matrix according to maximise parameter
#'
#' @inheritParams is_nondominated
#'
#' @return `x` transformed such that every column where `maximise` is `TRUE` is multiplied by `-1`.
#'
#' @examples
#' x <- data.frame(f1=1:10, f2=101:110)
#' rownames(x) <- letters[1:10]
#' transform_maximise(x, maximise=c(FALSE,TRUE))
#' transform_maximise(x, maximise=TRUE)
#' x <- as.matrix(x)
#' transform_maximise(x, maximise=c(FALSE,TRUE))
#' transform_maximise(x, maximise=TRUE)
#'
#' @export
transform_maximise <- function(x, maximise)
{
  if (any(maximise)) {
    if (length(maximise) == 1L)
      return(-x)
    if (length(maximise) != ncol(x))
      stop("length of maximise must be either 1 or ncol(x)")
    if (all(maximise))
      return(-x)
    x[,maximise] <- -x[,maximise, drop=FALSE]
  }
  x
}

#' Convert input to a matrix with `"double"` storage mode ([base::storage.mode()]).
#'
#' @param x `data.frame()`|`matrix()`\cr A numerical data frame or matrix with at least 1 row and 2 columns.
#' @return `x` is coerced to a numerical `matrix()`.
#' @export
as_double_matrix <- function(x)
{
  name <- deparse(substitute(x))   # FIXME: Only do this if there is an error.
  if (length(dim(x)) != 2L)
    stop("'", name, "' must be a data.frame or a matrix")
  if (nrow(x) < 1L)
    stop("not enough points (rows) in '", name, "'")
  if (ncol(x) < 2L)
    stop("'", name, "' must have at least 2 columns")
  x <- as.matrix(x)
  if (!is.numeric(x))
    stop("'", name, "' must be numeric")
  if (storage.mode(x) != "double")
    storage.mode(x) <- "double"
  x
}

is_wholenumber <- function(x, tol = .Machine$double.eps^0.5)
  is.finite(x) && abs(x - round(x)) < tol

as_integer <- function(x)
{
  if (!is_wholenumber(x)) {
    stop("'", deparse(substitute(x)), "' is not an integer: ", x)
  }
  as.integer(x)
}
