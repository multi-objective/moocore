# Same as [`eaf()`](https://multi-objective.github.io/moocore/r/reference/eaf.md) but performs no checks and does not transform the input or the output. This function should be used by other packages that want to avoid redundant checks and transformations.

Same as
[`eaf()`](https://multi-objective.github.io/moocore/r/reference/eaf.md)
but performs no checks and does not transform the input or the output.
This function should be used by other packages that want to avoid
redundant checks and transformations.

## Usage

``` r
compute_eaf_call(x, cumsizes, percentiles)
```

## Arguments

- x:

  [`matrix()`](https://rdrr.io/r/base/matrix.html)\|[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
  Matrix or data frame of numerical values that represents multiple sets
  of points, where each row represents a point. If `sets` is missing,
  the last column of `x` gives the sets.

- cumsizes:

  [`integer()`](https://rdrr.io/r/base/integer.html)  
  Cumulative size of the different sets of points in `x`.

- percentiles:

  [`numeric()`](https://rdrr.io/r/base/numeric.html)  
  Vector indicating which percentiles are computed. `NULL` computes all.

## Value

[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
A data frame containing the exact representation of EAF. The last column
gives the percentile that corresponds to each point. If groups is not
`NULL`, then an additional column indicates to which group the point
belongs.

## See also

[`as_double_matrix()`](https://multi-objective.github.io/moocore/r/reference/as_double_matrix.md)
[`transform_maximise()`](https://multi-objective.github.io/moocore/r/reference/transform_maximise.md)
