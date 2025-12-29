# Convert a list of attainment surfaces to a single EAF `data.frame`.

Convert a list of attainment surfaces to a single EAF `data.frame`.

## Usage

``` r
attsurf2df(x)
```

## Arguments

- x:

  [`list()`](https://rdrr.io/r/base/list.html)  
  List of `data.frames` or matrices. The names of the list give the
  percentiles of the attainment surfaces. This is the format returned by
  [`eaf_as_list()`](https://multi-objective.github.io/moocore/r/reference/eaf_as_list.md).

## Value

[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
Data frame with as many columns as objectives and an additional column
`percentiles`.

## See also

[`eaf_as_list()`](https://multi-objective.github.io/moocore/r/reference/eaf_as_list.md)

## Examples

``` r
data(SPEA2relativeRichmond)
attsurfs <- eaf_as_list(eaf(SPEA2relativeRichmond, percentiles = c(0,50,100)))
str(attsurfs)
#> List of 3
#>  $ 0  : num [1:7, 1:2] 90 90.3 91.9 93.1 98 ...
#>  $ 50 : num [1:7, 1:2] 95.1 95.2 97.2 98.2 103.2 ...
#>  $ 100: num [1:5, 1:2] 100 101 102 104 112 ...
eaf_df <- attsurf2df(attsurfs)
str(eaf_df)
#>  num [1:19, 1:3] 90 90.3 91.9 93.1 98 ...
#>  - attr(*, "dimnames")=List of 2
#>   ..$ : NULL
#>   ..$ : chr [1:3] "" "" "percentiles"
```
