# ruff: noqa: D100, D103
import pytest
import moocore


@pytest.fixture(autouse=True)
def add_namespace(doctest_namespace):
    doctest_namespace["moocore"] = moocore
