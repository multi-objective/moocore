# ruff: noqa: D104
from ._moocore import (
    Hypervolume,
    ReadDatasetsError,
    RelativeHypervolume,
    any_dominated,
    apply_within_sets,
    avg_hausdorff_dist,
    eaf,
    eafdiff,
    epsilon_additive,
    epsilon_mult,
    r2_exact,
    filter_dominated,
    filter_dominated_within_sets,
    hv_approx,
    hv_contributions,
    hypervolume,
    igd,
    igd_plus,
    is_nondominated,
    is_nondominated_within_sets,
    largest_eafdiff,
    normalise,
    pareto_rank,
    read_datasets,
    total_whv_rect,
    vorob_dev,
    vorob_t,
    whv_hype,
    whv_rect,
)

from ._datasets import (
    get_dataset,
    get_dataset_path,
)

from ._generate import generate_ndset, generate_sequence

from ._crossover import (
    blend_alpha_crossover,
    bounded_exponential_crossover,
    de_current_1_crossover,
    de_rand_1_bin_crossover,
    local_crossover,
    parent_centric_crossover,
    single_arithmetic_crossover,
    single_point_crossover,
    uniform_crossover,
)

from importlib.metadata import version as _metadata_version

__version__ = _metadata_version("moocore")
# Remove symbols imported for internal use
del _metadata_version


__all__ = [
    "Hypervolume",
    "ReadDatasetsError",
    "RelativeHypervolume",
    "any_dominated",
    "apply_within_sets",
    "avg_hausdorff_dist",
    "blend_alpha_crossover",
    "bounded_exponential_crossover",
    "de_current_1_crossover",
    "de_rand_1_bin_crossover",
    "eaf",
    "eafdiff",
    "epsilon_additive",
    "epsilon_mult",
    "filter_dominated",
    "filter_dominated_within_sets",
    "generate_ndset",
    "generate_sequence",
    "get_dataset",
    "get_dataset_path",
    "hv_approx",
    "hv_contributions",
    "hypervolume",
    "igd",
    "igd_plus",
    "is_nondominated",
    "is_nondominated_within_sets",
    "largest_eafdiff",
    "local_crossover",
    "normalise",
    "parent_centric_crossover",
    "pareto_rank",
    "r2_exact",
    "read_datasets",
    "single_arithmetic_crossover",
    "single_point_crossover",
    "total_whv_rect",
    "uniform_crossover",
    "vorob_dev",
    "vorob_t",
    "whv_hype",
    "whv_rect",
]
