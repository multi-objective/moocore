# Identify largest EAF differences

Given a list of datasets, return the indexes of the pair with the
largest EAF differences according to the method proposed by Diaz and
López-Ibáñez (2021) .

## Usage

``` r
largest_eafdiff(x, maximise = FALSE, intervals = 5L, reference, ideal = NULL)
```

## Arguments

- x:

  [`list()`](https://rdrr.io/r/base/list.html)  
  A list of matrices or data frames with at least 3 columns (last column
  indicates the set).

- maximise:

  [`logical()`](https://rdrr.io/r/base/logical.html)  
  Whether the objectives must be maximised instead of minimised. Either
  a single logical value that applies to all objectives or a vector of
  logical values, with one value per objective.

- intervals:

  `integer(1)`  
  The absolute range of the differences \\\[0, 1\]\\ is partitioned into
  the number of intervals provided.

- reference:

  [`numeric()`](https://rdrr.io/r/base/numeric.html)  
  Reference point as a vector of numerical values.

- ideal:

  [`numeric()`](https://rdrr.io/r/base/numeric.html)  
  Ideal point as a vector of numerical values. If `NULL`, it is
  calculated as minimum (or maximum if maximising that objective) of
  each objective in the input data.

## Value

[`list()`](https://rdrr.io/r/base/list.html)  
A list with two components `pair` and `value`.

## References

Juan Esteban Diaz, Manuel López-Ibáñez (2021). “Incorporating
Decision-Maker's Preferences into the Automatic Configuration of
Bi-Objective Optimisation Algorithms.” *European Journal of Operational
Research*, **289**(3), 1209–1222.
[doi:10.1016/j.ejor.2020.07.059](https://doi.org/10.1016/j.ejor.2020.07.059)
.

## Examples

``` r
# FIXME: This example is too large, we need a smaller one.
data(tpls50x20_1_MWT)
nadir <- apply(tpls50x20_1_MWT[,2:3], 2L, max)
x <- largest_eafdiff(split.data.frame(tpls50x20_1_MWT[,2:4], tpls50x20_1_MWT[, 1L]),
                     reference = nadir)
str(x)
#> List of 2
#>  $ pair : int [1:2] 3 6
#>  $ value: num 777017
```
