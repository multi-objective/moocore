# Approximation of the (weighted) hypervolume by Monte-Carlo sampling (2D only)

Return an estimation of the hypervolume of the space dominated by the
input data following the procedure described by Auger et al. (2009) . A
weight distribution describing user preferences may be specified.

## Usage

``` r
whv_hype(
  x,
  reference,
  ideal,
  maximise = FALSE,
  nsamples = 100000L,
  seed = NULL,
  dist = "uniform",
  mu = NULL
)
```

## Arguments

- x:

  [`matrix()`](https://rdrr.io/r/base/matrix.html)\|[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
  Matrix or data frame of numerical values, where each row gives the
  coordinates of a point.

- reference:

  [`numeric()`](https://rdrr.io/r/base/numeric.html)  
  Reference point as a vector of numerical values.

- ideal:

  [`numeric()`](https://rdrr.io/r/base/numeric.html)  
  Ideal point as a vector of numerical values.

- maximise:

  [`logical()`](https://rdrr.io/r/base/logical.html)  
  Whether the objectives must be maximised instead of minimised. Either
  a single logical value that applies to all objectives or a vector of
  logical values, with one value per objective.

- nsamples:

  `integer(1)`  
  Number of samples for Monte-Carlo sampling.

- seed:

  `integer(1)`  
  Random seed.

- dist:

  `character(1)`  
  Weight distribution type. See Details.

- mu:

  [`numeric()`](https://rdrr.io/r/base/numeric.html)  
  Parameter of the weight distribution. See Details.

## Value

A single numerical value.

## Details

The current implementation only supports 2 objectives.

A weight distribution (Auger et al. 2009) can be provided via the `dist`
argument. The ones currently supported are:

- `"uniform"` corresponds to the default hypervolume (unweighted).

- `"point"` describes a goal in the objective space, where the parameter
  `mu` gives the coordinates of the goal. The resulting weight
  distribution is a multivariate normal distribution centred at the
  goal.

- `"exponential"` describes an exponential distribution with rate
  parameter `1/mu`, i.e., \\\lambda = \frac{1}{\mu}\\.

## References

Anne Auger, Johannes Bader, Dimo Brockhoff, Eckart Zitzler (2009).
“Articulating User Preferences in Many-Objective Problems by Sampling
the Weighted Hypervolume.” In Franz Rothlauf (ed.), *Proceedings of the
Genetic and Evolutionary Computation Conference, GECCO 2009*, 555–562.
ACM Press, New York, NY.

## See also

[`read_datasets()`](https://multi-objective.github.io/moocore/r/reference/read_datasets.md),
[`eafdiff()`](https://multi-objective.github.io/moocore/r/reference/eafdiff.md),
[`whv_rect()`](https://multi-objective.github.io/moocore/r/reference/whv_rect.md)

## Examples

``` r
whv_hype(matrix(2, ncol=2), reference = 4, ideal = 1, seed = 42)
#> [1] 3.99807
whv_hype(matrix(c(3,1), ncol=2), reference = 4, ideal = 1, seed = 42)
#> [1] 3.00555
whv_hype(matrix(2, ncol=2), reference = 4, ideal = 1, seed = 42,
         dist = "exponential", mu=0.2)
#> [1] 1.14624
whv_hype(matrix(c(3,1), ncol=2), reference = 4, ideal = 1, seed = 42,
         dist = "exponential", mu=0.2)
#> [1] 1.66815
whv_hype(matrix(2, ncol=2), reference = 4, ideal = 1, seed = 42,
         dist = "point", mu=c(2.9,0.9))
#> [1] 0.64485
whv_hype(matrix(c(3,1), ncol=2), reference = 4, ideal = 1, seed = 42,
         dist = "point", mu=c(2.9,0.9))
#> [1] 4.03632
```
