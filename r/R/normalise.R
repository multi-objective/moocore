#' Normalise points
#'
#' Normalise points per coordinate to a range, e.g., `c(1,2)`, where the
#' minimum value will correspond to 1 and the maximum to 2. If bounds are
#' given, they are used for the normalisation.
#'
#' @inheritParams is_nondominated
#'
#' @param to_range `numerical(2)`\cr Normalise values to this range. If the objective is
#'   maximised, it is normalised to `c(to_range[1], to_range[0])`
#'   instead.
#'
#' @param lower,upper `numerical()`\cr Bounds on the values. If `NA`, the maximum and minimum
#'   values of each coordinate are used.
#'
#' @return `matrix()`\cr A numerical matrix
#'
#' @author Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
#'
#' @examples
#'
#' data(SPEA2minstoptimeRichmond)
#' # The second objective must be maximized
#' head(SPEA2minstoptimeRichmond[, 1:2])
#'
#' head(normalise(SPEA2minstoptimeRichmond[, 1:2], maximise = c(FALSE, TRUE)))
#'
#' head(normalise(SPEA2minstoptimeRichmond[, 1:2], to_range = c(0,1), maximise = c(FALSE, TRUE)))
#'
#' @export
normalise <- function(x, to_range = c(1, 2), lower = NA, upper = NA, maximise = FALSE)
{
  x <- as_double_matrix(x)
  nobjs <- ncol(x)
  lower <- rep_len(as.double(lower), nobjs)
  upper <- rep_len(as.double(upper), nobjs)
  # Handle NA
  no.lower <- is.na(lower)
  no.upper <- is.na(upper)
  minmax <- colRanges(x)
  lower[no.lower] <- minmax[no.lower, 1L]
  upper[no.upper] <- minmax[no.upper, 2L]
  maximise <- rep_len(as.logical(maximise), nobjs)

  if (length(to_range) != 2L)
    stop("'to_range' must be a vector of length 2")

  x <- t.default(x)
  .Call(normalise_C,
    x, # This is modified by normalise_C
    as.double(to_range),
    lower, upper, maximise)
  # FIXME: Transposing in C may avoid a copy.
  t.default(x)
}
