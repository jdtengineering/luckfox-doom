# Install Doom on a Luckfox Pico Plus

This guide adds Doom to an already working Linux image. It does not flash the
board or rebuild the kernel. Commands labelled **computer** run on your PC or
build host; commands labelled **board** run inside SSH.

## 1. Check the board

You need a Luckfox Pico Plus running a compatible ARM/uClibc Buildroot image,
a writable root filesystem, about **5 MB free flash**, and SSH access with
permission to write `/opt` and `/usr/bin`. The tested account is `root`.

From your computer:

```sh
ssh root@luckfox 'uname -m; df -h /; ls /lib/libc.so.0'
```

Replace `luckfox` everywhere below with your board's actual hostname or IP.
The tested architecture is `armv7l`. Set up SSH login using your image's own
instructions before continuing; this project does not set passwords or keys.

## 2. Build on Linux or WSL

The compiler runs on an x86-64 Linux computer, not on the board. Windows users
can use Ubuntu under WSL. Ubuntu build-host prerequisites:

```sh
sudo apt update
sudo apt install git make curl openssh-client
```

Obtain the [official Luckfox SDK](https://github.com/LuckfoxTECH/luckfox-pico)
or use an existing SDK checkout. Its prebuilt compiler is under
`tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf`.
The verified SDK revision is listed below. Follow the SDK's host setup if its
compiler reports missing host libraries. There is no need to run its image build.

```sh
git clone https://github.com/jdtengineering/luckfox-doom.git
cd luckfox-doom
export TOOLCHAIN=/absolute/path/to/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf
./scripts/build-luckfox.sh
```

The result is **`_unix/game/doom-ascii`**. The script enables `_GNU_SOURCE` for
the SDK's uClibc headers and strips the binary. Use `make clean` before switching
compilers. A normal ARM/glibc compiler is not a substitute for this image's toolchain.

## 3. Get game data

Use Doom 1.9 shareware to play the first episode, or provide your own legally
obtained compatible WAD. Game data is deliberately excluded from Git.

### Windows / PowerShell

Run from your local repository checkout:

```powershell
./scripts/fetch-shareware.ps1
```

The helper downloads `_data/doom1.wad` and verifies SHA-256 before accepting it.
If PowerShell blocks local scripts, use the Linux instructions below or your
normal approved script-execution policy.

### Linux / WSL

Run from the repository checkout:

```sh
mkdir -p _data
curl -fL https://raw.githubusercontent.com/Akbar30Bill/DOOM_wads/master/doom1.wad -o _data/doom1.wad
printf '%s  %s\n' 1d7d43be501e67d927e415e0b8f3e29c3bf33075e859721816f652a526cac771 _data/doom1.wad | sha256sum -c -
```

Continue only if the checksum reports **OK**. This is the 4,196,020-byte Doom
1.9 shareware WAD, not a retail game download.

## 4. Copy to the board

Stop any running game before updating. The installer preserves an existing
configuration and saved games, and replaces the executable, launcher, and WAD.

### Linux / WSL

```sh
./scripts/deploy.sh root@luckfox ./_unix/game/doom-ascii ./_data/doom1.wad
```

### Windows / PowerShell

If you built on a separate Linux host, first copy `_unix/game/doom-ascii` to
your computer using SCP. Pass the local file location to `-Binary`:

```powershell
./scripts/deploy.ps1 -Target root@luckfox -Binary ./_unix/game/doom-ascii -Wad ./_data/doom1.wad
```

For a WSL build, you can simply run the Linux installer from that WSL checkout.
Both installers use legacy SCP mode (`-O`) for images without an SFTP subsystem.
They print the installed executable's SHA-256 and your launch command.

## 5. Play

Maximize Windows Terminal or another ANSI true-colour terminal, then run:

```sh
ssh -t root@luckfox doom
```

Choose **New Game** with the arrow keys and Enter. Space shoots, E opens doors,
Escape opens the menu, and Ctrl+C exits. Full controls are in the [README](../README.md#controls).
The launcher calculates the display size when it starts. For best detail use at
least 160 columns and 52 rows. Optional streamed audio is described below.

### Pixel version

Both installers also install the `doom-pixels` command:

```sh
ssh -t root@luckfox doom-pixels
```

Use Windows Terminal 1.22+ or another terminal supporting Sixel. Its default
640 x 400 image needs enough visible terminal space. Add `-pixel-scale 1` for
320 x 200, or `-pixel-scale 3` for 960 x 600. The original 256-colour game palette
is retained. Display output is limited to about 15 frames/s. If graphics do not
appear, use the ASCII `doom` command or switch to a Sixel-capable terminal.

## Music and sound effects

The optional audio backend mixes sound effects and synthesizes the original Doom
MUS soundtrack **on the board**. It sends 22,050 Hz, signed 16-bit little-endian
stereo PCM through `/tmp/doom-audio.pcm`. The client receives it over a separate
SSH connection and plays it with FFmpeg's `ffplay`. The synthesizer uses a General
MIDI SoundFont, so the music timbre differs from the original AdLib/OPL hardware.

### Requirements

- **Computer:** Python 3, OpenSSH, and FFmpeg including `ffplay` on PATH.
- **SSH:** key-based login must work (`ssh -o BatchMode=yes root@luckfox true`).
- **Board:** about 6 MB additional flash for instruments. In the combined test,
  roughly 16 MB RAM remained available on the 52 MB Linux system.

For Ubuntu/WSL, `sudo apt install python3 ffmpeg openssh-client` installs the
client tools. On Windows, install a full FFmpeg build that includes `ffplay`,
then verify `ffplay -version` and `python --version` in Windows Terminal.

### Install instruments once

From the repository on your computer:

```sh
python scripts/fetch-soundfont.py --target root@luckfox
```

This downloads TimGM6mb from Debian, verifies the extracted SoundFont's SHA-256,
and copies it with its copyright notice into `/opt/doom`. It requires no Debian
package manager on the board. You can provide a different compatible SoundFont
with the game's `-soundfont /path/to/file.sf2` option.

### Play with audio

```sh
# Real pixels, music and sound effects
python scripts/play.py --target root@luckfox --pixels --audio

# ASCII, music and sound effects
python scripts/play.py --target root@luckfox --audio
```

The helper starts the audio receiver, player, and interactive game. Ctrl+C exits;
child processes are cleaned up. Use the game's sound menu to adjust music/effect
volumes. Run one audio-enabled game per board at a time. Audio is supported by
this Linux backend, not by the upstream native Windows build.

The stream uses about 0.7 Mbit/s before SSH overhead. Graphics and audio travel
on separate connections and do not have a shared playback clock; network stalls
can produce latency or gaps. Sixel presents each rendered game frame; the
achieved rate depends on the scene, SSH connection, and terminal. This is a small embedded board, not a low-latency
gaming system. The implemented music reader supports Doom's MUS format; MIDI
music in custom WADs is not currently supported.

### Audio verification

Tests cover MUS event timing, invalid/truncated tracks, synthesizer output and
looping, pause, stereo panning, and PCM sample values. A live combined Sixel/audio
session produced non-silent stereo PCM; `ffplay` 8.1 successfully played it on
Windows. Startup, Ctrl+C, and SSH disconnect cleanup were checked. TimGM6mb:

```text
c5378b62028c920cb11e4803327983fee2f2cdff5dc89c708e39da417e51c854
```

## Troubleshooting

| Symptom | What to check |
| --- | --- |
| Hostname not found | Use the board's IP address instead of `luckfox`. |
| `Permission denied` from SSH | Verify your SSH account, password/key, and board image settings. |
| `Run with a terminal` or `tcgetattr` error | Include `ssh -t`; the game requires an interactive terminal. |
| Executable exists but reports `not found` | Check the ELF loader and libc: rebuild with the matching Luckfox toolchain. |
| `Exec format error` | You copied a host binary instead of the ARM build. |
| Display wraps or clips | Enlarge the terminal and restart the game. |
| Colours look wrong | Use a terminal with ANSI true-colour support and a dark background. |
| SCP reports a subsystem error | Use the supplied installer or `scp -O`. |
| Terminal stays scrambled after disconnection | Run `stty sane` in your local Unix shell, or reopen the terminal tab. |
| No audio | Use the Python helper with `--audio`; confirm `ffplay` is on PATH and SSH key login works. |
| Effects but no music | Install `TimGM6mb.sf2` with the helper and check the music volume. |
| Not enough storage | Check `df -h /`; the binary plus shareware data need about 5 MB. |

## Verified build

Validated on 2026-09-23 with Luckfox Pico Plus, Buildroot 2023.02.6, Linux
5.10.160, ARMv7 hard-float and uClibc 1.0.31. SDK revision:
`824b817f889c2cbff1d48fcdb18ab494a68f69d1`, compiler GCC 8.3.

The initial ASCII-only executable was **399,244 bytes**, dynamically needed only
`libc.so.0`, and had this SHA-256 (the current build also includes Sixel):

```text
066722fa0b32d70f041ae93f95e5576fb0d9dd3c8d23907031480b5fae7318f9
```

Rebuild hashes can vary with compiler versions and source changes. Functional
checks used actual SSH terminals at 160 x 52 and 80 x 25, loaded E1M1, captured
colour frames, and verified Ctrl+C cleanup. The README image is a PNG rendering
of a complete frame received from the board's SSH terminal, not a local PC game.

## Renderer regression test

### Performance measurements

On a Pico Plus, a 22-second E1M1 session with music and effects improved from
approximately 5.4 to 21.2 received Sixel frames/s after buffering the encoder,
encoding horizontal runs at native width, avoiding the unused RGB copy, and
removing the extra 15 Hz frame limiter. This measures SSH reception, not the
Windows display refresh rate; other scenes and terminals can differ.
The encoder preserves the output bytes at each pixel scale. The tests also
cover solid colours and short/long runs as well as a 256-colour pattern.

The image exposes 408, 600, 816, and 1104 MHz CPU settings. Holding 1104 MHz
with the performance governor gave only a small gain in the intermediate
build (11.8 to 12.3 frames/s); the board was returned to `ondemand`.
Short gameplay tests peaked at 46.4 degrees C, below this image's first
75-degree thermal trip. These are short measurements, not a thermal soak test.
No voltage, clock-table, or thermal-protection changes were made.

### Running the tests

On a Linux computer with a C compiler and Python 3:

```sh
python3 tests/test_sixel.py
python3 tests/test_audio.py
# Also exercise synthesis and looping with downloaded instruments:
python3 tests/test_audio.py _data/TimGM6mb.sf2
```

This compiles the real encoder, decodes its Sixel output, and checks every pixel
and palette entry at all three scales, including invalid-scale fallback.
