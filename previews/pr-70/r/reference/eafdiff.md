# Compute empirical attainment function differences

Calculate the differences between the empirical attainment functions of
two data sets.

## Usage

``` r
eafdiff(x, y, intervals = NULL, maximise = FALSE, rectangles = FALSE)
```

## Arguments

- x, y:

  `matrix`\|[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
  Data frames corresponding to the input data of left and right sides,
  respectively. Each data frame has at least three columns, the last one
  is the set of each point. See also
  [`read_datasets()`](https://multi-objective.github.io/moocore/r/reference/read_datasets.md).

- intervals:

  `integer(1)`  
  The absolute range of the differences \\\[0, 1\]\\ is partitioned into
  the number of intervals provided.

- maximise:

  [`logical()`](https://rdrr.io/r/base/logical.html)  
  Whether the objectives must be maximised instead of minimised. Either
  a single logical value that applies to all objectives or a vector of
  logical values, with one value per objective.

- rectangles:

  `logical(1)`  
  If TRUE, the output is in the form of rectangles of the same color.

## Value

With `rectangle=FALSE`, a `data.frame` containing points where there is
a transition in the value of the EAF differences. With `rectangle=TRUE`,
a `matrix` where the first 4 columns give the coordinates of two corners
of each rectangle. In both cases, the last column gives the difference
in terms of sets in `x` minus sets in `y` that attain each point (i.e.,
negative values are differences in favour `y`).

## Details

This function calculates the differences between the EAFs of two data
sets.

## See also

[`read_datasets()`](https://multi-objective.github.io/moocore/r/reference/read_datasets.md)

## Examples

``` r
A1 <- read_datasets(text='
 3 2
 2 3

 2.5 1
 1 2

 1 2
')

A2 <- read_datasets(text='
 4 2.5
 3 3
 2.5 3.5

 3 3
 2.5 3.5

 2 1
')
d <- eafdiff(A1, A2)
str(d)
#>  num [1:9, 1:3] 1 2 2.5 2 2 3 2.5 3 4 2 ...
d
#>       [,1] [,2] [,3]
#>  [1,]  1.0  2.0    2
#>  [2,]  2.0  1.0   -1
#>  [3,]  2.5  1.0    0
#>  [4,]  2.0  2.0    1
#>  [5,]  2.0  3.0    2
#>  [6,]  3.0  2.0    2
#>  [7,]  2.5  3.5    0
#>  [8,]  3.0  3.0    0
#>  [9,]  4.0  2.5    1









d <- eafdiff(A1, A2, rectangles = TRUE)
str(d)
#>  num [1:9, 1:5] 2 1 2.5 2 2 2 3 3 4 1 ...
#>  - attr(*, "dimnames")=List of 2
#>   ..$ : NULL
#>   ..$ : chr [1:5] "xmin" "ymin" "xmax" "ymax" ...
d
#>       xmin ymin xmax ymax diff
#>  [1,]  2.0  1.0  2.5  2.0   -1
#>  [2,]  1.0  2.0  2.0  Inf    2
#>  [3,]  2.5  1.0  Inf  2.0    0
#>  [4,]  2.0  2.0  3.0  3.0    1
#>  [5,]  2.0  3.5  2.5  Inf    2
#>  [6,]  2.0  3.0  3.0  3.5    2
#>  [7,]  3.0  2.5  4.0  3.0    2
#>  [8,]  3.0  2.0  Inf  2.5    2
#>  [9,]  4.0  2.5  Inf  3.0    1










```
