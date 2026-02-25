# Package index

## Pareto dominance

- [`is_nondominated()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md)
  [`filter_dominated()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md)
  [`any_dominated()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md)
  [`pareto_rank()`](https://multi-objective.github.io/moocore/r/reference/nondominated.md)
  : Identify, remove and rank dominated points according to Pareto
  optimality

## Multi-objective performance assessment metrics

- [`epsilon_additive()`](https://multi-objective.github.io/moocore/r/reference/epsilon.md)
  [`epsilon_mult()`](https://multi-objective.github.io/moocore/r/reference/epsilon.md)
  : Epsilon metric
- [`hv_approx()`](https://multi-objective.github.io/moocore/r/reference/hv_approx.md)
  : Approximate the hypervolume indicator.
- [`hv_contributions()`](https://multi-objective.github.io/moocore/r/reference/hv_contributions.md)
  : Hypervolume contribution of a set of points
- [`hypervolume()`](https://multi-objective.github.io/moocore/r/reference/hypervolume.md)
  : Hypervolume metric
- [`igd()`](https://multi-objective.github.io/moocore/r/reference/igd.md)
  [`igd_plus()`](https://multi-objective.github.io/moocore/r/reference/igd.md)
  [`avg_hausdorff_dist()`](https://multi-objective.github.io/moocore/r/reference/igd.md)
  : Inverted Generational Distance (IGD and IGD+) and Averaged Hausdorff
  Distance
- [`r2_exact()`](https://multi-objective.github.io/moocore/r/reference/r2_exact.md)
  : Exact R2 indicator
- [`whv_hype()`](https://multi-objective.github.io/moocore/r/reference/whv_hype.md)
  : Approximation of the (weighted) hypervolume by Monte-Carlo sampling
  (2D only)
- [`whv_rect()`](https://multi-objective.github.io/moocore/r/reference/whv_rect.md)
  [`total_whv_rect()`](https://multi-objective.github.io/moocore/r/reference/whv_rect.md)
  : Compute (total) weighted hypervolume given a set of rectangles

## Computing the Empirical Attainment Function

- [`vorob_t()`](https://multi-objective.github.io/moocore/r/reference/Vorob.md)
  [`vorob_dev()`](https://multi-objective.github.io/moocore/r/reference/Vorob.md)
  : Vorob'ev threshold, expectation and deviation

- [`attsurf2df()`](https://multi-objective.github.io/moocore/r/reference/attsurf2df.md)
  :

  Convert a list of attainment surfaces to a single EAF `data.frame`.

- [`choose_eafdiff()`](https://multi-objective.github.io/moocore/r/reference/choose_eafdiff.md)
  : Interactively choose according to empirical attainment function
  differences

- [`compute_eaf_call()`](https://multi-objective.github.io/moocore/r/reference/compute_eaf_call.md)
  :

  Same as
  [`eaf()`](https://multi-objective.github.io/moocore/r/reference/eaf.md)
  but performs no checks and does not transform the input or the output.
  This function should be used by other packages that want to avoid
  redundant checks and transformations.

- [`compute_eafdiff_call()`](https://multi-objective.github.io/moocore/r/reference/compute_eafdiff_call.md)
  :

  Same as
  [`eafdiff()`](https://multi-objective.github.io/moocore/r/reference/eafdiff.md)
  but performs no checks and does not transform the input or the output.
  This function should be used by other packages that want to avoid
  redundant checks and transformations.

- [`eaf()`](https://multi-objective.github.io/moocore/r/reference/eaf.md)
  : Exact computation of the Empirical Attainment Function (EAF)

- [`eaf_as_list()`](https://multi-objective.github.io/moocore/r/reference/eaf_as_list.md)
  :

  Convert an EAF data frame to a list of data frames, where each element
  of the list is one attainment surface. The function
  [`attsurf2df()`](https://multi-objective.github.io/moocore/r/reference/attsurf2df.md)
  can be used to convert the list into a single data frame.

- [`eafdiff()`](https://multi-objective.github.io/moocore/r/reference/eafdiff.md)
  : Compute empirical attainment function differences

- [`largest_eafdiff()`](https://multi-objective.github.io/moocore/r/reference/largest_eafdiff.md)
  : Identify largest EAF differences

## Read / Write / Transform / Generate datasets

- [`read_datasets()`](https://multi-objective.github.io/moocore/r/reference/read_datasets.md)
  : Read several data sets

- [`write_datasets()`](https://multi-objective.github.io/moocore/r/reference/write_datasets.md)
  : Write data sets

- [`normalise()`](https://multi-objective.github.io/moocore/r/reference/normalise.md)
  : Normalise points

- [`rbind_datasets()`](https://multi-objective.github.io/moocore/r/reference/rbind_datasets.md)
  :

  Combine datasets `x` and `y` by row taking care of making all sets
  unique.

- [`transform_maximise()`](https://multi-objective.github.io/moocore/r/reference/transform_maximise.md)
  : Transform matrix according to maximise parameter

- [`as_double_matrix()`](https://multi-objective.github.io/moocore/r/reference/as_double_matrix.md)
  :

  Convert input to a matrix with `"double"` storage mode
  ([`base::storage.mode()`](https://rdrr.io/r/base/mode.html)).

- [`generate_ndset()`](https://multi-objective.github.io/moocore/r/reference/generate_ndset.md)
  : Generate a random set of mutually nondominated points

## Datasets

- [`CPFs`](https://multi-objective.github.io/moocore/r/reference/CPFs.md)
  : Conditional Pareto fronts obtained from Gaussian processes
  simulations.
- [`HybridGA`](https://multi-objective.github.io/moocore/r/reference/HybridGA.md)
  : Results of Hybrid GA on Vanzyl and Richmond water networks
- [`SPEA2minstoptimeRichmond`](https://multi-objective.github.io/moocore/r/reference/SPEA2minstoptimeRichmond.md)
  : Results of SPEA2 when minimising electrical cost and maximising the
  minimum idle time of pumps on Richmond water network.
- [`SPEA2relativeRichmond`](https://multi-objective.github.io/moocore/r/reference/SPEA2relativeRichmond.md)
  : Results of SPEA2 with relative time-controlled triggers on Richmond
  water network.
- [`SPEA2relativeVanzyl`](https://multi-objective.github.io/moocore/r/reference/SPEA2relativeVanzyl.md)
  : Results of SPEA2 with relative time-controlled triggers on Vanzyl's
  water network.
- [`tpls50x20_1_MWT`](https://multi-objective.github.io/moocore/r/reference/tpls50x20_1_MWT.md)
  : Various strategies of Two-Phase Local Search applied to the
  Permutation Flowshop Problem with Makespan and Weighted Tardiness
  objectives.
