# Convert an EAF data frame to a list of data frames, where each element of the list is one attainment surface. The function [`attsurf2df()`](https://multi-objective.github.io/moocore/r/reference/attsurf2df.md) can be used to convert the list into a single data frame.

Convert an EAF data frame to a list of data frames, where each element
of the list is one attainment surface. The function
[`attsurf2df()`](https://multi-objective.github.io/moocore/r/reference/attsurf2df.md)
can be used to convert the list into a single data frame.

## Usage

``` r
eaf_as_list(eaf)
```

## Arguments

- eaf:

  [`data.frame()`](https://rdrr.io/r/base/data.frame.html)\|[`matrix()`](https://rdrr.io/r/base/matrix.html)  
  Data frame or matrix that represents the EAF.

## Value

[`list()`](https://rdrr.io/r/base/list.html)  
A list of data frames. Each `data.frame` represents one attainment
surface.

## See also

[`eaf()`](https://multi-objective.github.io/moocore/r/reference/eaf.md)
[`attsurf2df()`](https://multi-objective.github.io/moocore/r/reference/attsurf2df.md)

## Examples

``` r
extdata_path <- system.file(package="moocore", "extdata")
x <- read_datasets(file.path(extdata_path, "example1_dat"))
attsurfs <- eaf_as_list(eaf(x, percentiles = c(0, 50, 100)))
str(attsurfs)
#> List of 3
#>  $ 0  : num [1:9, 1:2] 5128176 5134240 5142568 5144532 5155408 ...
#>  $ 50 : num [1:25, 1:2] 5135414 5136906 5137952 5143188 5146024 ...
#>  $ 100: num [1:16, 1:2] 5153534 5155512 5155716 5158466 5160938 ...
```
