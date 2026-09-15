# Results of Hybrid GA on Vanzyl and Richmond water networks

Results of Hybrid GA on Vanzyl and Richmond water networks

## Usage

``` r
HybridGA
```

## Format

A list with two data frames, each of them with three columns, as
produced by
[`read_datasets()`](https://multi-objective.github.io/moocore/r/reference/read_datasets.md).

- `$vanzyl`:

  data frame of results on Vanzyl network

- `$richmond`:

  data frame of results on Richmond network. The second column is filled
  with `NA`

## Source

Manuel López-Ibáñez (2009). *Operational Optimisation of Water
Distribution Networks*. Ph.D. thesis, School of Engineering and the
Built Environment, Edinburgh Napier University, UK.
<https://lopez-ibanez.eu/publications#LopezIbanezPhD>. .

## Examples

``` r
data(HybridGA)
print(HybridGA$vanzyl)
#>       V1 V2
#> 1 347.10  4
#> 2 352.06  3
#> 3 352.15  4
#> 4 344.43  5
#> 5 344.81  4
#> 6 354.79  5
#> 7 344.74  5
print(HybridGA$richmond)
#>     V1 V2
#> 1  101 NA
#> 2  104 NA
#> 3  100 NA
#> 4   97 NA
#> 5   97 NA
#> 6  101 NA
#> 7  101 NA
#> 8   99 NA
#> 9   98 NA
#> 10  98 NA
```
