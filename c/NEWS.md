# moocore C library

## 0.17.0

 * `hv_contributions()` gains a parameter `ignore_dominated`.
 * In the output of `hv --contributions`, dominated points do not affect the
   contribution of nondominated points.

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
