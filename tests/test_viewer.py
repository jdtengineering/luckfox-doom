"""Protocol framing and pixel fidelity, including fragmented SSH reads."""
import importlib.util
import io
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location("viewer", Path(__file__).resolve().parents[1] / "scripts/viewer.py")
viewer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(viewer)


class Fragmented(io.BytesIO):
    def read(self, size=-1):
        return super().read(min(size, 37))


class ViewerTests(unittest.TestCase):
    def test_fragmented_frames_preserve_every_palette_and_pixel_byte(self):
        palette = bytes(range(256)) * 3
        pixels = bytes(range(256)) * 250
        payload = b"startup\n" + viewer.MAGIC
        payload += struct.pack("<II", 0, 1234) + palette + pixels
        payload += struct.pack("<II", 1, 1251) + palette[::-1] + pixels[::-1]
        frames = viewer.frames(Fragmented(payload))
        self.assertEqual(next(frames), (0, 1234, palette, pixels))
        self.assertEqual(next(frames), (1, 1251, palette[::-1], pixels[::-1]))
        with self.assertRaises(EOFError): next(frames)

    def test_truncated_frame_is_not_displayed(self):
        frames = viewer.frames(io.BytesIO(viewer.MAGIC + b"\0" * (viewer.FRAME_BYTES - 1)))
        with self.assertRaises(EOFError): next(frames)

    def test_invalid_header_has_bounded_search(self):
        with self.assertRaises(ValueError):
            next(viewer.frames(io.BytesIO(b"x" * 65537)))


if __name__ == "__main__": unittest.main()
