# ruff: noqa: D100
import pytest
import moocore
from .helpers.immutability import deep_copy, deep_equal, freeze_numpy


@pytest.fixture(autouse=True, scope="module")
def test_datapath(request):
    """Return the directory of the currently running test script."""

    def _(file_path: str):
        filename = request.path.parent / "test_data" / file_path
        if filename.is_file():
            return filename
        return moocore.get_dataset_path(file_path)

    return _


@pytest.fixture
def immutable_call():
    """
    Call a function and assert that selected arguments remain immutable.

    Usage:
        immutable_call(fn, *args, **kwargs, immutable=[0, "y"], freeze=True)
    """

    def _call(fn, *args, immutable=None, freeze=True, **kwargs):
        # If immutable is None, then all positional and keyword args are
        # immutable.
        if immutable is None:
            immutable = set(list(range(len(args))) + list(kwargs.keys()))
        else:
            immutable = set(immutable)

        args_copy = [deep_copy(a) for a in args]
        kwargs_copy = {k: deep_copy(v) for k, v in kwargs.items()}

        # Freeze mutable args if needed
        if freeze:
            for idx, a in enumerate(args):
                if idx in immutable:
                    freeze_numpy(a)
            for k, v in kwargs.items():
                if k in immutable:
                    freeze_numpy(v)

        # Run the function — any exception is a failure
        try:
            result = fn(*args, **kwargs)
        except Exception as e:
            raise AssertionError(
                f"Function '{fn.__name__}' raised an unexpected exception"
                f"under immutability checking: {type(e).__name__}: {e}"
            ) from e

        # Normal immutability checks (no freeze error triggered)
        for idx, (orig, copied) in enumerate(zip(args, args_copy)):
            if idx in immutable:
                assert deep_equal(orig, copied), (
                    f"Function '{fn.__name__}' mutated positional arg {idx}"
                )

        for key in kwargs:
            if key in immutable:
                assert deep_equal(kwargs[key], kwargs_copy[key]), (
                    f"Function '{fn.__name__}' mutated keyword arg '{key}'"
                )

        return result

    return _call
