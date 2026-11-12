# Exact computation of the Empirical Attainment Function (EAF)

This function computes the EAF given a set of 2D or 3D points and a
vector `set` that indicates to which set each point belongs.

## Usage

``` r
eaf(x, sets, percentiles = NULL, maximise = FALSE, groups = NULL)
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

- percentiles:

  [`numeric()`](https://rdrr.io/r/base/numeric.html)  
  Vector indicating which percentiles are computed. `NULL` computes all.

- maximise:

  [`logical()`](https://rdrr.io/r/base/logical.html)  
  Whether the objectives must be maximised instead of minimised. Either
  a single logical value that applies to all objectives or a vector of
  logical values, with one value per objective.

- groups:

  [`factor()`](https://rdrr.io/r/base/factor.html)  
  Indicates that the EAF must be computed separately for data belonging
  to different groups.

## Value

[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
A data frame containing the exact representation of EAF. The last column
gives the percentile that corresponds to each point. If groups is not
`NULL`, then an additional column indicates to which group the point
belongs.

## Details

The empirical first-order attainment function (EAF) is used to assess
the performance of stochastic multiobjective optimisers such as
multiobjective evolutionary algorithms (Grunert da Fonseca et al. 2001)
. It is an estimator for the first-order attainment function, which
provides information about the location and, to some extent, the
variability of the random sets of nondominated objective vectors
produced by such optimisers when applied to given problem instances.

Given a set \\A \subset \mathbb{R}^d\\, the attainment function of
\\A\\, denoted by \\\alpha\_{A}\colon \mathbb{R}^d\to \\0,1\\\\,
specifies which points in the objective space are weakly dominated by
\\A\\, where \\\alpha_A(\vec{z}) = 1\\ if \\\exists \vec{a} \in A,
\vec{a} \leq \vec{z}\\, and \\\alpha_A(\vec{z}) = 0\\, otherwise.

Let \\\mathcal{A} = \\A_1, \dots, A_n\\\\ be a multi-set of \\n\\ sets
\\A_i \subset \mathbb{R}^d\\, the EAF (Grunert da Fonseca et al. 2001;
Grunert da Fonseca and Fonseca 2010) is the function
\\\hat{\alpha}\_{\mathcal{A}}\colon \mathbb{R}^d\to \[0,1\]\\, such
that:

\$\$\hat{\alpha}\_{\mathcal{A}}(\vec{z}) = \frac{1}{n}\sum\_{i=1}^n
\alpha\_{A_i}(\vec{z})\$\$

The EAF is a coordinate-wise non-decreasing step function, similar to
the empirical cumulative distribution function (ECDF) (López-Ibáñez et
al. 2025) . Thus, a finite representation of the EAF can be computed as
the set of minima, in terms of Pareto optimality, with a value of the
EAF not smaller than a given \\t/n\\, where \\t=1,\dots,n\\ (Fonseca et
al. 2011) . Formally, the EAF can be represented by the sequence \\(L_1,
L_2, \dots, L_n)\\, where:

\$\$L_t = \min \\\vec{z} \in \mathbb{R}^d :
\hat{\alpha}\_{\mathcal{A}}(\vec{z}) \geq t/n\\\$\$

It is also common to refer to the \\k\\ \in \[0,100\]\\ percentile. For
example, the *median* (or 50%) attainment surface corresponds to
\\L\_{\lceil n/2 \rceil}\\ and it is the lower boundary of the vector
space attained by at least 50% of the input sets \\A_i\\. Similarly,
\\L_1\\ is called the *best* attainment surface (\\\frac{1}{n}\\%) and
represents the lower boundary of the space attained by at least one
input set, whereas \\L\_{100}\\ is called the *worst* attainment surface
(100%) and represents the lower boundary of the space attained by all
input sets.

In the current implementation, the EAF is computed using the algorithms
proposed by Fonseca et al. (2011) , which have complexity \\\Theta(m\log
m + nm)\\ in 2D and \\O(n^2 m \log m)\\ in 3D, where \\n\\ is the number
of input sets and \\m\\ is the total number of input points.

## Note

There are several examples of data sets in
`system.file(package="moocore","extdata")`. The current implementation
only supports two and three dimensional points.

## References

Carlos M. Fonseca, Andreia P. Guerreiro, Manuel López-Ibáñez, Luís
Paquete (2011). “On the Computation of the Empirical Attainment
Function.” In R H C Takahashi, Kalyanmoy Deb, Elizabeth F. Wanner,
Salvatore Greco (eds.), *Evolutionary Multi-criterion Optimization, EMO
2011*, volume 6576 of *Lecture Notes in Computer Science*, 106–120.
Springer, Berlin~/ Heidelberg.
[doi:10.1007/978-3-642-19893-9_8](https://doi.org/10.1007/978-3-642-19893-9_8)
.  
  
Viviane Grunert da Fonseca, Carlos M. Fonseca (2010). “The
Attainment-Function Approach to Stochastic Multiobjective Optimizer
Assessment and Comparison.” In Thomas Bartz-Beielstein, Marco
Chiarandini, Luís Paquete, Mike Preuss (eds.), *Experimental Methods for
the Analysis of Optimization Algorithms*, 103–130. Springer, Berlin~/
Heidelberg.
[doi:10.1007/978-3-642-02538-9_5](https://doi.org/10.1007/978-3-642-02538-9_5)
.  
  
Viviane Grunert da Fonseca, Carlos M. Fonseca, Andreia O. Hall (2001).
“Inferential Performance Assessment of Stochastic Optimisers and the
Attainment Function.” In Eckart Zitzler, Kalyanmoy Deb, Lothar Thiele,
Carlos A. Coello Coello, David Corne (eds.), *Evolutionary
Multi-criterion Optimization, EMO 2001*, volume 1993 of *Lecture Notes
in Computer Science*, 213–225. Springer, Berlin~/ Heidelberg.
[doi:10.1007/3-540-44719-9_15](https://doi.org/10.1007/3-540-44719-9_15)
.  
  
Manuel López-Ibáñez, Diederick Vermetten, Johann Dreo, Carola Doerr
(2025). “Using the Empirical Attainment Function for Analyzing
Single-objective Black-box Optimization Algorithms.” *IEEE Transactions
on Evolutionary Computation*, **29**(5), 1774–1782.
[doi:10.1109/TEVC.2024.3462758](https://doi.org/10.1109/TEVC.2024.3462758)
.

## See also

[`read_datasets()`](https://multi-objective.github.io/moocore/r/reference/read_datasets.md)

## Author

Manuel López-Ibáñez

## Examples

``` r
extdata_path <- system.file(package="moocore", "extdata")

x <- read_datasets(file.path(extdata_path, "example1_dat"))
# Compute full EAF (sets is the last column)
str(eaf(x))
#>  num [1:215, 1:3] 5128176 5134240 5142568 5144532 5155408 ...

# Compute only best, median and worst
str(eaf(x[,1:2], sets = x[,3], percentiles = c(0, 50, 100)))
#>  num [1:50, 1:3] 5128176 5134240 5142568 5144532 5155408 ...

x <- read_datasets(file.path(extdata_path, "spherical-250-10-3d.txt"))
y <- read_datasets(file.path(extdata_path, "uniform-250-10-3d.txt"))
x <- rbind(data.frame(x, groups = "spherical"),
           data.frame(y, groups = "uniform"))
# Compute only median separately for each group
z <- eaf(x[,1:3], sets = x[,4], groups = x[,5], percentiles = 50)
str(z)
#> 'data.frame':    12650 obs. of  5 variables:
#>  $ X1    : num  0.865 0.787 0.682 0.739 0.865 ...
#>  $ X2    : num  0.966 0.966 0.997 0.966 0.926 ...
#>  $ X3    : num  0.00264 0.00421 0.00483 0.00449 0.00421 ...
#>  $ X4    : num  50 50 50 50 50 50 50 50 50 50 ...
#>  $ groups: chr  "spherical" "spherical" "spherical" "spherical" ...
```
