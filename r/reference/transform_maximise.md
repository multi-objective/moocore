# Transform matrix according to maximise parameter

Transform matrix according to maximise parameter

## Usage

``` r
transform_maximise(x, maximise)
```

## Arguments

- x:

  [`matrix()`](https://rdrr.io/r/base/matrix.html)\|[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
  Matrix or data frame of numerical values, where each row gives the
  coordinates of a point.

- maximise:

  [`logical()`](https://rdrr.io/r/base/logical.html)  
  Whether the objectives must be maximised instead of minimised. Either
  a single logical value that applies to all objectives or a vector of
  logical values, with one value per objective.

## Value

`x` transformed such that every column where `maximise` is `TRUE` is
multiplied by `-1`.

## Examples

``` r
x <- data.frame(f1=1:10, f2=101:110)
rownames(x) <- letters[1:10]
transform_maximise(x, maximise=c(FALSE,TRUE))
#>   f1   f2
#> a  1 -101
#> b  2 -102
#> c  3 -103
#> d  4 -104
#> e  5 -105
#> f  6 -106
#> g  7 -107
#> h  8 -108
#> i  9 -109
#> j 10 -110
transform_maximise(x, maximise=TRUE)
#>    f1   f2
#> a  -1 -101
#> b  -2 -102
#> c  -3 -103
#> d  -4 -104
#> e  -5 -105
#> f  -6 -106
#> g  -7 -107
#> h  -8 -108
#> i  -9 -109
#> j -10 -110
x <- as.matrix(x)
transform_maximise(x, maximise=c(FALSE,TRUE))
#>   f1   f2
#> a  1 -101
#> b  2 -102
#> c  3 -103
#> d  4 -104
#> e  5 -105
#> f  6 -106
#> g  7 -107
#> h  8 -108
#> i  9 -109
#> j 10 -110
transform_maximise(x, maximise=TRUE)
#>    f1   f2
#> a  -1 -101
#> b  -2 -102
#> c  -3 -103
#> d  -4 -104
#> e  -5 -105
#> f  -6 -106
#> g  -7 -107
#> h  -8 -108
#> i  -9 -109
#> j -10 -110
```
