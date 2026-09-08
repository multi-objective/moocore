# Interactively choose according to empirical attainment function differences

Interactively choose according to empirical attainment function
differences

## Usage

``` r
choose_eafdiff(x, left = stop("'left' must be either TRUE or FALSE"))
```

## Arguments

- x:

  [`matrix()`](https://rdrr.io/r/base/matrix.html)  
  Matrix of rectangles representing EAF differences returned by
  [`eafdiff()`](https://multi-objective.github.io/moocore/r/reference/eafdiff.md)
  with `rectangles=TRUE`.

- left:

  `logical(1)`  
  With `left=TRUE` return the rectangles with positive differences,
  otherwise return those with negative differences but differences are
  converted to positive.

## Value

[`matrix()`](https://rdrr.io/r/base/matrix.html) where the first 4
columns give the coordinates of two corners of each rectangle and the
last column. In both cases, the last column gives the positive
differences in favor of the chosen side.

## Examples

``` r
# \donttest{
extdata_dir <- system.file(package="moocore", "extdata")
A1 <- read_datasets(file.path(extdata_dir, "wrots_l100w10_dat"))
A2 <- read_datasets(file.path(extdata_dir, "wrots_l10w100_dat"))
# Choose A1
rectangles <- eafdiff(A1, A2, intervals = 5, rectangles = TRUE)
rectangles <- choose_eafdiff(rectangles, left = TRUE)
reference <- c(max(A1[, 1], A2[, 1]), max(A1[, 2], A2[, 2]))
x <- split.data.frame(A1[,1:2], A1[,3])
hv_A1 <- sapply(split.data.frame(A1[, 1:2], A1[, 3]),
                 hypervolume, reference=reference)
hv_A2 <- sapply(split.data.frame(A2[, 1:2], A2[, 3]),
                 hypervolume, reference=reference)
print(fivenum(hv_A1))
#>           41           67            5           89           86 
#> 785206811240 798074793438 802769574696 810246464630 827967721404 
print(fivenum(hv_A2))
#>           29           46           24           74           70 
#> 814132427856 820458749530 823618555606 826673145848 836333173304 
whv_A1 <- sapply(split.data.frame(A1[, 1:2], A1[, 3]),
                 whv_rect, rectangles=rectangles, reference=reference)
whv_A2 <- sapply(split.data.frame(A2[, 1:2], A2[, 3]),
                 whv_rect, rectangles=rectangles, reference=reference)
print(fivenum(whv_A1))
#>          58           9          78           4          16 
#>  1891625587  6232795880  8278480355  9995743574 15230375078 
print(fivenum(whv_A2))
#>          84          20          65           5          10 
#>   906264527  2827084909  4651576336  6792802423 11748787068 
# }
```
