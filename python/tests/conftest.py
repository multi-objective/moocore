# ruff: noqa: D100, D101, D102, D103
import pytest
import os
import moocore


@pytest.fixture(autouse=True, scope="module")
def test_datapath(request):
    """Return the directory of the currently running test script."""

    def _(file_path: str):
        filename = moocore.get_dataset_path(file_path)
        if os.path.isfile(filename):
            return filename
        return request.path.parent.joinpath("test_data/" + file_path)

    return _
