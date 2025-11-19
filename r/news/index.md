# Changelog

## moocore 0.2.0

- `mooocore` now requires R \>= 4.1.
- [`pareto_rank()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md)
  is faster in 3D.

## moocore 0.1.9

CRAN release: 2025-11-01

- [`hv_contributions()`](https://multi-objective.github.io/moocore/r/reference/hv_contributions.md)
  ignores dominated points by default. Set `ignore_dominated=FALSE` to
  restore the previous behavior. The 3D case uses the HVC3D algorithm.
- New function
  [`any_dominated()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md).
- New function
  [`generate_ndset()`](https://multi-objective.github.io/moocore/r/reference/generate_ndset.md)
  to generate random nondominated sets with different shapes.
- New article:
  “[Benchmarks](https://multi-objective.github.io/moocore/r/articles/benchmarks.html)”
- New article: “[Computing Multi-Objective Quality
  Metrics](https://multi-objective.github.io/moocore/r/articles/metrics.html)”
- New article: “[Sampling Random Nondominated
  Sets](https://multi-objective.github.io/moocore/r/articles/generate.html)”
- [`is_nondominated()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md),
  [`any_dominated()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md)
  and
  [`pareto_rank()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md)
  now handle single-objective inputs correctly
  ([\#27](https://github.com/multi-objective/moocore/issues/27))
  ([\#29](https://github.com/multi-objective/moocore/issues/29)).
- [`is_nondominated()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md)
  and
  [`filter_dominated()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md)
  are faster for dimensions larger than 3.
- [`is_nondominated()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md)
  and
  [`filter_dominated()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md)
  are now stable in 2D and 3D with `keep_weakly=FALSE`, that is, only
  the first of duplicated points is marked as nondominated.

## moocore 0.1.8

CRAN release: 2025-07-15

- Document the EAF and Vorob’ev expectation and deviation in more
  detail.
- New function
  [`hv_approx()`](https://multi-objective.github.io/moocore/r/reference/hv_approx.md).
- Function
  [`hv_contributions()`](https://multi-objective.github.io/moocore/r/reference/hv_contributions.md)
  is much faster for 2D inputs.
- New article “[Approximating the
  hypervolume](https://multi-objective.github.io/moocore/r/articles/hv_approx.html)”.
- New datasets `DTLZLinearShape.8d.front.60pts.10` and
  `ran.10pts.9d.10`.

## moocore 0.1.7

CRAN release: 2025-06-05

- [`hypervolume()`](https://multi-objective.github.io/moocore/r/reference/hypervolume.md)
  now uses the HV3D+ algorithm for the 3D case and the HV4D+ algorithm
  for the 4D case. For dimensions larger than 4, the recursive algorithm
  uses HV4D+ as the base case, which is significantly faster.

- [`read_datasets()`](https://multi-objective.github.io/moocore/r/reference/read_datasets.md)
  is significantly faster for large files.

- [`is_nondominated()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md)
  and
  [`filter_dominated()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md)
  are faster for 3D inputs.

## moocore 0.1.6

CRAN release: 2025-05-13

- Fix parallel build in CRAN.

## moocore 0.1.5

CRAN release: 2025-05-11

- Rename `vorobT()` and `vorobDev()` to
  [`vorob_t()`](https://multi-objective.github.io/moocore/r/reference/Vorob.md)
  and
  [`vorob_dev()`](https://multi-objective.github.io/moocore/r/reference/Vorob.md)
  to be consistent with other function names.

## moocore 0.1.2

CRAN release: 2024-09-18

- Fix more warnings and problems that only show in CRAN.

## moocore 0.1.1

- Fix problems that only show in CRAN.

## moocore 0.1.0

CRAN release: 2024-07-28

- Initial version uploaded to CRAN.
