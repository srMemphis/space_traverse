"""Run the smoke scenario with a given space_traverse.exe.

Usage: py tools/smoke/run.py <path/to/space_traverse.exe> <label>

Copies space_traverse/configs/smoke.yaml over space_traverse/config.yaml,
runs the exe from space_traverse/, moves space_traverse/out/smoke to
space_traverse/out/smoke_<label> and restores config.yaml afterwards.
Compare two runs with tools/smoke/compare.py.
"""

import shutil
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
APP_DIR = REPO / "space_traverse"
CONFIG = APP_DIR / "config.yaml"
SMOKE_CONFIG = APP_DIR / "configs" / "smoke.yaml"
OUT_DIR = APP_DIR / "out" / "smoke"


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__.strip(), file=sys.stderr)
        return 2

    exe = Path(sys.argv[1]).resolve()
    label = sys.argv[2]
    target = APP_DIR / "out" / f"smoke_{label}"

    if not exe.is_file():
        print(f"exe not found: {exe}", file=sys.stderr)
        return 2

    # Multithreaded mode: two Release runs of smoke.yaml match bit for bit
    # with OpenMP's default thread count (checked in issue #19), so
    # OMP_NUM_THREADS is left as is.
    original_config = CONFIG.read_bytes()
    try:
        shutil.copyfile(SMOKE_CONFIG, CONFIG)
        start = time.perf_counter()
        # The exe mirrors its log to the console; space_traverse/simulog.log keeps it.
        result = subprocess.run([str(exe)], cwd=APP_DIR, stdout=subprocess.DEVNULL)
        elapsed = time.perf_counter() - start
    finally:
        CONFIG.write_bytes(original_config)

    print(f"run time: {elapsed:.1f} s")
    if result.returncode != 0:
        print(f"exe exited with code {result.returncode}", file=sys.stderr)
        return result.returncode

    if target.exists():
        shutil.rmtree(target)
    shutil.move(OUT_DIR, target)
    print(f"output: {target}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
