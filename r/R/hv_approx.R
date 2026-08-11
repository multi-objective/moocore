#' Approximate the hypervolume indicator.
#'
#' Approximate the value of the hypervolume metric with respect to a given
#' reference point assuming minimization of all objectives. Methods
#' `"Rphi-FWE+"` and `"DZ2019-HW"` are deterministic and ignore the parameter
#' `seed`, while `method="DZ2019-MC"` relies on Monte-Carlo sampling
#' \citep{DenZha2019approxhv}.  All methods tend to get more accurate with
#' higher values of `nsamples`, but the increase in accuracy is not monotonic,
#' as shown in the vignette \HTMLVignette{hv_approx}{Approximating the
#' hypervolume}.
#'
#' @author Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
#'
#' @details
#'
#' All available methods approximate the hypervolume as a
#' \eqn{(m-1)}-dimensional integral over the surface of hypersphere
#' \citep{DenZha2019approxhv}:
#'
#' \deqn{\widehat{HV}_r(A) = \frac{2\pi^\frac{m}{2}}{\Gamma(\frac{m}{2})}\frac{1}{m 2^m}\frac{1}{n}\sum_{i=1}^n \max_{y \in A} s(w^{(i)}, y)^m}
#'
#' where \eqn{m} is the number of objectives, \eqn{w^{(i)}} are weights
#' uniformly distributed on \eqn{S_{+}}, i.e., the positive orthant of the
#' \eqn{(m-1)}-D unit hypersphere, \eqn{n} is the number of weights sampled,
#' \eqn{\Gamma()} is the gamma function [gamma()], i.e., the analytical
#' continuation of the factorial function, and \eqn{s(w, y) = \min_{k=1}^m (r_k
#' - y_k)/w_k}.
#'
#' In the default `method="Rphi-FWE+"` \citep{Lop2026hvapprox}, the weights
#' \eqn{w^{(i)}, i=1\ldots n} are defined using the deterministic
#' low-discrepancy sequence \eqn{R_\phi} \citep{Rob2018unreasonable} mapped to
#' the positive orthant of the hypersphere using a modified version of Fang and
#' Wang efficient mapping \citep{FanWan1994numtheory}.
#'
#' In `method="DZ2019-HW"` \citep{DenZha2019approxhv}, the weights
#' \eqn{w^{(i)}, i=1\ldots n} are defined using a deterministic low-discrepancy
#' sequence. The weight values depend on their number (`nsamples`), thus
#' increasing the number of weights may not necessarily increase accuracy
#' because the set of weights would be different.
#'
#' In `method="DZ2019-MC"` \citep{DenZha2019approxhv}, the weights
#' \eqn{w^{(i)}, i=1\ldots n} are sampled from the unit normal vector such that
#' each weight \eqn{w = \frac{|x|}{\|x\|_2}} where each component of \eqn{x} is
#' independently sampled from the standard normal distribution
#' \citep{Muller1959sphere}.
#'
#' The original source code in C++/MATLAB for both `"DZ2019-HW"` and
#' `"DZ2019-MC"` methods can be found at
#' <https://github.com/Ksrma/Hypervolume-Approximation-using-polar-coordinate>.
#'
#' \citet{Lop2026hvapprox} empirically shows that `"Rphi-FWE+"` typically
#' produces an approximation error as low as the other methods, with a
#' computational cost similar to `"DZ2019-MC"` and significantly faster than
#' `"DZ2019-HW"`.
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
#' @expect equal(37.999979)
#' hv_approx(x, ref=10, method="Rphi-FWE+")
#' @expect equal(37.999958)
#' hv_approx(x, ref=10, method="DZ2019-HW")
#' @expect equal(38.000806)
#' hv_approx(x, ref=10, seed=42, method="DZ2019-MC")
#'
#' @export
#' @concept metrics
hv_approx <- function(x, reference, maximise = FALSE, nsamples = 262144L, seed = NULL,
                      method = c("Rphi-FWE+", "DZ2019-HW", "DZ2019-MC"))
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
  check_dimension_max(nobjs, .libmoocore_constants[["MOOCORE_HVAPPROX_DIMENSION_MAX"]])

  if (!is_wholenumber(nsamples) || nsamples <= 0 || nsamples > 2147483648)
    stop("nsamples (", nsamples, ") must be a positive integer value smaller than 2147483648")

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
  } else  {
    # match.arg() should already give an error if method is wrong.
    stopifnot(method == "Rphi-FWE+")
    return(.Call(hv_approx_rphi_fang_wang_plus_C,
      t(x),
      as.double(reference),
      as.logical(maximise),
      as.integer(nsamples)))
  }
}


#' Approximate the hypervolume indicator via a fully polynomial-time randomized approximation scheme (FPRAS).
#'
#' This function implements the approximation algorithm by
#' \citet{BriFri2010approx}.  This algorithm returns, with probability
#' \eqn{(1 - \delta)}, an \eqn{\epsilon}-approximation of the hypervolume
#' metric with respect to the given reference point, assuming minimization of
#' all objectives by default.
#'
#' @details
#'
#' This function computes an approximation \eqn{\hat{v}} of the true
#' hypervolume \eqn{v = \text{hyp}_r(A)} of the input points in \eqn{A
#' \subset \mathbb{R}^m} with respect to the reference point \eqn{r \in
#' \mathbb{R}^m}, such that
#'
#' \deqn{\text{Pr}[(1-\epsilon)v \leq \hat{v} \leq (1+\epsilon)v] \geq (1 - \delta)}
#'
#' where \eqn{\epsilon > 0} and \eqn{0 < \delta < 1}.
#'
#' The algorithm requires \eqn{O(\frac{nm}{\epsilon^2}\log\frac{1}{\delta})}.
#' That is, it is linear on the number of points and dimensions, but quadratic
#' in the approximation error.  In other words, more accurate approximations
#' require significantly more time.
#'
#' In contrast to the (quasi)-Monte-Carlo methods provided by [hv_approx()],
#' the presence of weakly-dominated points not only increases the runtime, but
#' also changes the returned approximation for a fixed random seed.
#'
#' The implementation uses Walker-Vose's alias method for sampling from a
#' discrete probability distribution \citep{Vose1991alias}, which requires
#' \eqn{O(1)} per sample.  Using the naive roulette-wheel method would add, at
#' least, a factor of \eqn{O(\log n)} to the above runtime.
#'
#' @inheritParams hv_approx
#'
#' @return A single numerical value.
#'
#' @param epsilon `double(1)`\cr Desired relative error of the approximation,
#'   \eqn{\epsilon > 0}.
#'
#' @param delta `double(1)`\cr Desired failure probability
#'   \eqn{0 < \delta < 1}; \eqn{(1 - \delta)} gives the confidence level.
#'
#' @warning Lower values of `epsilon` (\eqn{\epsilon}) or `delta`
#'   (\eqn{\delta}) require significantly longer computation time.
#'
#' @seealso [hypervolume()], [whv_hype()], [hv_approx()]
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
#' @expect equal(37.999979)
#' hv_approx(x, ref=10, method="Rphi-FWE+")
#' @expect equal(38.1446, tolerance=1e-4)
#' hv_approx_fpras(x, ref=10, epsilon=0.1, delta=0.2, seed=42)
#' @expect equal(37.9541, tolerance=1e-4)
#' hv_approx_fpras(x, ref=10, epsilon=0.01, delta=0.2, seed=42)
#'
#' @export
#' @concept metrics
hv_approx_fpras <- function(x, reference, maximise = FALSE, seed = NULL,
                            epsilon = 0.01, delta = 0.1)
{
  x <- as_double_matrix(x)
  nobjs <- ncol(x)

  if (!is.numeric(reference))
    stop("a numerical reference vector must be provided")
  if (length(reference) == 1L) reference <- rep_len(reference, nobjs)
  stopifnot(length(reference) == nobjs)

  if (length(maximise) == 1L) maximise <- rep_len(maximise, nobjs)
  stopifnot(length(maximise) == nobjs)
  check_dimension_max(nobjs, .libmoocore_constants[["MOOCORE_HVAPPROX_DIMENSION_MAX"]])

  if (!is.numeric(epsilon) || length(epsilon) != 1L || epsilon <= 0)
    stop("epsilon must be a positive numeric value")
  if (!is.numeric(delta) || length(delta) != 1L || delta <= 0 || delta >= 1)
    stop("delta must be strictly within (0, 1)")

  seed <- if (is.null(seed)) get_seed() else as_integer(seed)
  hv <- .Call(hv_approx_fpras_C,
    t(x),
    as.double(reference),
    as.logical(maximise),
    seed,
    as.double(epsilon),
    as.double(delta))
  if (hv < 0)
    stop("The requested approximation (epsilon=", epsilon, ", delta=", delta,
         ") would require a very long time")
  hv
}
