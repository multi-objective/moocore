# ruff: noqa: D100, D101, D102, D103
import pytest
import moocore


@pytest.fixture(autouse=True)
def add_doctest_imports(doctest_namespace) -> None:
    doctest_namespace["moocore"] = moocore
