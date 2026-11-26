# Results of SPEA2 when minimising electrical cost and maximising the minimum idle time of pumps on Richmond water network.

Results of SPEA2 when minimising electrical cost and maximising the
minimum idle time of pumps on Richmond water network.

## Usage

``` r
SPEA2minstoptimeRichmond
```

## Format

A data frame as produced by
[`read_datasets()`](https://multi-objective.github.io/moocore/r/reference/read_datasets.md).
The second column measures time in seconds and corresponds to a
maximisation problem.

## Source

Manuel López-Ibáñez (2009). *Operational Optimisation of Water
Distribution Networks*. Ph.D. thesis, School of Engineering and the
Built Environment, Edinburgh Napier University, UK.
<https://lopez-ibanez.eu/publications#LopezIbanezPhD>.

## Examples

``` r
data(SPEA2minstoptimeRichmond)
str(SPEA2minstoptimeRichmond)
#> 'data.frame':    166 obs. of  3 variables:
#>  $ V1 : num  106 108 109 109 109 ...
#>  $ V2 : num  357 911 3960 7560 16532 ...
#>  $ set: num  1 1 1 1 1 1 1 1 1 1 ...
```
