# Approximate the hypervolume indicator via a fully polynomial-time randomized approximation scheme (FPRAS).

This function implements the approximation algorithm by Bringmann and
Friedrich (2010) . This algorithm returns, with probability \\(1 -
\delta)\\, an \\\epsilon\\-approximation of the hypervolume metric with
respect to the given reference point, assuming minimization of all
objectives by default. Lower values of `epsilon` (\\\epsilon\\) or
`delta` (\\\delta\\) require significantly longer computation time.

## Usage

``` r
hv_approx_fpras(
  x,
  reference,
  maximise = FALSE,
  seed = NULL,
  epsilon = 0.01,
  delta = 0.1
)
```

## Arguments

- x:

  [`matrix()`](https://rdrr.io/r/base/matrix.html)\|[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
  Matrix or data frame of numerical values, where each row gives the
  coordinates of a point.

- reference:

  [`numeric()`](https://rdrr.io/r/base/numeric.html)  
  Reference point as a vector of numerical values.

- maximise:

  [`logical()`](https://rdrr.io/r/base/logical.html)  
  Whether the objectives must be maximised instead of minimised. Either
  a single logical value that applies to all objectives or a vector of
  logical values, with one value per objective.

- seed:

  `integer(1)`  
  Random seed.

- epsilon:

  `double(1)`  
  Desired relative error of the approximation, \\\epsilon \> 0\\.

- delta:

  `double(1)`  
  Desired failure probability \\0 \< \delta \< 1\\; \\(1 - \delta)\\
  gives the confidence level.

## Value

A single numerical value.

## Details

This function computes an approximation \\\hat{v}\\ of the true
hypervolume \\v = \text{hyp}\_r(A)\\ of the input points in \\A \subset
\mathbb{R}^m\\ with respect to the reference point \\r \in
\mathbb{R}^m\\, such that

\$\$\text{Pr}\[(1-\epsilon)v \leq \hat{v} \leq (1+\epsilon)v\] \geq (1 -
\delta)\$\$

where \\\epsilon \> 0\\ and \\0 \< \delta \< 1\\.

The algorithm requires \\O(\frac{nm}{\epsilon^2}\log\frac{1}{\delta})\\.
That is, it is linear on the number of points and dimensions, but
quadratic in the approximation error. In other words, more accurate
approximations require significantly more time.

In contrast to the (quasi)-Monte-Carlo methods provided by
[`hv_approx()`](https://multi-objective.github.io/moocore/r/reference/hv_approx.md),
the presence of weakly-dominated points not only increases the runtime,
but also changes the returned approximation for a fixed random seed.

The implementation uses Walker-Vose's alias method for sampling from a
discrete probability distribution (Vose 1991) , which requires \\O(1)\\
per sample. Using the naive roulette-wheel method would add, at least, a
factor of \\O(\log n)\\ to the above runtime.

## References

Karl Bringmann, Tobias Friedrich (2010). “Approximating the volume of
unions and intersections of high-dimensional geometric objects.”
*Computational Geometry*, **43**(6–7), 601–610.
[doi:10.1016/j.comgeo.2010.03.004](https://doi.org/10.1016/j.comgeo.2010.03.004)
.  
  
Michael D. Vose (1991). “A linear algorithm for generating random
numbers with a given distribution.” *IEEE Transactions on Software
Engineering*, **17**(9), 972–975.
[doi:10.1109/32.92917](https://doi.org/10.1109/32.92917) .

## See also

[`hypervolume()`](https://multi-objective.github.io/moocore/r/reference/hypervolume.md),
[`whv_hype()`](https://multi-objective.github.io/moocore/r/reference/whv_hype.md),
[`hv_approx()`](https://multi-objective.github.io/moocore/r/reference/hv_approx.md)

## Author

Manuel López-Ibáñez

## Examples

``` r
x <- matrix(c(5, 5, 4, 6, 2, 7, 7, 4), ncol=2, byrow=TRUE)
hypervolume(x, ref=10)
#> [1] 38
hv_approx(x, ref=10, method="Rphi-FWE+")
#> [1] 37.99998
hv_approx_fpras(x, ref=10, epsilon=0.1, delta=0.2, seed=42)
#> [1] 38.1446
hv_approx_fpras(x, ref=10, epsilon=0.01, delta=0.2, seed=42)
#> [1] 37.95418
```
