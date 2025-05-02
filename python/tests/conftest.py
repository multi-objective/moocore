# ruff: noqa: D100, D101, D102, D103
import pytest
import moocore


@pytest.fixture(autouse=True, scope="module")
def test_datapath(request):
    """Return the directory of the currently running test script."""

    def _(file_path: str):
        filename = request.path.parent / "test_data" / file_path
        if filename.is_file():
            return filename
        return moocore.get_dataset_path(file_path)

    return _
