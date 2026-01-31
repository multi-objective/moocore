#' Approximate the hypervolume indicator.
#'
#' Approximate the value of the hypervolume metric with respect to a given
#' reference point assuming minimization of all objectives. The default
#' `method="DZ2019-HW"` is deterministic and ignores the parameter `seed`,
#' while `method="DZ2019-MC"` relies on Monte-Carlo sampling
#' \citep{DenZha2019approxhv}.  Both methods tend to get more accurate with
#' higher values of `nsamples`, but the increase in accuracy is not monotonic,
#' as shown in the vignette \HTMLVignette{hv_approx}{Approximating the hypervolume}.
#'
#' @author Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
#'
#' @details
#'
#' This function implements the method proposed by \citet{DenZha2019approxhv}
#' to approximate the hypervolume:
#'
#' \deqn{\widehat{HV}_r(A) = \frac{2\pi^\frac{m}{2}}{\Gamma(\frac{m}{2})}\frac{1}{m 2^m}\frac{1}{n}\sum_{i=1}^n \max_{y \in A} s(w^{(i)}, y)^m}
#'
#' where \eqn{m} is the number of objectives, \eqn{n} is the number of weights
#' \eqn{w^{(i)}} sampled, \eqn{\Gamma()} is the gamma function [gamma()], i.e.,
#' the analytical continuation of the factorial function, and \eqn{s(w, y) =
#' \min_{k=1}^m (r_k - y_k)/w_k}.
#'
#' In the default `method="DZ2019-HW"`, the weights \eqn{w^{(i)}, i=1\ldots n}
#' are defined using a deterministic low-discrepancy sequence. The weight
#' values depend on their number (`nsamples`), thus increasing the number of
#' weights may not necessarily increase accuracy because the set of weights
#' would be different. In `method="DZ2019-MC"`, the weights
#' \eqn{w^{(i)}, i=1\ldots n} are sampled from the unit normal vector such that
#' each weight \eqn{w = \frac{|x|}{\|x\|_2}} where each component of \eqn{x} is
#' independently sampled from the standard normal distribution.  The original
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
#' @doctest
#'
#' x <- matrix(c(5, 5, 4, 6, 2, 7, 7, 4), ncol=2, byrow=TRUE)
#' @expect equal(38.0)
#' hypervolume(x, ref=10)
#' @expect equal(38.014754)
#' hv_approx(x, ref=10, seed=42, method="DZ2019-MC")
#' @expect equal(37.99989)
#' hv_approx(x, ref=10, method="DZ2019-HW")
#'
#' @export
#' @concept metrics
hv_approx <- function(x, reference, maximise = FALSE, nsamples = 100000L, seed = NULL,
                      method = c("DZ2019-HW", "DZ2019-MC"))
{
  method <- match.arg(method)
  x <- as_double_matrix(x)
  nobjs <- ncol(x)

  if (!is.numeric(reference))
    stop("a numerical reference vector must be provided")
  if (length(reference) == 1L) reference <- rep_len(reference, nobjs)
  stopifnot(length(reference) == nobjs)

  if (length(maximise) == 1L) maximise <- rep_len(maximise, nobjs)
  stopifnot(length(maximise) == nobjs)

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
