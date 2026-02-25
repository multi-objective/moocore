# Hypervolume contribution of a set of points

Computes the hypervolume contribution of each point of a set of points
with respect to a given reference point. Duplicated and dominated points
have zero contribution. By default, dominated points are ignored, that
is, they do not affect the contribution of other points. See the Notes
below for more details. For details about the hypervolume, see
[`hypervolume()`](https://multi-objective.github.io/moocore/r/reference/hypervolume.md).

## Usage

``` r
hv_contributions(x, reference, maximise = FALSE, ignore_dominated = TRUE)
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

- ignore_dominated:

  `logical(1)`  
  Whether dominated points are ignored when computing the contribution
  of nondominated points. The value of this parameter has an effect on
  the return values only if the input contains dominated points. Setting
  this to `FALSE` slows down the computation significantly. See the
  Notes below for a detailed explanation.

## Value

[`numeric()`](https://rdrr.io/r/base/numeric.html)  
A numerical vector

## Details

The hypervolume contribution of point \\\vec{p} \in X\\ is defined as
\\\text{hvc}(\vec{p}) = \text{hyp}(X) - \text{hyp}(X \setminus
\\\vec{p}\\)\\. This definition implies that duplicated points have zero
contribution even if not dominated, because removing one of the
duplicates does not change the hypervolume of the remaining set.
Moreover, dominated points also have zero contribution. However, a point
that is dominated by a single (dominating) nondominated point reduces
the contribution of the latter, because removing the dominating point
makes the dominated one become nondominated.

Handling this special case is non-trivial and makes the computation more
expensive, thus the default (`ignore_dominated=TRUE`) ignores all
dominated points in the input, that is, their contribution is set to
zero and their presence does not affect the contribution of any other
point. Setting `ignore_dominated=FALSE` will consider dominated points
according to the mathematical definition given above, but the
computation will be slower.

When the input only consists of mutually nondominated points, the value
of `ignore_dominated` does not change the result, but the default value
is significantly faster.

The current implementation uses a \\O(n\log n)\\ dimension-sweep
algorithm for 2D. With `ignore_dominated=TRUE`, the 3D case uses the
HVC3D algorithm (Guerreiro and Fonseca 2018) , which has \\O(n\log n)\\
complexity. Otherwise, the implementation uses the naive algorithm that
requires calculating the hypervolume \\\|X\|+1\\ times.

## References

Andreia P. Guerreiro, Carlos M. Fonseca (2018). “Computing and Updating
Hypervolume Contributions in Up to Four Dimensions.” *IEEE Transactions
on Evolutionary Computation*, **22**(3), 449–463.
[doi:10.1109/tevc.2017.2729550](https://doi.org/10.1109/tevc.2017.2729550)
.

## See also

[`hypervolume()`](https://multi-objective.github.io/moocore/r/reference/hypervolume.md)

## Author

Manuel López-Ibáñez

## Examples

``` r
x <- matrix(c(5,1, 1,5, 4,2, 4,4, 5,1), ncol=2, byrow=TRUE)
hv_contributions(x, reference=c(6,6))
#> [1] 0 3 3 0 0
# hvc[(5,1)] = 0 = duplicated
# hvc[(1,5)] = 3 = (4 - 1) * (6 - 5)
# hvc[(4,2)] = 3 = (5 - 4) * (5 - 2)
# hvc[(4,4)] = 0 = dominated
# hvc[(5,1)] = 0 = duplicated
hv_contributions(x, reference=c(6,6), ignore_dominated = FALSE)
#> [1] 0 3 2 0 0
# hvc[(5,1)] = 0 = duplicated
# hvc[(1,5)] = 3 = (4 - 1) * (6 - 5)
# hvc[(4,2)] = 2 = (5 - 4) * (4 - 2)
# hvc[(4,4)] = 0 = dominated
# hvc[(5,1)] = 0 = duplicated
data(SPEA2minstoptimeRichmond)
# The second objective must be maximized
# We calculate the hypervolume contribution of each point of the union of all sets.
hv_contributions(SPEA2minstoptimeRichmond[, 1:2], reference = c(250, 0),
            maximise = c(FALSE, TRUE))
#>   [1]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#>   [8]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#>  [15]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#>  [22]     0.000     8.197     0.000     0.000     0.000     0.000     0.000
#>  [29]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#>  [36]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#>  [43]     0.000     0.000     0.000     0.000     0.000     0.000  7959.940
#>  [50]  1945.800  8147.132     0.000     0.000     0.000     0.000     0.000
#>  [57]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#>  [64]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#>  [71]    26.255     0.000     0.000     0.000     0.000     0.000     0.000
#>  [78]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#>  [85]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#>  [92]     0.000  3698.640     0.000     0.000     5.971     0.000     0.000
#>  [99]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#> [106]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#> [113]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#> [120]     0.000  3069.000   779.240     0.000     0.000     0.000     0.000
#> [127]     0.000     0.000     0.000     0.000     0.000 41994.755     0.000
#> [134]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#> [141]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#> [148]     0.000     0.000     0.000     0.000     0.000     0.000     0.000
#> [155]     0.000     0.000     0.000     0.000     0.000  2294.064     0.000
#> [162]     0.000     0.000     0.000     0.000     0.000

# Duplicated points show zero contribution above, even if not
# dominated. However, filter_dominated removes all duplicates except
# one. Hence, there are more points below with nonzero contribution.
hv_contributions(filter_dominated(SPEA2minstoptimeRichmond[, 1:2], maximise = c(FALSE, TRUE)),
                 reference = c(250, 0), maximise = c(FALSE, TRUE))
#>  [1]  89283.920 255278.978      8.197   2242.660   7959.940   1945.800
#>  [7]   8147.132     73.054     26.255   3698.640      5.971 193143.324
#> [13]   3069.000    779.240  41994.755   2294.064
```
