#' Epsilon metric
#'
#' Computes the epsilon metric, either additive or multiplicative.
#'
#' @inheritParams is_nondominated
#'
#' @param reference `matrix`|`data.frame`\cr Reference set as a matrix or
#'   data.frame of numerical values.
#'
#' @return  `numeric(1)` A single numerical value.
#'
#' @name epsilon
#'
#' @author Manuel \enc{López-Ibáñez}{Lopez-Ibanez}
#'
#' @details
#'
#' The epsilon metric of a set \eqn{A} with respect to a reference set \eqn{R}
#' is defined as
#'
#' \deqn{epsilon(A,R) = \max_{r \in R} \min_{a \in A} \max_{1 \leq i \leq n} epsilon(a_i, r_i)}
#'
#' where \eqn{a} and \eqn{b} are objective vectors of length \eqn{n}.
#'
#' In the case of minimization of objective \eqn{i}, \eqn{epsilon(a_i,b_i)} is
#' computed as \eqn{a_i/b_i} for the multiplicative variant (respectively,
#' \eqn{a_i - b_i} for the additive variant), whereas in the case of
#' maximization of objective \eqn{i}, \eqn{epsilon(a_i,b_i) = b_i/a_i} for the
#' multiplicative variant (respectively, \eqn{b_i - a_i} for the additive
#' variant). This allows computing a single value for problems where some
#' objectives are to be maximized while others are to be minimized. Moreover, a
#' lower value corresponds to a better approximation set, independently of the
#' type of problem (minimization, maximization or mixed). However, the meaning
#' of the value is different for each objective type. For example, imagine that
#' objective 1 is to be minimized and objective 2 is to be maximized, and the
#' multiplicative epsilon computed here for \eqn{epsilon(A,R) = 3}. This means
#' that \eqn{A} needs to be multiplied by 1/3 for all \eqn{a_1} values and by 3
#' for all \eqn{a_2} values in order to weakly dominate \eqn{R}.
#'
#' The multiplicative variant can be computed as \eqn{\exp(epsilon_{+}(\log(A),
#' \log(R)))}, which makes clear that the computation of the multiplicative
#' version for zero or negative values doesn't make sense. See the examples
#' below.
#'
#' The current implementation uses the naive algorithm that requires
#' \eqn{O(n \cdot |A| \cdot |R|)}, where \eqn{n} is the number of objectives
#' (dimension of vectors).
#'
#' @references
#'
#' \insertRef{ZitThiLauFon2003:tec}{moocore}
#'
NULL
#> NULL

#' @rdname epsilon
#' @concept metrics
#' @doctest
#' # Fig 6 from Zitzler et al. (2003).
#' A1 <- matrix(c(9,2,8,4,7,5,5,6,4,7), ncol=2, byrow=TRUE)
#' A2 <- matrix(c(8,4,7,5,5,6,4,7), ncol=2, byrow=TRUE)
#' A3 <- matrix(c(10,4,9,5,8,6,7,7,6,8), ncol=2, byrow=TRUE)
#' @omit
#' if (requireNamespace("graphics", quietly = TRUE)) {
#'    plot(A1, xlab=expression(f[1]), ylab=expression(f[2]),
#'         panel.first=grid(nx=NULL), pch=4, cex=1.5, xlim = c(0,10), ylim=c(0,8))
#'    points(A2, pch=0, cex=1.5)
#'    points(A3, pch=1, cex=1.5)
#'    legend("bottomleft", legend=c("A1", "A2", "A3"), pch=c(4,0,1),
#'           pt.bg="gray", bg="white", bty = "n", pt.cex=1.5, cex=1.2)
#' }
#' @expect equal(0.9)
#' epsilon_mult(A1, A3) # A1 epsilon-dominates A3 => e = 9/10 < 1
#' @expect equal(1)
#' epsilon_mult(A1, A2) # A1 weakly dominates A2 => e = 1
#' @expect equal(2)
#' epsilon_mult(A2, A1) # A2 is epsilon-dominated by A1 => e = 2 > 1
#' # Equivalence between additive and multiplicative
#' @expect equal(2)
#' exp(epsilon_additive(log(A2), log(A1)))
#'
#' # A more realistic example
#' extdata_path <- system.file(package="moocore","extdata")
#' path.A1 <- file.path(extdata_path, "ALG_1_dat.xz")
#' path.A2 <- file.path(extdata_path, "ALG_2_dat.xz")
#' A1 <- read_datasets(path.A1)[,1:2]
#' A2 <- read_datasets(path.A2)[,1:2]
#' ref <- filter_dominated(rbind(A1, A2))
#' @expect equal(199090640)
#' epsilon_additive(A1, ref)
#' @expect equal(132492066)
#' epsilon_additive(A2, ref)
#' # Multiplicative version of epsilon metric
#' ref <- filter_dominated(rbind(A1, A2))
#' @expect equal(1.05401476)
#' epsilon_mult(A1, ref)
#' @expect equal(1.023755)
#' epsilon_mult(A2, ref)
#' @export
epsilon_additive <- function(x, reference, maximise = FALSE)
  unary_common(data = x, reference = reference, maximise = maximise,
    fn = function(DATA, REFERENCE, MAXIMISE) .Call(epsilon_add_C, DATA, REFERENCE, MAXIMISE))

#' @rdname epsilon
#' @concept metrics
#' @export
epsilon_mult <- function(x, reference, maximise = FALSE)
  unary_common(data = x, reference = reference, maximise = maximise,
    fn = function(DATA, REFERENCE, MAXIMISE) .Call(epsilon_mul_C, DATA, REFERENCE, MAXIMISE))

unary_common <- function(data, reference, maximise, fn)
{
  data <- as_double_matrix(data)
  nobjs <- ncol(data)
  if (is.null(reference)) stop("reference cannot be NULL")
  reference <- as_double_matrix(reference)
  if (ncol(reference) != nobjs)
    stop("data and reference must have the same number of columns")
  fn(t(data), t(reference), as.logical(rep_len(maximise, nobjs)))
}
