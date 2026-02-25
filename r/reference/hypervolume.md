# Hypervolume metric

Compute the hypervolume metric with respect to a given reference point
assuming minimization of all objectives.

## Usage

``` r
hypervolume(x, reference, maximise = FALSE)
```

## Arguments

- x:

  [`matrix()`](https://rdrr.io/r/base/matrix.html)\|[`data.frame()`](https://rdrr.io/r/base/data.frame.html)  
  Matrix or data frame of numerical values, where each row gives the
  coordinates of a point.

- reference:

  [`numeric()`](https://rdrr.io/r/base/numeric.html)  
  Reference point as a vector of numerical values.

- maximise:

  [`logical()`](https://rdrr.io/r/base/logical.html)  
  Whether the objectives must be maximised instead of minimised. Either
  a single logical value that applies to all objectives or a vector of
  logical values, with one value per objective.

## Value

`numeric(1)` A single numerical value.

## Details

The hypervolume of a set of multidimensional points \\A \subset
\mathbb{R}^m\\, where \\m\\ is the dimension of the points, with respect
to a reference point \\\vec{r} \in \mathbb{R}^m\\ is the volume of the
region dominated by the set and bounded by the reference point (Zitzler
and Thiele 1998) . Points in \\A\\ that do not strictly dominate
\\\vec{r}\\ do not contribute to the hypervolume value, thus, ideally,
the reference point must be strictly dominated by all points in the true
Pareto front.

More precisely, the hypervolume is the [Lebesgue
measure](https://en.wikipedia.org/wiki/Lebesgue_measure) of the union of
axis-aligned hyperrectangles
([orthotopes](https://en.wikipedia.org/wiki/Hyperrectangle)), where each
hyperrectangle is defined by one point from \\\vec{a} \in A\\ and the
reference point. The union of axis-aligned hyperrectangles is also
called an *orthogonal polytope*.

The hypervolume is compatible with Pareto-optimality (Knowles and Corne
2002; Zitzler et al. 2003) , that is, \\\nexists A,B \subset
\mathbb{R}^m\\, such that \\A\\ is better than \\B\\ in terms of
Pareto-optimality and \\\text{hyp}(A) \leq \text{hyp}(B)\\. In other
words, if a set is better than another in terms of Pareto-optimality,
the hypervolume of the former must be strictly larger than the
hypervolume of the latter. Conversely, if the hypervolume of a set is
larger than the hypervolume of another, then we know for sure than the
latter set cannot be better than the former in terms of
Pareto-optimality.

Like most measures of unions of high-dimensional geometric objects,
computing the hypervolume is \#P-hard (Bringmann and Friedrich 2010) .

For 2D and 3D, the algorithms used (Fonseca et al. 2006; Beume et al.
2009) have \\O(n \log n)\\ complexity, where \\n\\ is the number of
input points. The 3D case uses the \\\text{HV3D}^{+}\\ algorithm
(Guerreiro and Fonseca 2018) , which has the sample complexity as the
HV3D algorithm (Fonseca et al. 2006; Beume et al. 2009) , but it is
faster in practice.

For 4D, the algorithm used is \\\text{HV4D}^{+}\\ (Guerreiro and Fonseca
2018) , which has \\O(n^2)\\ complexity. Compared to the [original
implementation](https://github.com/apguerreiro/HVC/), this
implementation correctly handles weakly dominated points and has been
further optimized for speed.

For 5D or higher and up to 15 points, the implementation uses the
inclusion-exclusion algorithm (Wu and Azam 2001) , which has \\O(m
2^{n})\\ time and \\O(n\cdot m)\\ space complexity, but it is very fast
for such small sets. For larger number of points, it uses a recursive
algorithm (Fonseca et al. 2006) that computes 4D contributions
(Guerreiro and Fonseca 2018) as the base case, resulting in a
\\O(n^{m-2})\\ time complexity and \\O(n)\\ space complexity in the
worst-case. Experimental results show that the pruning techniques used
may reduce the time complexity even further. The original proposal
(Fonseca et al. 2006) had the HV3D algorithm as the base case, giving a
time complexity of \\O(n^{m-2} \log n)\\. Andreia P. Guerreiro enhanced
the numerical stability of the algorithm by avoiding floating-point
comparisons of partial hypervolumes.

The hypervolume of 1D inputs is defined as `max(0, ref - min(x))`.

## References

Nicola Beume, Carlos M. Fonseca, Manuel López-Ibáñez, Luís Paquete, Jan
Vahrenhold (2009). “On the complexity of computing the hypervolume
indicator.” *IEEE Transactions on Evolutionary Computation*, **13**(5),
1075–1082.
[doi:10.1109/TEVC.2009.2015575](https://doi.org/10.1109/TEVC.2009.2015575)
.  
  
Karl Bringmann, Tobias Friedrich (2010). “Approximating the volume of
unions and intersections of high-dimensional geometric objects.”
*Computational Geometry*, **43**(6–7), 601–610.
[doi:10.1016/j.comgeo.2010.03.004](https://doi.org/10.1016/j.comgeo.2010.03.004)
.  
  
Carlos M. Fonseca, Luís Paquete, Manuel López-Ibáñez (2006). “An
improved dimension-sweep algorithm for the hypervolume indicator.” In
*Proceedings of the 2006 Congress on Evolutionary Computation (CEC
2006)*, 1157–1163.
[doi:10.1109/CEC.2006.1688440](https://doi.org/10.1109/CEC.2006.1688440)
.  
  
Andreia P. Guerreiro, Carlos M. Fonseca (2018). “Computing and Updating
Hypervolume Contributions in Up to Four Dimensions.” *IEEE Transactions
on Evolutionary Computation*, **22**(3), 449–463.
[doi:10.1109/tevc.2017.2729550](https://doi.org/10.1109/tevc.2017.2729550)
.  
  
Joshua D. Knowles, David Corne (2002). “On Metrics for Comparing
Non-Dominated Sets.” In *Proceedings of the 2002 Congress on
Evolutionary Computation (CEC'02)*, 711–716.  
  
J Wu, S Azam (2001). “Metrics for Quality Assessment of a Multiobjective
Design Optimization Solution Set.” *Journal of Mechanical Design*,
**123**(1), 18–25.
[doi:10.1115/1.1329875](https://doi.org/10.1115/1.1329875) .  
  
Eckart Zitzler, Lothar Thiele (1998). “Multiobjective Optimization Using
Evolutionary Algorithms - A Comparative Case Study.” In Agoston E.
Eiben, Thomas Bäck, Marc Schoenauer, Hans-Paul Schwefel (eds.),
*Parallel Problem Solving from Nature – PPSN V*, volume 1498 of *Lecture
Notes in Computer Science*, 292–301. Springer, Heidelberg, Germany.
[doi:10.1007/BFb0056872](https://doi.org/10.1007/BFb0056872) .  
  
Eckart Zitzler, Lothar Thiele, Marco Laumanns, Carlos M. Fonseca,
Viviane Grunert da Fonseca (2003). “Performance Assessment of
Multiobjective Optimizers: an Analysis and Review.” *IEEE Transactions
on Evolutionary Computation*, **7**(2), 117–132.
[doi:10.1109/TEVC.2003.810758](https://doi.org/10.1109/TEVC.2003.810758)
.

## Author

Manuel López-Ibáñez

## Examples

``` r
dat = matrix(c(5, 5, 4, 6, 2, 7, 7, 4), ncol=2, byrow=TRUE)
hypervolume(dat, ref=c(10, 10))
#> [1] 38
hypervolume(dat, ref=0, maximise=TRUE)
#> [1] 39

data(SPEA2minstoptimeRichmond)
# The second objective must be maximized
# We calculate the hypervolume of the union of all sets.
hypervolume(SPEA2minstoptimeRichmond[, 1:2], reference = c(250, 0),
            maximise = c(FALSE, TRUE))
#> [1] 7911376
```
