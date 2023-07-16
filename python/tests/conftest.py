# ruff: noqa: D100, D101, D102, D103
import pytest


@pytest.fixture(autouse=True, scope="module")
def test_datapath(request):
    """Return the directory of the currently running test script."""

    def _(file_path: str):
        return request.path.parent.joinpath("test_data/" + file_path)

    return _
