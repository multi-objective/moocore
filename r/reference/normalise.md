# Normalise points

Normalise points per coordinate to a range, e.g., `c(1,2)`, where the
minimum value will correspond to 1 and the maximum to 2. If bounds are
given, they are used for the normalisation.

## Usage

``` r
normalise(x, to_range = c(1, 2), lower = NA, upper = NA, maximise = FALSE)
```

## Arguments

- x:

  [`matrix()`](https://rdrr.io/r/base/matrix.html)\|[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
  Matrix or data frame of numerical values, where each row gives the
  coordinates of a point.

- to_range:

  `numerical(2)`  
  Normalise values to this range. If the objective is maximised, it is
  normalised to `c(to_range[1], to_range[0])` instead.

- lower, upper:

  `numerical()`  
  Bounds on the values. If `NA`, the maximum and minimum values of each
  coordinate are used.

- maximise:

  [`logical()`](https://rdrr.io/r/base/logical.html)  
  Whether the objectives must be maximised instead of minimised. Either
  a single logical value that applies to all objectives or a vector of
  logical values, with one value per objective.

## Value

[`matrix()`](https://rdrr.io/r/base/matrix.html)  
A numerical matrix

## Author

Manuel López-Ibáñez

## Examples

``` r
data(SPEA2minstoptimeRichmond)
# The second objective must be maximized
head(SPEA2minstoptimeRichmond[, 1:2])
#>        V1    V2
#> 1 105.832   357
#> 2 108.187   911
#> 3 108.519  3960
#> 4 108.641  7560
#> 5 108.896 16532
#> 6 112.556 19957

head(normalise(SPEA2minstoptimeRichmond[, 1:2], maximise = c(FALSE, TRUE)))
#>            V1       V2
#> [1,] 1.044174 2.000000
#> [2,] 1.059891 1.993561
#> [3,] 1.062107 1.958126
#> [4,] 1.062921 1.916286
#> [5,] 1.064623 1.812013
#> [6,] 1.089049 1.772207

head(normalise(SPEA2minstoptimeRichmond[, 1:2], to_range = c(0,1), maximise = c(FALSE, TRUE)))
#>              V1        V2
#> [1,] 0.04417408 1.0000000
#> [2,] 0.05989095 0.9935614
#> [3,] 0.06210666 0.9581256
#> [4,] 0.06292087 0.9162860
#> [5,] 0.06462270 0.8120126
#> [6,] 0.08904891 0.7722069
```
