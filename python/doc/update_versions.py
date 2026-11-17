"""Update versions.json for PyData Sphinx Theme version switcher."""

import json
import os
import subprocess
import sys
from pathlib import Path

BASE = "https" + "://multi-objective.github.io/moocore/python/"


def load_existing_versions() -> list[dict]:
    """Load existing version entries from gh-pages."""
    result = subprocess.run(
        ["git", "show", "origin/gh-pages:python/versions.json"],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0 or not result.stdout.strip():
        return []
    data = json.loads(result.stdout)
    return data if isinstance(data, list) else []


def update_versions(version: str) -> None:
    """Update versions.json with the given documentation version."""
    data = load_existing_versions()
    if version == "dev":
        entry = {"name": "dev", "version": "dev", "url": BASE + "dev/"}
    else:
        entry = {
            "version": version,
            "url": f"{BASE}{version}/",
            "preferred": True,
        }
        for item in data:
            if isinstance(item, dict):
                item.pop("preferred", None)

    updated = [
        item
        for item in data
        if not (isinstance(item, dict) and item.get("version") == version)
    ]
    updated.append(entry)

    out_dir = Path("versions_meta")
    out_dir.mkdir(parents=True, exist_ok=True)
    with (out_dir / "versions.json").open("w", encoding="utf-8") as f:
        json.dump(updated, f, indent=2)
        f.write("\n")


def main() -> None:
    """Parse the target version and update versions.json."""
    version = sys.argv[1] if len(sys.argv) > 1 else os.environ["VERSION"]
    update_versions(version)


if __name__ == "__main__":
    main()
