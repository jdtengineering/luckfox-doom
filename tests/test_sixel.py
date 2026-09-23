"""Compile the real renderer; decode its output and compare every source pixel.
Run on a Linux build host: python3 tests/test_sixel.py
"""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class SixelTests(unittest.TestCase):
    def test_roundtrip(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = str(Path(directory) / "sixel-test")
            subprocess.run([os.environ.get("CC", "cc"), "-O2", "-std=c99", "-Wall", "-Wextra",
                            "-I", str(ROOT / "src"), str(ROOT / "tests/sixel_harness.c"),
                            str(ROOT / "src/doomgeneric_sixel.c"), "-o", binary], check=True)
            cases = [(a, s, p) for a, s in [(1, 1), (2, 2), (3, 3), (0, 2), (4, 2)]
                     for p in range(3)]
            for argument, scale, pattern in cases:
                with self.subTest(argument=argument, pattern=pattern):
                    stream = subprocess.check_output([binary, "-pixel-scale", str(argument), str(pattern)])
                    self.assertTrue(stream.startswith(b"\x1b[2J\x1b[H\x1bP0;0;0q"))
                    self.assertTrue(stream.endswith(b"\x1b\\"))
                    body = stream[len(b"\x1b[2J\x1b[H\x1bP0;0;0q"):-2].decode("ascii")
                    width, height = 320 * scale, 200 * scale
                    output = bytearray(width * height)
                    coverage = bytearray(width * height)
                    palette = {}
                    x = y = colour = i = 0
                    while i < len(body):
                        char = body[i]; i += 1
                        if char in '#"':
                            match = re.match(r"[0-9;]+", body[i:])
                            values = list(map(int, match[0].split(";")))
                            i += len(match[0])
                            if char == '"':
                                self.assertEqual(values, [1, 1, width, height])
                            else:
                                colour = values[0]
                                if len(values) > 1: palette[colour] = values[2:]
                        elif char == "$": x = 0
                        elif char == "-": x = 0; y += 6
                        else:
                            count = 1
                            if char == "!":
                                match = re.match(r"\d+", body[i:])
                                count = int(match[0]); i += len(match[0])
                                char = body[i]; i += 1
                            bits = ord(char) - 63
                            self.assertTrue(0 <= bits < 64)
                            self.assertLessEqual(x + count, width)
                            for bit in range(6):
                                if bits & (1 << bit):
                                    self.assertLess(y + bit, height)
                                    offset = (y + bit) * width + x
                                    output[offset:offset+count] = bytes([colour]) * count
                                    coverage[offset:offset+count] = b"\1" * count
                            x += count
                    expected = bytes(37 if pattern == 1 else
                                     ((x // scale // 7 + y // scale // 3) & 255) if pattern == 2 else
                                     ((x // scale) * 17 + (y // scale) * 31) & 255
                                     for y in range(height) for x in range(width))
                    self.assertEqual(coverage.count(0), 0, "Unpainted pixels")
                    self.assertEqual(output, expected)
                    self.assertEqual(len(palette), 256)
                    for c in range(256):
                        self.assertEqual(palette[c], [(v * 100 + 127) // 255
                                                     for v in (c, 255-c, c//2)])

if __name__ == "__main__": unittest.main()
