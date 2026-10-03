# Exact R2 indicator

Computes the exact R2 indicator with respect to a given ideal/utopian
reference point assuming minimization of all objectives.

## Usage

``` r
r2_exact(x, reference, maximise = FALSE)
```

## Arguments

- x:

  [`matrix()`](https://rdrr.io/r/base/matrix.html)\|[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
  Matrix or data frame of numerical values, where each row gives the
  coordinates of a point.

- reference:

  [`numeric()`](https://rdrr.io/r/base/numeric.html)  
  Reference (ideal) point as a vector of numerical values.

- maximise:

  [`logical()`](https://rdrr.io/r/base/logical.html)  
  Whether the objectives must be maximised instead of minimised. Either
  a single logical value that applies to all objectives or a vector of
  logical values, with one value per objective.

## Value

`numeric(1)` A single numerical value.

## Details

The unary R2 indicator is a quality indicator for a set \\A \subset
\mathbb{R}^m\\ w.r.t. an ideal or utopian reference point \\\vec{r} \in
\mathbb{R}^m\\. It was originally proposed by Hansen and Jaszkiewicz
(1998) and is defined as the expected Tchebycheff utility under a
uniform distribution of weight vectors (w.l.o.g. assuming minimization):

\$\$R2(A) := \int\_{w \in W} \min\_{a \in A} \left\\ \max\_{i=1,\dots,m}
w_i (a_i - r_i) \right\\ \\ dw,\$\$

where \\W\\ denotes the uniform distribution across weights:

\$\$W = \\w \in \mathbb{R}^m \mid w_i \geq 0, \sum\_{i=1}^m w_i =
1\\.\$\$

The R2 indicator is to be minimized and has an optimal value of 0 when
\\\vec{r} \in A\\.

The exact R2 indicator is strongly Pareto-compliant, i.e., compatible
with Pareto-optimality:

\$\$\forall A, B \subset \mathbb{R}^m: A \prec B \Rightarrow R2(A) \<
R2(B).\$\$

Given an ideal or utopian reference point, which is available in most
scenarios, all non-dominated solutions always contribute to the value of
the exact R2 indicator. However, it is scale-dependent and care should
be taken such that all objectives contribute approximately equally to
the indicator, e.g., by normalizing the Pareto front to the unit
hypercube.

The current implementation exclusively supports bi-objective solution
sets and runs in \\O(n \log n)\\ following Schäpermeier and Kerschke
(2025) .

## References

Michael Pilegaard Hansen, Andrzej Jaszkiewicz (1998). “Evaluating the
quality of approximations to the non-dominated set.” Technical Report
IMM-REP-1998-7, Institute of Mathematical Modelling, Technical
University of Denmark, Lyngby, Denmark.  
  
Lennart Schäpermeier, Pascal Kerschke (2025). “R2 v2: The
Pareto-compliant R2 Indicator for Better Benchmarking in Bi-objective
Optimization.” *Evolutionary Computation*, 1–17.
[doi:10.1162/evco.a.366](https://doi.org/10.1162/evco.a.366) .

## Examples

``` r
dat <- matrix(c(5, 5, 4, 6, 2, 7, 7, 4), ncol = 2, byrow = TRUE)
r2_exact(dat, reference = c(0, 0))
#> [1] 2.594192

# This function assumes minimisation by default. We can easily specify maximisation:
r2_exact(dat, reference = c(10, 10), maximise = TRUE)
#> [1] 2.519697

# Merge all the sets of a dataset by removing the set number column:
extdata_path <- system.file(package="moocore","extdata")
dat <- read_datasets(file.path(extdata_path, "example1_dat"))[, 1:2]
nrow(dat)
#> [1] 65

# Dominated points are ignored, so this:
r2_exact(dat, reference = 0)
#> [1] 3865393

# gives the same exact R2 value as this:
dat <- filter_dominated(dat)
nrow(dat)
#> [1] 9
r2_exact(dat, reference = 0)
#> [1] 3865393
```
