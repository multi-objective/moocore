# Write data sets

Write data sets to a file in the same format as
[`read_datasets()`](https://multi-objective.github.io/moocore/r/reference/read_datasets.md).

## Usage

``` r
write_datasets(x, file = "")
```

## Arguments

- x:

  `matrix`\|[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
  Dataset with at least three columns, the last one is the set of each
  point. See also
  [`read_datasets()`](https://multi-objective.github.io/moocore/r/reference/read_datasets.md).

- file:

  Either a character string naming a file or a connection open for
  writing. `""` indicates output to the console.

## Value

No return value, called for side effects

## See also

[`utils::write.table()`](https://rdrr.io/r/utils/write.table.html),
[`read_datasets()`](https://multi-objective.github.io/moocore/r/reference/read_datasets.md)

## Examples

``` r
x <- read_datasets(text="1 2\n3 4\n\n5 6\n7 8\n", col_names=c("obj1", "obj2"))
write_datasets(x)
#> # obj1   obj2 
#> 1 2
#> 3 4
#> 
#> 5 6
#> 7 8
```
