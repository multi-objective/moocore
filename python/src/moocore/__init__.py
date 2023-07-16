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
    hypervolume,
    igd,
    igd_plus,
    is_nondominated,
    pareto_rank,
    normalise,
    read_datasets,
    vorobDev,
    vorobT,
    get_dataset_path,
    groupby,
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
    "hypervolume",
    "igd",
    "igd_plus",
    "is_nondominated",
    "pareto_rank",
    "normalise",
    "read_datasets",
    "vorobDev",
    "vorobT",
    "get_dataset_path",
    "groupby",
]
