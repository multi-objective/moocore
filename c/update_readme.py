from pathlib import Path
import subprocess


def replace_between_markers(
    filename: str,
    command: list[str],
) -> None:
    exe_name = Path(command[0]).name
    marker = exe_name + " " + " ".join(command[1:])
    begin = f"<!-- BEGIN {marker} -->"
    end = f"<!-- END {marker} -->"
    path = Path(filename)
    text = path.read_text()
    lines = text.splitlines(keepends=True)

    begin_indexes = [i for i, line in enumerate(lines) if line.strip() == begin]
    end_indexes = [i for i, line in enumerate(lines) if line.strip() == end]
    if len(begin_indexes) != 1:
        raise ValueError(f"Expected exactly one BEGIN marker: {begin!r}")
    if len(end_indexes) != 1:
        raise ValueError(f"Expected exactly one END marker: {end!r}")

    begin_index = begin_indexes[0]
    end_index = end_indexes[0]
    if begin_index >= end_index:
        raise ValueError("BEGIN marker must appear before END marker")

    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
        check=True,
    )
    # Whitespace cleanup.
    generated_lines = [line.rstrip() for line in result.stdout.splitlines()]
    while generated_lines and not generated_lines[0]:
        generated_lines.pop(0)
    while generated_lines and not generated_lines[-1]:
        generated_lines.pop()

    # Add pre-formatted markers.
    generated_lines = ["```"] + generated_lines + ["```"]
    # Add newline back.
    newline = "\r\n" if "\r\n" in text else "\n"
    generated_lines = [line + newline for line in generated_lines]
    lines[begin_index + 1 : end_index] = generated_lines
    path.write_text("".join(lines))


def main():
    bin_path = "../bin/"
    for cmd in [
        "nondominated",
        "dominatedsets",
        "igd",
        "epsilon",
        "hvapprox",
        "eaf",
        "ndsort",
    ]:
        replace_between_markers("README.md", [bin_path + cmd, "--help"])


if __name__ == "__main__":
    main()
