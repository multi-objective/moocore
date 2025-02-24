#' Approximate the hypervolume indicator.
#'
#' Approximate the value of the hypervolume metric with respect to a given
#' reference point assuming minimization of all objectives. The default
#' `method="DZ2019-HW"` is deterministic and ignores the parameter `seed`,
#' while `method="DZ2019-MC"` relies on Monte-Carlo sampling
#' \citep{DenZha2019approxhv}.  Both methods tend to get more accurate, for
#' higher values of `nsamples`, but the increase in accuracy is not monotonic
#' as shown in the example below.
#'
#' @author Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
#'
#' @details
#'
#' This function implements the method proposed by \citet{DenZha2019approxhv}
#' to approximate the hypervolume:
#'
#' \deqn{\widehat{HV}_r(A) = \frac{\pi^\frac{m}{2}}{2^m \Gamma(\frac{m}{2} + 1)}\frac{1}{n}\sum_{i=1}^n \max_{y \in A} s(w^{(i)}, y)^m}
#'
#' where \eqn{m} is the number of objectives, \eqn{n} is the number of weights
#' \eqn{w^{(i)}} sampled, \eqn{\Gamma} is the gamma function [gamma()], i.e.,
#' the analytical continuation of the factorial function, and \eqn{s(w, y) =
#' \min_{k=1}^m (r_k - y_k)/w_k}.
#'
#' In the default `method="DZ2019-HW"`, the weights \eqn{w^{(i)}, i=1\ldots n}
#' are defined using a deterministic low-discrepancy sequence. The weight
#' values depend on their number (`nsamples`), thus increasing the number of
#' weights may not necessarily increase accuracy because the set of weights
#' would be different. In method `method="DZ2019-MC"`, the weights
#' \eqn{w^{(i)}, i=1\ldots n} are sampled from the unit normal vector such that
#' each weight \eqn{w = \frac{|x|}{\|x\|_2}} where each component of \eqn{x} is
#' independently sampled from the standard normal distribution. The original
#' source code in C++/MATLAB for both methods can be found at
#' <https://github.com/Ksrma/Hypervolume-Approximation-using-polar-coordinate>.
#'
#' @inherit whv_hype params return
#'
#' @param method `character(1)`\cr Method to generate the sampling weights. See `Details'.
#'
#' @author Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
#'
#' @references
#'
#' \insertAllCited{}
#'
#' @examples
#'
#' data(SPEA2minstoptimeRichmond)
#' # The second objective must be maximized
#' # We calculate the hypervolume of the union of all sets.
#' hv_approx(SPEA2minstoptimeRichmond[, 1:2], reference = c(250, 0),
#'             maximise = c(FALSE, TRUE))
#'
#' @export
#' @concept metrics
hv_approx <- function(x, reference, maximise = FALSE, nsamples = 100000, seed = NULL,
                      method = c("DZ2019-HW","DZ2019-MC"))
{
  method <- match.arg(method)
  x <- as_double_matrix(x)
  nobjs <- ncol(x)

  if (is.null(reference)) stop("reference cannot be NULL")
  if (length(reference) == 1L) reference <- rep_len(reference, nobjs)

  if (method == "DZ2019-MC") {
    seed <- if (is.null(seed)) get_seed() else as_integer(seed)
    return(.Call(hv_approx_dz2019_mc_C,
      t(x),
      as.double(reference),
      as.logical(maximise),
      as.integer(nsamples),
      seed))
  } else if (method == "DZ2019-HW") {
    return(.Call(hv_approx_dz2019_hw_C,
      t(x),
      as.double(reference),
      as.logical(maximise),
      as.integer(nsamples)))
  } else
    stop("Unknown method: ", method)
}
