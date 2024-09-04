# ruff: noqa: D104
from ._moocore import (
    ReadDatasetsError,
    avg_hausdorff_dist,
    eaf,
    eafdiff,
    epsilon_additive,
    epsilon_mult,
    filter_dominated,
    filter_dominated_within_sets,
    get_dataset,
    get_dataset_path,
    groupby,
    hypervolume,
    hv_approx,
    igd,
    igd_plus,
    is_nondominated,
    normalise,
    pareto_rank,
    read_datasets,
    vorobDev,
    vorobT,
    whv_hype,
    whv_rect,
    total_whv_rect,
)

from importlib.metadata import version as _metadata_version

__version__ = _metadata_version("moocore")
# Remove symbols imported for internal use
del _metadata_version


__all__ = [
    "ReadDatasetsError",
    "avg_hausdorff_dist",
    "eaf",
    "eafdiff",
    "epsilon_additive",
    "epsilon_mult",
    "filter_dominated",
    "filter_dominated_within_sets",
    "get_dataset",
    "get_dataset_path",
    "groupby",
    "hypervolume",
    "hv_approx",
    "igd",
    "igd_plus",
    "is_nondominated",
    "normalise",
    "pareto_rank",
    "read_datasets",
    "vorobDev",
    "vorobT",
    "whv_hype",
    "whv_rect",
    "total_whv_rect",
]
