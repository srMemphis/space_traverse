"""Compare the .mat files of two smoke runs.

Usage: py tools/smoke/compare.py <run_dir_a> <run_dir_b>

Both directories must hold the same set of .mat files. Each pair is compared
byte for byte from byte 117 on: the first 116 bytes are the MAT-file text
header, where TinyMAT writes the creation time, so they differ between any
two runs. Exits with 0 when everything matches.
"""

import sys
from pathlib import Path

HEADER_SIZE = 116


def first_difference(a: bytes, b: bytes) -> int | None:
    """Offset of the first differing byte past the header, or None."""
    a, b = a[HEADER_SIZE:], b[HEADER_SIZE:]
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            return HEADER_SIZE + i
    if len(a) != len(b):
        return HEADER_SIZE + min(len(a), len(b))
    return None


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__.strip(), file=sys.stderr)
        return 2

    dir_a, dir_b = Path(sys.argv[1]), Path(sys.argv[2])
    for d in (dir_a, dir_b):
        if not d.is_dir():
            print(f"not a directory: {d}", file=sys.stderr)
            return 2

    names_a = {p.name for p in dir_a.glob("*.mat")}
    names_b = {p.name for p in dir_b.glob("*.mat")}
    ok = True

    for name in sorted(names_a - names_b):
        print(f"only in {dir_a}: {name}")
        ok = False
    for name in sorted(names_b - names_a):
        print(f"only in {dir_b}: {name}")
        ok = False

    common = sorted(names_a & names_b)
    for name in common:
        offset = first_difference((dir_a / name).read_bytes(), (dir_b / name).read_bytes())
        if offset is not None:
            print(f"differs: {name} at byte offset {offset}")
            ok = False

    if ok:
        print(f"match: {len(common)} .mat files")
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(main())
