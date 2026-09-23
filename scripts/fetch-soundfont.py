#!/usr/bin/env python3
"""Download and verify TimGM6mb from Debian; optionally copy it to the board."""
import argparse
import hashlib
import io
from pathlib import Path
import subprocess
import tarfile
import urllib.request

URL = "https://deb.debian.org/debian/pool/main/t/timgm6mb-soundfont/timgm6mb-soundfont_1.3-5_all.deb"
SHA256 = "c5378b62028c920cb11e4803327983fee2f2cdff5dc89c708e39da417e51c854"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", help="Optional SSH target, for example root@luckfox")
    parser.add_argument("--output", type=Path, default=Path(__file__).resolve().parents[1] / "_data")
    args = parser.parse_args()
    if args.target and args.target.startswith("-"): parser.error("Invalid SSH target")
    with urllib.request.urlopen(URL, timeout=60) as response:
        package = response.read()
    if not package.startswith(b"!<arch>\n"): raise ValueError("Not a Debian ar archive")
    position = 8
    archive = None
    while position + 60 <= len(package):
        header = package[position:position+60]
        length = int(header[48:58].strip())
        name = header[:16].decode("ascii").strip().rstrip("/")
        position += 60
        if name.startswith("data.tar"):
            archive = package[position:position+length]
            break
        position += length + length % 2
    if archive is None: raise ValueError("Missing Debian data archive")
    with tarfile.open(fileobj=io.BytesIO(archive), mode="r:*") as contents:
        soundfont = contents.extractfile("./usr/share/sounds/sf2/TimGM6mb.sf2").read()
        license_text = contents.extractfile("./usr/share/doc/timgm6mb-soundfont/copyright").read()
    if hashlib.sha256(soundfont).hexdigest() != SHA256:
        raise ValueError("SoundFont checksum mismatch")
    args.output.mkdir(parents=True, exist_ok=True)
    sf = args.output / "TimGM6mb.sf2"
    license_file = args.output / "TimGM6mb.LICENSE.txt"
    sf.write_bytes(soundfont); license_file.write_bytes(license_text)
    print(f"Verified {sf} ({len(soundfont)} bytes)")
    if args.target:
        subprocess.run(["scp", "-O", str(sf), str(license_file), args.target + ":/opt/doom/"], check=True)
        print("Installed music instruments on the board")


if __name__ == "__main__": main()
