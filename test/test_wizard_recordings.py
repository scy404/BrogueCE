"""Check the full wizard recording and truncated creation payloads via real playback."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path, help="Path to brogue or bin/brogue.exe")
    args = parser.parse_args()
    executable = args.executable.resolve()
    fixture = Path(__file__).parent / "recordings/wizard-creation/creation-and-resume.broguerec"
    data = fixture.read_bytes()
    # Header (36), initial RNG check (2), gold command (1), item command (7),
    # monster command (8). Keep the header's original length when truncating.
    assert data[38] == 9 and data[39] == 10 and data[46] == 11, "Update fixture offsets"
    cases = [("complete", data, 0)]
    cases += [("missing-gold-command", data[:38], 2)]
    cases += [(f"item-payload-{n}", data[:40 + n], 2) for n in range(6)]
    cases += [(f"monster-payload-{n}", data[:47 + n], 2) for n in range(7)]
    with tempfile.TemporaryDirectory(prefix="brogue-wizard-") as directory:
        for name, contents, expected in cases:
            recording = Path(directory) / f"{name}.broguerec"
            recording.write_bytes(contents)
            result = subprocess.run(
                [str(executable), "-vn", str(recording)],
                cwd=directory, capture_output=True, text=True, timeout=15,
            )
            if result.returncode != expected:
                raise AssertionError(
                    f"{name}: expected exit {expected}, got {result.returncode}\n"
                    f"{result.stdout}\n{result.stderr}"
                )
            print(f"{name}: PASS (exit {expected})")


if __name__ == "__main__":
    main()
