# Read several data sets

Reads a text file in table format and creates a matrix from it. The file
may contain several sets, separated by empty lines. Lines starting by
`'#'` are considered comments and treated as empty lines. The function
adds an additional column `set` to indicate to which set each row
belongs.

## Usage

``` r
read_datasets(file, col_names, text)
```

## Arguments

- file:

  [`character()`](https://rdrr.io/r/base/character.html)  
  Filename that contains the data. Each row of the table appears as one
  line of the file. If it does not contain an *absolute* path, the file
  name is *relative* to the current working directory,
  [`base::getwd()`](https://rdrr.io/r/base/getwd.html). Tilde-expansion
  is performed where supported. Files compressed with `xz` are
  supported.

- col_names:

  [`character()`](https://rdrr.io/r/base/character.html)  
  Vector of optional names for the variables. The default is to use
  `"V"` followed by the column number.

- text:

  [`character()`](https://rdrr.io/r/base/character.html)  
  If `file` is not supplied and this is, then data are read from the
  value of `text` via a text connection. Notice that a literal string
  can be used to include (small) data sets within R code.

## Value

[`matrix()`](https://rdrr.io/r/base/matrix.html)  
A numerical matrix of the data in the file. An extra column `set` is
added to indicate to which set each row belongs.

## Note

There are several examples of data sets in
`system.file(package="moocore","extdata")`.

## Warning

A known limitation is that the input file must use newline characters
native to the host system, otherwise they will be, possibly silently,
misinterpreted. In GNU/Linux the program `dos2unix` may be used to fix
newline characters.

## See also

[`utils::read.table()`](https://rdrr.io/r/utils/read.table.html)

## Author

Manuel López-Ibáñez

## Examples

``` r
extdata_path <- system.file(package="moocore","extdata")
A1 <- read_datasets(file.path(extdata_path,"ALG_1_dat.xz"))
str(A1)
#>  num [1:23260, 1:3] 1.23e+10 1.11e+10 1.18e+10 1.13e+10 9.80e+09 ...
#>  - attr(*, "dimnames")=List of 2
#>   ..$ : NULL
#>   ..$ : chr [1:3] "V1" "V2" "set"

read_datasets(text="1 2\n3 4\n\n5 6\n7 8\n", col_names=c("obj1", "obj2"))
#>      obj1 obj2 set
#> [1,]    1    2   1
#> [2,]    3    4   1
#> [3,]    5    6   2
#> [4,]    7    8   2
```
