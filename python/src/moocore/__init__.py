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
    get_dataset_path,
    groupby,
    hypervolume,
    igd,
    igd_plus,
    is_nondominated,
    normalise,
    pareto_rank,
    read_datasets,
    vorobDev,
    vorobT,
    whv_hype,
)

from importlib.metadata import version as _metadata_version

__version__ = _metadata_version(__package__ or __name__)
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
    "get_dataset_path",
    "groupby",
    "hypervolume",
    "igd",
    "igd_plus",
    "is_nondominated",
    "normalise",
    "pareto_rank",
    "read_datasets",
    "vorobDev",
    "vorobT",
    "whv_hype",
]
