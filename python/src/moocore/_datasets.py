from __future__ import annotations

import hashlib
import numpy as np
import os
import shutil
import warnings
import time
from pathlib import Path
from tempfile import NamedTemporaryFile
from urllib.error import URLError
from urllib.request import urlretrieve

from importlib.resources import files
from importlib.metadata import version as _metadata_version
from platformdirs import user_cache_path
from ._moocore import read_datasets

_moocore_version = _metadata_version("moocore")

_BASE_URL = (
    "https://github.com/multi-objective/testsuite/raw/refs/heads/main/data/"
)

# FIXME: This could be a namedtuple so that each dataset can have its own URL.
_DATASETS_CHECKSUMS = {
    "ALG_1_dat.xz": "a51165fe69b356c45e5bb052c747e7dcb97432975b5e7a632fc94f0b59620046",
}


# FIXME: Python >=3.11 has hashlib.file_digest()
def _file_checksum(path) -> str:
    """Calculate the sha256 hash of the file at path."""
    sha256hash = hashlib.sha256()
    chunk_size = 8192
    with open(path, "rb") as f:
        while True:
            buffer = f.read(chunk_size)
            if not buffer:
                break
            sha256hash.update(buffer)
    return sha256hash.hexdigest()


def _fetch_dataset(
    name: str,
    checksum: str,
    force: bool = False,
    n_retries: int = 3,
    delay: int = 1,
) -> Path:
    """Fetch a dataset from if not already present in the local cache folder.

    If the file already exists locally and the SHA256 checksums match what is
    expected by this version of ``moocore``, the path to the local file is
    returned without re-downloading.

    Parameters
    ----------
    name :
        Name of the dataset.

    checksum :
        Expected checksum.

    force :
        If ``True``, always download the dataset. By default, the dataset is only downloaded
        if it doesn't exist locally or the checksum does not match.

    n_retries :
        Number of retries when download errors are encountered.

    delay :
        Number of seconds between retries.

    Returns
    -------
    file_path :
        Full path of the downloaded file.

    """
    url = _BASE_URL + name

    local_folder = user_cache_path(
        appname="moocore",
        appauthor=None,
        version=_moocore_version,
        opinion=True,
        ensure_exists=True,
    )

    file_path = local_folder / name
    if not force and file_path.exists():
        local_checksum = _file_checksum(file_path)
        if local_checksum == checksum:
            return file_path
        else:
            warnings.warn(
                f"The SHA256 checksum of existing local file {file_path.name} "
                f"({local_checksum}) differs from expected ({checksum}): "
                f"re-downloading from {url} ."
            )
    # We create a temporary file dedicated to this particular download to avoid
    # conflicts with parallel downloads. If the download is successful, the
    # temporary file is atomically renamed to the final file path (with
    # `shutil.move`). We therefore pass `delete=False` to `NamedTemporaryFile`.
    # Otherwise, garbage collecting temp_file would raise an error when
    # attempting to delete a file that was already renamed. If the download
    # fails or the result does not match the expected SHA256 digest, the
    # temporary file is removed manually in the except block.
    temp_file = NamedTemporaryFile(
        prefix=name + ".part_", dir=local_folder, delete=False
    )
    # Note that Python 3.12's `delete_on_close=True` is ignored as we set
    # `delete=False` explicitly. So after this line the empty temporary file still
    # exists on disk to make sure that it's uniquely reserved for this specific call of
    # `_fetch_remote` and therefore it protects against any corruption by parallel
    # calls.
    temp_file.close()
    try:
        temp_file_path = Path(temp_file.name)
        while True:
            try:
                urlretrieve(url, temp_file_path)
                break
            except (URLError, TimeoutError):
                if n_retries == 0:
                    # If no more retries are left, re-raise the caught exception.
                    raise
                warnings.warn(f"Retry downloading from url: {url}")
                n_retries -= 1
                time.sleep(delay)

        local_checksum = _file_checksum(temp_file_path)
        if local_checksum != checksum:
            message = (
                f"The SHA256 checksum of remote file {name} ({local_checksum}) "
                + f"differs from expected ({checksum}). "
                + "The remote file may not be valid for this version of moocore. "
            )
            if force:
                warnings.warn(
                    message + "Overwriting local file because 'force=True'."
                )
            else:
                raise OSError(
                    message + "Use 'force=True' to download the file."
                )

    except (Exception, KeyboardInterrupt):
        os.unlink(temp_file.name)
        raise

    # The following renaming is atomic whenever temp_file_path and
    # file_path are on the same filesystem. This should be the case most of
    # the time, but we still use shutil.move instead of os.rename in case
    # they are not.
    shutil.move(temp_file_path, file_path)

    return file_path


def get_dataset_path(
    filename: str, /, *, force: bool = False, n_retries: int = 3, delay: int = 1
) -> Path:
    """Return path to a dataset provided with ``moocore``.

    Small datasets are installed together with the ``moocore`` package.
    Large datasets are downloaded from a remote repository and stored in the
    local cache folder. If the file already exists locally (and the SHA256
    checksums match when provided), the path to the local file is returned
    without re-downloading.

    Parameters
    ----------
    filename :
        Name of the dataset.

    force :
        If ``True``, always download remote datasets, even if present or their
        checksum does not match what is expected for this version of ``moocore``.

    n_retries :
        Number of retries when download errors are encountered.

    delay :
        Number of seconds between retries when downloading.

    Returns
    -------
        Full path to the dataset.

    """
    local_path = files("moocore.data") / filename
    if local_path.exists():
        return local_path

    checksum = _DATASETS_CHECKSUMS.get(filename)
    if checksum is None:
        local_names = "\n".join(
            [f.name for f in files("moocore.data").iterdir() if f.is_file()]
        )
        remote_names = "\n".join(_DATASETS_CHECKSUMS.keys())
        raise ValueError(
            f"Unknown dataset '{filename}', local datasets are:\n"
            f"{local_names}\n\nand remote datasets are:\n{remote_names}"
        )

    return _fetch_dataset(
        filename,
        checksum=checksum,
        force=force,
        n_retries=n_retries,
        delay=delay,
    )


def get_dataset(filename: str, /, **kwargs) -> np.ndarray:
    """Return dataset provided by ``moocore`` as a NumPy array.

    Parameters
    ----------
    filename :
        Name of the dataset.

    kwargs :
        Additional arguments are passed to :func:`get_dataset_path`.

    Returns
    -------
        An array containing a representation of the data in the file.
        The first :math:`n-1` columns contain the numerical data for each of the objectives.
        The last column contains an identifier for which set the data is relevant to.

    See Also
    --------
    read_datasets : Function used to read the dataset.

    """
    return read_datasets(get_dataset_path(filename))
