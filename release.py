#!/usr/bin/env python3

import re
from datetime import datetime


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


def update_package_version(file_path, search, remove="", replace=""):
    pattern = re.compile(rf'^({search}\s*[=:]\s*)(["\']?)(.*?)(["\']?)\s*$')

    with open(file_path, "r") as file:
        lines = file.readlines()

    updated_lines = []
    found = False

    for line in lines:
        match = pattern.match(line)
        if match:
            prefix, quote_start, value, quote_end = match.groups()
            if remove:
                cleaned_value = value.replace(remove, "").strip()
                new_line = f"{prefix}{quote_start}{cleaned_value}{quote_end}\n"
            else:
                new_line = f"{prefix}{quote_start}{replace}{quote_end}\n"
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


if __name__ == "__main__":
    version = get_package_version("Makefile")
    print(f"{version}")
    update_package_version("c/Makefile", "VERSION", remove="$(REVISION)")
    update_package_version("r/DESCRIPTION", "Version", replace=version)
    update_package_version("python/pyproject.toml", "version", replace=version)
    replace_dev_with_date("python/doc/source/whatsnew/index.rst", version)
    update_news_md("r/NEWS.md", version)
