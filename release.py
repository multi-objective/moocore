#!/usr/bin/env python3

import re
from datetime import datetime
import argparse


def print_status(prefix, total_width=40):
    dots_needed = total_width - len(prefix) - len("DONE")
    dots = "." * max(dots_needed, 0)
    print(f"{prefix} {dots} DONE")


def update_news_md(file_path, version):
    pattern = re.compile(r"^(#\s+[a-z]+)\s+[0-9.]+")
    replacement = rf"\1 {version}"

    with open(file_path, "r") as file:
        content = file.read()

    if not pattern.search(content):
        raise ValueError(f'"No match found in {file_path} .')

    new_content = pattern.sub(replacement, content, count=1)

    with open(file_path, "w") as file:
        file.write(new_content)

    print_status(file_path)


def replace_dev_with_date(file_path, version):
    today_str = datetime.now().strftime("%d/%m/%Y")
    pattern = re.compile(r"Version\s+[0-9.]+\s+\([^)]+\)")
    replacement = rf"Version {version} ({today_str})"

    with open(file_path, "r") as file:
        content = file.read()

    if not pattern.search(content):
        raise ValueError(f'"Version (dev)" not found in {file_path} .')

    new_content = pattern.sub(replacement, content, count=1)

    with open(file_path, "w") as file:
        file.write(new_content)

    print_status(file_path)


def update_package_version(file_path, search, remove="", replace="", append=""):
    pattern = re.compile(rf'^({search}\s*[=:]\s*)(["\']?)(.*?)(["\']?)\s*$')

    with open(file_path, "r") as file:
        lines = file.readlines()

    updated_lines = []
    found = False

    for line in lines:
        m = pattern.match(line)
        if m:
            prefix, quote_start, value, quote_end = m.groups()
            if remove:
                new_value = value.replace(remove, "").strip()
            elif replace:
                new_value = replace
            elif append:
                new_value = value + append
            new_line = f"{prefix}{quote_start}{new_value}{quote_end}\n"
            updated_lines.append(new_line)
            found = True
        else:
            updated_lines.append(line)

    if not found:
        raise ValueError(f"{search} not found in {file_path}.")

    with open(file_path, "w") as file:
        file.writelines(updated_lines)

    print_status(file_path)


def get_package_version(file_path):
    with open(file_path, "r") as file:
        for line in file:
            if line.startswith("PACKAGEVERSION"):
                _, version_str = line.split("=", 1)
                return version_str.strip().strip("'\"")
    raise ValueError("PACKAGEVERSION not found in the file.")


def main():
    parser = argparse.ArgumentParser(description="Release helper script")
    parser.add_argument(
        "--dev", type=str, help="Switch to development version given as argument"
    )
    args = parser.parse_args()
    version = get_package_version("Makefile")
    if args.dev is not None:
        print(f"Current version is: {version}")
        print(f"Development version set to: {args.dev}")
        if version == args.dev:
            raise ValueError(
                f"Development version should higher than current one ({version})"
            )
        update_package_version("Makefile", "PACKAGEVERSION", replace=args.dev)
        update_package_version("c/Makefile", "VERSION", append="$(REVISION)")
        update_package_version("r/DESCRIPTION", "Version", replace=version + ".900")
        update_package_version(
            "python/pyproject.toml", "version", replace=args.dev + ".dev0"
        )
        print("*** If happy with the changes, then do:")
        print(f"git ci -a -m 'Start development of v{version}'")
    else:
        print(f"Current version is: {version}")
        print("Preparing for release")
        update_package_version("c/Makefile", "VERSION", remove="$(REVISION)")
        update_package_version("r/DESCRIPTION", "Version", replace=version)
        update_package_version("python/pyproject.toml", "version", replace=version)
        replace_dev_with_date("python/doc/source/whatsnew/index.rst", version)
        update_news_md("r/NEWS.md", version)
        print("*** If happy with the changes, then do:")
        print(f"git ci -a -m 'Prepare to release v{version}'")


if __name__ == "__main__":
    main()
