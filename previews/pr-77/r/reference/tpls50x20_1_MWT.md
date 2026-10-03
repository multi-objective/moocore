# Various strategies of Two-Phase Local Search applied to the Permutation Flowshop Problem with Makespan and Weighted Tardiness objectives.

Various strategies of Two-Phase Local Search applied to the Permutation
Flowshop Problem with Makespan and Weighted Tardiness objectives.

## Usage

``` r
tpls50x20_1_MWT
```

## Format

A data frame with 1511 observations of 4 variables:

- `algorithm`:

  TPLS search strategy

- `Makespan`:

  first objective values.

- `WeightedTardiness`:

  second objective values.

- `run`:

  index of the run.

## Source

Jérémie Dubois-Lacoste, Manuel López-Ibáñez, Thomas Stützle (2011).
“Improving the Anytime Behavior of Two-Phase Local Search.” *Annals of
Mathematics and Artificial Intelligence*, **61**(2), 125–154.
[doi:10.1007/s10472-011-9235-0](https://doi.org/10.1007/s10472-011-9235-0)
.

## Examples

``` r
data(tpls50x20_1_MWT)
str(tpls50x20_1_MWT)
#> 'data.frame':    1511 obs. of  4 variables:
#>  $ algorithm        : chr  "1to2" "1to2" "1to2" "1to2" ...
#>  $ Makespan         : num  4280 4238 4137 4024 4014 ...
#>  $ WeightedTardiness: num  10231 10999 11737 14871 17825 ...
#>  $ run              : num  1 1 1 1 1 1 1 1 1 1 ...
```
