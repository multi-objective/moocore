# Same as [`eafdiff()`](https://multi-objective.github.io/moocore/r/reference/eafdiff.md) but performs no checks and does not transform the input or the output. This function should be used by other packages that want to avoid redundant checks and transformations.

Same as
[`eafdiff()`](https://multi-objective.github.io/moocore/r/reference/eafdiff.md)
but performs no checks and does not transform the input or the output.
This function should be used by other packages that want to avoid
redundant checks and transformations.

## Usage

``` r
compute_eafdiff_call(x, y, cumsizes_x, cumsizes_y, intervals, ret)
```

## Arguments

- x, y:

  `matrix`\|[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
  Data frames corresponding to the input data of left and right sides,
  respectively. Each data frame has at least three columns, the last one
  is the set of each point. See also
  [`read_datasets()`](https://multi-objective.github.io/moocore/r/reference/read_datasets.md).

- cumsizes_x, cumsizes_y:

  Cumulative size of the different sets of points in `x` and `y`.

- intervals:

  `integer(1)`  
  The absolute range of the differences \\\[0, 1\]\\ is partitioned into
  the number of intervals provided.

- ret:

  (`"points"|"rectangles"|"polygons"`)  
  The format of the returned EAF differences.

## Value

With `rectangle=FALSE`, a `data.frame` containing points where there is
a transition in the value of the EAF differences. With `rectangle=TRUE`,
a `matrix` where the first 4 columns give the coordinates of two corners
of each rectangle. In both cases, the last column gives the difference
in terms of sets in `x` minus sets in `y` that attain each point (i.e.,
negative values are differences in favour `y`).

## See also

[`as_double_matrix()`](https://multi-objective.github.io/moocore/r/reference/as_double_matrix.md)
[`transform_maximise()`](https://multi-objective.github.io/moocore/r/reference/transform_maximise.md)
