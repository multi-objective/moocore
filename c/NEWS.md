# moocore C library

## 0.17.0

 * `hv_contributions()` gains a parameter `ignore_dominated`.  The 3D case uses the HVC3D algorithm implemented in `hvc3d()`.
 * In the output of `hv --contributions`, dominated points do not affect the
   contribution of nondominated points.
 * `find_dominated_point_()` and `find_nondominated_set_()` are significantly faster due to several code optimizations.
 * `pareto_rank()` returns 0-based ranks.
 * `find_dominated_point_()` and `find_nondominated_set_()` are now stable in 2D and 3D with `!keep_weakly`, that is, only the first of duplicated points is marked as nondominated.

## 0.16.6

 * main-hvapprox.c, hv_approx.c: New.
 * The license of files copyrighted by Manuel López-Ibáñez, Carlos M. Fonseca, Luís Paquete, Andreia P. Guerreiro and Leonardo C.T. Bezerra is now MPL v2.0.
 * Add option `--contributions` to `hv` to compute exclusive hypervolume
   contributions.

## 0.16.5

 * The base case of the recursive HV algorithm is HV4D+.
 * Remove the `--shift` option of `hv` executable.

## 0.16.4

 * Implementation of HV4D+ algorithm.

## 0.16.3

 * Implementation of HV3D+ algorithm.

## 0.16.2

 * Faster reading of datasets.
