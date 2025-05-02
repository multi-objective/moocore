# ruff: noqa: D104
from ._moocore import (
    ReadDatasetsError,
    apply_within_sets,
    avg_hausdorff_dist,
    eaf,
    eafdiff,
    epsilon_additive,
    epsilon_mult,
    filter_dominated,
    filter_dominated_within_sets,
    hypervolume,
    Hypervolume,
    RelativeHypervolume,
    hv_approx,
    igd,
    igd_plus,
    is_nondominated,
    is_nondominated_within_sets,
    largest_eafdiff,
    normalise,
    pareto_rank,
    read_datasets,
    vorob_dev,
    vorob_t,
    whv_hype,
    whv_rect,
    total_whv_rect,
)

from ._datasets import (
    get_dataset,
    get_dataset_path,
)

from importlib.metadata import version as _metadata_version

__version__ = _metadata_version("moocore")
# Remove symbols imported for internal use
del _metadata_version


__all__ = [
    "Hypervolume",
    "ReadDatasetsError",
    "RelativeHypervolume",
    "apply_within_sets",
    "avg_hausdorff_dist",
    "eaf",
    "eafdiff",
    "epsilon_additive",
    "epsilon_mult",
    "filter_dominated",
    "filter_dominated_within_sets",
    "get_dataset",
    "get_dataset_path",
    "hv_approx",
    "hypervolume",
    "igd",
    "igd_plus",
    "is_nondominated",
    "is_nondominated_within_sets",
    "largest_eafdiff",
    "normalise",
    "pareto_rank",
    "read_datasets",
    "total_whv_rect",
    "vorob_dev",
    "vorob_t",
    "whv_hype",
    "whv_rect",
]
