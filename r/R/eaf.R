#' Exact computation of the Empirical Attainment Function (EAF)
#'
#' This function computes the EAF given a set of 2D or 3D points and a vector
#' `set` that indicates to which set each point belongs.
#'
#' @param x `matrix()`|`data.frame()`\cr Matrix or data frame of numerical
#'   values that represents multiple sets of points, where each row represents
#'   a point. If `sets` is missing, the last column of `x` gives the sets.
#'
#' @param sets `integer()`\cr Vector that indicates the set of each point in
#'   `x`. If missing, the last column of `x` is used instead.
#'
#' @param percentiles `numeric()`\cr Vector indicating which percentiles are
#'   computed.  `NULL` computes all.
#'
#' @inheritParams is_nondominated
#'
#' @param groups `factor()`\cr Indicates that the EAF must be computed
#'   separately for data belonging to different groups.
#'
#' @return `data.frame()`\cr A data frame containing the exact representation
#'   of EAF. The last column gives the percentile that corresponds to each
#'   point. If groups is not `NULL`, then an additional column indicates to
#'   which group the point belongs.
#'
#' @author  Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
#'
#' @details
#'
#' Given a set \eqn{A \subset \mathbb{R}^d}, the attainment function of
#' \eqn{A}, denoted by \eqn{\alpha_{A}\colon \mathbb{R}^d\to \{0,1\}},
#' specifies which points in the objective space are weakly dominated by
#' \eqn{A}, where \eqn{\alpha_A(\vec{z}) = 1} if \eqn{\exists \vec{a} \in A,
#' \vec{a} \leq \vec{z}}, and \eqn{\alpha_A(\vec{z}) = 0}, otherwise.
#'
#' Let \eqn{\mathcal{A} = \{A_1, \dots, A_n\}} be a multi-set of \eqn{n} sets
#' \eqn{A_i \subset \mathbb{R}^d}, the EAF \citep{Grunert01,GruFon2009:emaa} is
#' the function \eqn{\hat{\alpha}_{\mathcal{A}}\colon \mathbb{R}^d\to [0,1]},
#' such that:
#'
#'  \deqn{\hat{\alpha}_{\mathcal{A}}(\vec{z}) = \frac{1}{n}\sum_{i=1}^n \alpha_{A_i}(\vec{z})}
#'
#' The EAF is a coordinate-wise non-decreasing step function, similar to the
#' empirical cumulative distribution function (ECDF)
#' \citep{LopVerDreDoe2025}.  Thus, a finite representation of the EAF
#' can be computed as the set of minima, in terms of Pareto optimality, with a
#' value of the EAF not smaller than a given \eqn{t/n}, where
#' \eqn{t=1,\dots,n} \citep{FonGueLopPaq2011emo}. Formally, the EAF can
#' be represented by the sequence \eqn{(L_1, L_2, \dots, L_n)}, where:
#'
#' \deqn{L_t = \min \{\vec{z} \in \mathbb{R}^d : \hat{\alpha}_{\mathcal{A}}(\vec{z}) \geq t/n\}}
#'
#' It is also common to refer to the \eqn{k\% \in [0,100]} percentile. For
#' example, the *median* (or 50%) attainment surface corresponds to
#' \eqn{L_{\lceil n/2 \rceil}} and it is the lower boundary of the vector space
#' attained by at least 50% of the input sets \eqn{A_i}. Similarly, \eqn{L_1}
#' is called the *best* attainment surface (\eqn{\frac{1}{n}}%) and
#' represents the lower boundary of the space attained by at least one input
#' set, whereas \eqn{L_{100}} is called the *worst* attainment surface (100%)
#' and represents the lower boundary of the space attained by all input sets.
#'
#' In the current implementation, the EAF is computed using the algorithms
#' proposed by \citet{FonGueLopPaq2011emo}, which have complexity \eqn{O(m\log
#' m + nm)} in 2D and \eqn{O(n^2 m \log m)} in 3D, where \eqn{n} is the number
#' of input sets and \eqn{m} is the total number of input points.
#'
#' @note
#'
#' There are several examples of data sets in
#' `system.file(package="moocore","extdata")`.  The current implementation only
#' supports two and three dimensional points.
#'
#' @references
#'
#' \insertAllCited{}
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
#' @param x `list()`\cr List of `data.frames` or matrices. The names of the list
#'   give the percentiles of the attainment surfaces.  This is the format
#'   returned by [eaf_as_list()].
# FIXME: and [mooplot::eafplot()].
#'
#' @return `data.frame()`\cr Data frame with as many columns as objectives and an additional column `percentiles`.
#'
#' @seealso [eaf_as_list()]
#' @examples
#'
#' data(SPEA2relativeRichmond)
#' attsurfs <- eaf_as_list(eaf(SPEA2relativeRichmond, percentiles = c(0,50,100)))
#' str(attsurfs)
#' eaf_df <- attsurf2df(attsurfs)
#' str(eaf_df)
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
#' @inherit eaf title params return
#' @param cumsizes `integer()`\cr Cumulative size of the different sets of points in `x`.
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
#' @param eaf `data.frame()`|`matrix()`\cr Data frame or matrix that represents the EAF.
#'
#' @return `list()`\cr A list of data frames. Each `data.frame` represents one attainment surface.
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
