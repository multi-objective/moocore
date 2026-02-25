# Combine datasets `x` and `y` by row taking care of making all sets unique.

Combine datasets `x` and `y` by row taking care of making all sets
unique.

## Usage

``` r
rbind_datasets(x, y)
```

## Arguments

- x, y:

  `matrix`\|[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
  Each dataset has at least three columns, the last one is the set of
  each point. See also
  [`read_datasets()`](https://multi-objective.github.io/moocore/r/reference/read_datasets.md).

## Value

`matrix()|`data.frame()\`  
A dataset.

## Examples

``` r
x <- data.frame(f1 = 5:10, f2 = 10:5, set = 1:6)
y <- data.frame(f1 = 15:20, f2 = 20:15, set = 1:6)
rbind_datasets(x,y)
#>    f1 f2 set
#> 1   5 10   1
#> 2   6  9   2
#> 3   7  8   3
#> 4   8  7   4
#> 5   9  6   5
#> 6  10  5   6
#> 7  15 20   7
#> 8  16 19   8
#> 9  17 18   9
#> 10 18 17  10
#> 11 19 16  11
#> 12 20 15  12
```
