"""Run audio regression tests; optionally pass a SoundFont filename."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as directory:
    binary = str(Path(directory) / "audio-test")
    subprocess.run([os.environ.get("CC", "cc"), "-std=c99", "-D_GNU_SOURCE", "-O2",
                    "-I", str(root / "src"), str(root / "tests/audio_harness.c"),
                    "-lm", "-o", binary], check=True)
    subprocess.run([binary] + sys.argv[1:], check=True)
