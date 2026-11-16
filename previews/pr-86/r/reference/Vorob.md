# Vorob'ev threshold, expectation and deviation

Compute Vorob'ev threshold, expectation and deviation. Also, displaying
the symmetric deviation function is possible. The symmetric deviation
function is the probability for a given target in the objective space to
belong to the symmetric difference between the Vorob'ev expectation and
a realization of the (random) attained set.

## Usage

``` r
vorob_t(x, sets, reference, maximise = FALSE)

vorob_dev(x, sets, reference, ve = NULL, maximise = FALSE)
```

## Arguments

- x:

  [`matrix()`](https://rdrr.io/r/base/matrix.html)\|[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
  Matrix or data frame of numerical values that represents multiple sets
  of points, where each row represents a point. If `sets` is missing,
  the last column of `x` gives the sets.

- sets:

  [`integer()`](https://rdrr.io/r/base/integer.html)  
  Vector that indicates the set of each point in `x`. If missing, the
  last column of `x` is used instead.

- reference:

  [`numeric()`](https://rdrr.io/r/base/numeric.html)  
  Reference point as a vector of numerical values.

- maximise:

  [`logical()`](https://rdrr.io/r/base/logical.html)  
  Whether the objectives must be maximised instead of minimised. Either
  a single logical value that applies to all objectives or a vector of
  logical values, with one value per objective.

- ve:

  [`matrix()`](https://rdrr.io/r/base/matrix.html)  
  Vorob'ev expectation, e.g., as returned by `vorob_t()`.

## Value

`vorob_t` returns a list with elements `threshold`, `ve`, and `avg_hyp`
(average hypervolume)

`vorob_dev` returns the Vorob'ev deviation.

## Details

Let \\\mathcal{A} = \\A_1, \dots, A_n\\\\ be a multi-set of \\n\\ sets
\\A_i \subset \mathbb{R}^d\\ of mutually nondominated vectors, with
finite (but not necessarily equal) cardinality. If bounded by a
reference point \\\vec{r}\\ that is strictly dominated by any point in
any set, then these sets can be seen a samples from a random closed set
(Molchanov 2005) .

Let the \\\beta\\-quantile be the subset of the empirical attainment
function \\\mathcal{Q}\_\beta = \\\vec{z}\in \mathbb{R}^d :
\hat{\alpha}\_{\mathcal{A}}(\vec{z}) \geq \beta\\\\.

The Vorob'ev *expectation* is the \\\beta^{\*}\\-quantile set
\\\mathcal{Q}\_{\beta^{\*}}\\ such that the mean value hypervolume of
the sets is equal (or as close as possible) to the hypervolume of
\\\mathcal{Q}\_{\beta^{\*}}\\, that is, \\\text{hyp}(\mathcal{Q}\_\beta)
\leq \mathbb{E}\[\text{hyp}(\mathcal{A})\] \leq
\text{hyp}(\mathcal{Q}\_{\beta^{\*}})\\, \\\forall \beta \>
\beta^{\*}\\. Thus, the Vorob'ev expectation provides a definition of
the notion of *mean* nondominated set.

The value \\\beta^{\*} \in \[0,1\]\\ is called the Vorob'ev *threshold*.
Large differences from the median quantile (0.5) indicate a skewed
distribution of \\\mathcal{A}\\.

The Vorob'ev *deviation* is the mean hypervolume of the symmetric
difference between the Vorob'ev expectation and any set in
\\\mathcal{A}\\, that is,
\\\mathbb{E}\[\text{hyp}(\mathcal{Q}\_{\beta^{\*}} \ominus
\mathcal{A})\]\\, where the symmetric difference is defined as \\A
\ominus B = (A \setminus B) \cup (B \setminus A)\\. Low deviation values
indicate that the sets are very similar, in terms of the location of the
weakly dominated space, to the Vorob'ev expectation.

For more background, see Binois et al. (2015); Molchanov (2005);
Chevalier et al. (2013) .

## References

Mickaël Binois, David Ginsbourger, Olivier Roustant (2015). “Quantifying
uncertainty on Pareto fronts with Gaussian process conditional
simulations.” *European Journal of Operational Research*, **243**(2),
386–394.
[doi:10.1016/j.ejor.2014.07.032](https://doi.org/10.1016/j.ejor.2014.07.032)
.  
  
Clément Chevalier, David Ginsbourger, Julien Bect, Ilya Molchanov
(2013). “Estimating and Quantifying Uncertainties on Level Sets Using
the Vorob'ev Expectation and Deviation with Gaussian Process Models.” In
Dariusz Ucinski, Anthony C. Atkinson, Maciej Patan (eds.), *mODa
10–Advances in Model-Oriented Design and Analysis*, 35–43. Springer
International Publishing, Heidelberg, Germany.
[doi:10.1007/978-3-319-00218-7_5](https://doi.org/10.1007/978-3-319-00218-7_5)
.  
  
Ilya Molchanov (2005). *Theory of Random Sets*. Springer.

## Author

Mickael Binois

## Examples

``` r
data(CPFs)
res <- vorob_t(CPFs, reference = c(2, 200))
res$threshold
#> [1] 44.14062
res$avg_hyp
#> [1] 8943.333
# Now print Vorob'ev deviation
vd <- vorob_dev(CPFs, ve = res$ve, reference = c(2, 200))
vd
#> [1] 3017.13
```
