# Convert input to a matrix with `"double"` storage mode ([`base::storage.mode()`](https://rdrr.io/r/base/mode.html)).

Convert input to a matrix with `"double"` storage mode
([`base::storage.mode()`](https://rdrr.io/r/base/mode.html)).

## Usage

``` r
as_double_matrix(x)
```

## Arguments

- x:

  [`data.frame()`](https://rdrr.io/r/base/data.frame.html)\|[`matrix()`](https://rdrr.io/r/base/matrix.html)  
  A numerical data frame or matrix with at least 1 row and 2 columns.

## Value

`x` is coerced to a numerical
[`matrix()`](https://rdrr.io/r/base/matrix.html).
