# Approximate the hypervolume indicator.

Approximate the value of the hypervolume metric with respect to a given
reference point assuming minimization of all objectives. Methods
`"Rphi-FWE+"` and `"DZ2019-HW"` are deterministic and ignore the
parameter `seed`, while `method="DZ2019-MC"` relies on Monte-Carlo
sampling (Deng and Zhang 2019) . All methods tend to get more accurate
with higher values of `nsamples`, but the increase in accuracy is not
monotonic, as shown in the vignette [Approximating the
hypervolume](https://multi-objective.github.io/moocore/r/articles/hv_approx.html).

## Usage

``` r
hv_approx(
  x,
  reference,
  maximise = FALSE,
  nsamples = 262144L,
  seed = NULL,
  method = c("Rphi-FWE+", "DZ2019-HW", "DZ2019-MC")
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

- nsamples:

  `integer(1)`  
  Number of samples for Monte-Carlo sampling. Higher values typically
  produce more accurate approximations of the true hypervolume, but
  require more time.

- seed:

  `integer(1)`  
  Random seed.

- method:

  `character(1)`  
  Method to generate the sampling weights. See \`Details'.

## Value

A single numerical value.

## Details

All available methods approximate the hypervolume as a
\\(m-1)\\-dimensional integral over the surface of hypersphere (Deng and
Zhang 2019) :

\$\$\widehat{HV}\_r(A) =
\frac{2\pi^\frac{m}{2}}{\Gamma(\frac{m}{2})}\frac{1}{m
2^m}\frac{1}{n}\sum\_{i=1}^n \max\_{y \in A} s(w^{(i)}, y)^m\$\$

where \\m\\ is the number of objectives, \\w^{(i)}\\ are weights
uniformly distributed on \\S\_{+}\\, i.e., the positive orthant of the
\\(m-1)\\-D unit hypersphere, \\n\\ is the number of weights sampled,
\\\Gamma()\\ is the gamma function
[`gamma()`](https://rdrr.io/r/base/Special.html), i.e., the analytical
continuation of the factorial function, and \\s(w, y) = \min\_{k=1}^m
(r_k - y_k)/w_k\\.

In the default `method="Rphi-FWE+"` (López-Ibáñez 2026) , the weights
\\w^{(i)}, i=1\ldots n\\ are defined using the deterministic
low-discrepancy sequence \\R\_\phi\\ (Roberts 2018) mapped to the
positive orthant of the hypersphere using a modified version of Fang and
Wang efficient mapping (Fang and Wang 1994) .

In `method="DZ2019-HW"` (Deng and Zhang 2019) , the weights \\w^{(i)},
i=1\ldots n\\ are defined using a deterministic low-discrepancy
sequence. The weight values depend on their number (`nsamples`), thus
increasing the number of weights may not necessarily increase accuracy
because the set of weights would be different.

In `method="DZ2019-MC"` (Deng and Zhang 2019) , the weights \\w^{(i)},
i=1\ldots n\\ are sampled from the unit normal vector such that each
weight \\w = \frac{\|x\|}{\\x\\\_2}\\ where each component of \\x\\ is
independently sampled from the standard normal distribution (Muller
1959) .

The original source code in C++/MATLAB for both `"DZ2019-HW"` and
`"DZ2019-MC"` methods can be found at
<https://github.com/Ksrma/Hypervolume-Approximation-using-polar-coordinate>.

López-Ibáñez (2026) empirically shows that `"Rphi-FWE+"` typically
produces an approximation error as low as the other methods, with a
computational cost similar to `"DZ2019-MC"` and significantly faster
than `"DZ2019-HW"`.

## References

Jingda Deng, Qingfu Zhang (2019). “Approximating Hypervolume and
Hypervolume Contributions Using Polar Coordinate.” *IEEE Transactions on
Evolutionary Computation*, **23**(5), 913–918.
[doi:10.1109/tevc.2019.2895108](https://doi.org/10.1109/tevc.2019.2895108)
.  
  
K T Fang, Y Wang (1994). *Number-Theoretic Methods in Statistics*.
Chapman & Hall/CRC, London, UK.  
  
Manuel López-Ibáñez (2026). “Approximating the Hypervolume Indicator
using Fast Low-Discrepancy Sequences.” In Ting Hu, Leonardo Trujillo
(eds.), *Proceedings of the Genetic and Evolutionary Computation
Conference, GECCO 2026*. ACM Press, New York, NY.
[doi:10.1145/3795095.3805198](https://doi.org/10.1145/3795095.3805198)
.  
  
Mervin E. Muller (1959). “A Note on a Method for Generating Points
Uniformly on N-Dimensional Spheres.” *Communications of the ACM*,
**2**(4), 19–20.
[doi:10.1145/377939.377946](https://doi.org/10.1145/377939.377946) .  
  
Martin Roberts (2018). “The Unreasonable Effectiveness of Quasirandom
Sequences.”
<https://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/>.
Last visited: 13/04/2025.

## Author

Manuel López-Ibáñez

## Examples

``` r
x <- matrix(c(5, 5, 4, 6, 2, 7, 7, 4), ncol=2, byrow=TRUE)
hypervolume(x, ref=10)
#> [1] 38
hv_approx(x, ref=10, method="Rphi-FWE+")
#> [1] 37.99998
hv_approx(x, ref=10, method="DZ2019-HW")
#> [1] 37.99996
hv_approx(x, ref=10, seed=42, method="DZ2019-MC")
#> [1] 38.00081
```
