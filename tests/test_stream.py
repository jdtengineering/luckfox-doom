"""Compile the actual stream backend and render-only interpolation helpers."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class StreamTests(unittest.TestCase):
    def test_motion_boundaries(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = str(Path(directory) / "motion")
            subprocess.run([os.environ.get("CC", "cc"), "-std=c99", "-Wall", "-Wextra",
                            "-I", str(ROOT / "src"), str(ROOT / "tests/motion_harness.c"),
                            "-o", binary], check=True)
            subprocess.run([binary], check=True)

    @unittest.skipUnless(sys.platform.startswith("linux"), "Linux streaming backend")
    def test_exact_framebuffer_and_palette(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = str(Path(directory) / "stream")
            subprocess.run([os.environ.get("CC", "cc"), "-std=c99", "-Wall", "-Wextra",
                            "-I", str(ROOT / "src"), str(ROOT / "tests/stream_harness.c"),
                            str(ROOT / "src/doomgeneric_stream.c"), "-o", binary], check=True)
            data = subprocess.check_output([binary], input=b"")
            palette = bytes(v for c in range(256) for v in (c, 255-c, c//2))
            expected = b"LFDOOM1\n" + struct.pack("<II", 0, 1234) + palette + bytes(range(256))*250
            self.assertEqual(data, expected)


if __name__ == "__main__": unittest.main()
