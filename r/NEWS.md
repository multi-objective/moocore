# moocore 0.1.7.900

 * Document the EAF and Vorob'ev expectation and deviation in more detail.
 * New function `hv_approx()`.

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
