#' Exact computation of the EAF in 2D or 3D
#'
#' This function computes the EAF given a set of 2D or 3D points and a vector `set`
#' that indicates to which set each point belongs.
#'
#' @template arg_x_data
#'
#' @template arg_sets
#'
#' @template arg_percentiles
#'
#' @template arg_maximise
#'
#' @param groups Indicates that the EAF must be computed separately for data
#'   belonging to different groups.
#'
#' @return A data frame (`data.frame`) containing the exact representation of
#'   EAF. The last column gives the percentile that corresponds to each
#'   point. If groups is not `NULL`, then an additional column indicates to
#'   which group the point belongs.
#'
#' @author  Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
#'
#' @note There are several examples of data sets in
#'   `system.file(package="moocore","extdata")`.  The current implementation
#'   only supports two and three dimensional points.
#'
#' @references
#'
#' \insertRef{Grunert01}{moocore}
#'
#' \insertRef{FonGueLopPaq2011emo}{moocore}
#'
#'@seealso [read_datasets()]
#'
#'@examples
#' extdata_path <- system.file(package="moocore", "extdata")
#'
#' x <- read_datasets(file.path(extdata_path, "example1_dat"))
#' # Compute full EAF (sets is the last column)
#' str(eaf(x))
#'
#' # Compute only best, median and worst
#' str(eaf(x[,1:2], sets = x[,3], percentiles = c(0, 50, 100)))
#'
#' x <- read_datasets(file.path(extdata_path, "spherical-250-10-3d.txt"))
#' y <- read_datasets(file.path(extdata_path, "uniform-250-10-3d.txt"))
#' x <- rbind(data.frame(x, groups = "spherical"),
#'            data.frame(y, groups = "uniform"))
#' # Compute only median separately for each group
#' z <- eaf(x[,1:3], sets = x[,4], groups = x[,5], percentiles = 50)
#' str(z)
#' # library(plotly)
#' # plot_ly(z, x = ~X1, y = ~X2, z = ~X3, color = ~groups,
#' #         colors = c('#BF382A', '#0C4B8E')) %>% add_markers()
#' @concept eaf
#' @export
eaf <- function (x, sets, percentiles = NULL, maximise = FALSE, groups = NULL)
{
  if (missing(sets)) {
    sets <- x[, ncol(x)]
    x <- x[, -ncol(x), drop=FALSE]
  }

  x <- as_double_matrix(x)
  maximise <- as.logical(maximise)
  if (any(maximise))
    x <- transform_maximise(x, maximise)

  if (!is.null(percentiles)) {
    # FIXME: We should handle only integral levels inside the C code.
    percentiles <- unique.default(sort.int(as.numeric(percentiles)))
  }

  if (is.null(groups))
    return(compute_1_eaf(x, sets = sets, percentiles = percentiles, maximise = maximise))

  groups <- factor(groups)
  # FIXME: Is there a multi-variate tapply? Maybe data.table?
  do.call("rbind", lapply(levels(groups), function(g){
    where <- groups == g
    data.frame(compute_1_eaf(x[where, , drop=FALSE], sets = sets[where], percentiles = percentiles, maximise = maximise),
      groups = g)
  }))
}

#' Convert a list of attainment surfaces to a single EAF `data.frame`.
#'
#' @param x (`list()`) List of `data.frames` or matrices. The names of the list
#'   give the percentiles of the attainment surfaces.  This is the format
#'   returned by [eaf_as_list()] and [mooplot::eafplot()].
#'
#' @return A `data.frame` with as many columns as objectives and an additional column `percentiles`.
#'
#' @seealso [eaf_as_list()]
#' @examples
#'
#' data(SPEA2relativeRichmond)
#' attsurfs <- eaf_as_list(eaf(SPEA2relativeRichmond, percentiles = c(0,50,100)))
#' str(attsurfs)
#' eaf_df <- attsurf2df(attsurfs)
#' str(eaf_df)
#' # attsurfs <- eafplot (SPEA2relativeRichmond, percentiles = c(0,50,100),
#' #                      xlab = expression(C[E]), ylab = "Total switches",
#' #                      lty=0, pch=21, xlim = c(90, 140), ylim = c(0, 25))
#' # attsurfs <- attsurf2df(attsurfs)
#' # text(attsurfs[,1:2], labels = attsurfs[,3], adj = c(1.5,1.5))
#'
#' @concept eaf
#' @export
attsurf2df <- function(x)
{
  if (!is.list(x) || is.data.frame(x))
    stop("'x' must be a list of data.frames or matrices")

  percentiles <- as.numeric(names(x))
  percentiles <- rep.int(percentiles, sapply(x, nrow))
  x <- do.call("rbind", x)
  # Remove duplicated points (keep only the higher values)
  uniq <- !duplicated(x, fromLast = TRUE)
  cbind(x[uniq, , drop = FALSE], percentiles = percentiles[uniq])
}

compute_1_eaf <- function(x, sets, percentiles, maximise)
{
  # This function assumes that x has been already transformed according to maximise.
  if (anyNA(sets)) stop("'sets' must have only non-NA numerical values")
  order_sets <- order(sets)
  sets <- sets[order_sets]
  # The C code expects points within a set to be contiguous.
  x <- x[order_sets, , drop=FALSE]
  nsets <- nunique(sets)
  ## if (is.null(percentiles)) {
  ##   # FIXME: We should compute this in the C code.
  ##   percentiles <- 1L:nsets * (100 / nsets)
  ## }
  x <- compute_eaf_call(x, cumsizes = cumsum(unique_counts(sets)), percentiles = percentiles)
  # Undo previous transformation
  if (any(maximise))
      x[,-ncol(x)] <- transform_maximise(x[, -ncol(x), drop=FALSE], maximise)
  x
}

#' Same as [eaf()] but performs no checks and does not transform the input or
#' the output. This function should be used by other packages that want to
#' avoid redundant checks and transformations.
#'
#' @seealso [as_double_matrix()] [transform_maximise()]
#' @inheritParams eaf
#' @param cumsizes Cumulative size of the different sets of points in `x`.
#' @concept eaf
#' @export
compute_eaf_call <- function(x, cumsizes, percentiles)
  .Call(compute_eaf_C,
    t(x),
    cumsizes,
    percentiles)

#' Convert an EAF data frame to a list of data frames, where each element
#' of the list is one attainment surface. The function [attsurf2df()] can be
#' used to convert the list into a single data frame.
#'
#' @param eaf (`data.frame()`|`matrix()`) Data frame or matrix that represents the EAF.
#'
#' @return (`list()`) A list of data frames. Each `data.frame` represents one attainment surface.
#'
#' @seealso [eaf()] [attsurf2df()]
#'
#' @examples
#' extdata_path <- system.file(package="moocore", "extdata")
#' x <- read_datasets(file.path(extdata_path, "example1_dat"))
#' attsurfs <- eaf_as_list(eaf(x, percentiles = c(0, 50, 100)))
#' str(attsurfs)
#' @concept eaf
#' @export
eaf_as_list <- function(eaf)
{
  # FIXME: Add groups argument.
  setcol <- ncol(eaf)
  if (!is.null(colnames(eaf)) && colnames(eaf)[setcol] == "groups") {
    return(split.data.frame(eaf[, -c(setcol-1L, setcol), drop=FALSE],
      list(eaf[, setcol-1L], eaf[, setcol]), sep="-"))
  }
  split.data.frame(eaf[,-setcol, drop=FALSE], factor(eaf[,setcol]))
}

### Local Variables:
### ess-indent-level: 2
### ess-continued-statement-offset: 2
### ess-brace-offset: 0
### ess-expression-offset: 4
### ess-else-offset: 0
### ess-brace-imaginary-offset: 0
### ess-continued-brace-offset: 0
### ess-arg-function-offset: 2
### ess-close-brace-offset: 0
### indent-tabs-mode: nil
### ess-fancy-comments: nil
### End:
