# moocore 0.1.9

 * `hv_contributions()` ignores dominated points by default.  Set `ignore_dominated=FALSE` to restore the previous behavior.  The 3D case uses the HVC3D algorithm.
 * New function `any_dominated()`.
 * New function `generate_ndset()` to generate random nondominated sets with different shapes.
 * New article: "[Benchmarks](https://multi-objective.github.io/moocore/r/articles/benchmarks.html)"
 * New article: "[Computing Multi-Objective Quality Metrics](https://multi-objective.github.io/moocore/r/articles/metrics.html)"
 * New article: "[Sampling Random Nondominated Sets](https://multi-objective.github.io/moocore/r/articles/generate.html)"
 * `is_nondominated()`, `any_dominated()` and `pareto_rank()` now handle single-objective inputs correctly (#27) (#29).
 * `is_nondominated()` and `filter_dominated()` are faster for dimensions larger than 3.

# moocore 0.1.8

 * Document the EAF and Vorob'ev expectation and deviation in more detail.
 * New function `hv_approx()`.
 * Function `hv_contributions()` is much faster for 2D inputs.
 * New article "[Approximating the hypervolume](https://multi-objective.github.io/moocore/r/articles/hv_approx.html)".
 * New datasets `DTLZLinearShape.8d.front.60pts.10` and `ran.10pts.9d.10`.

# moocore 0.1.7

 * `hypervolume()` now uses the HV3D+ algorithm for the 3D case and the HV4D+ algorithm for the 4D case.
   For dimensions larger than 4, the recursive algorithm uses HV4D+ as the base case, which is significantly faster.

 * `read_datasets()` is significantly faster for large files.

 * `is_nondominated()` and `filter_dominated()` are faster for 3D inputs.

# moocore 0.1.6

 * Fix parallel build in CRAN.

# moocore 0.1.5

 * Rename `vorobT()` and `vorobDev()` to `vorob_t()` and `vorob_dev()` to be
   consistent with other function names.

# moocore 0.1.2

 * Fix more warnings and problems that only show in CRAN.

# moocore 0.1.1

 * Fix problems that only show in CRAN.

# moocore 0.1.0

 * Initial version uploaded to CRAN.
