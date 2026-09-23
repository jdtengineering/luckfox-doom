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
least 160 columns and 52 rows. Audio is not implemented by this port.

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
| Not enough storage | Check `df -h /`; the binary plus shareware data need about 5 MB. |

## Verified build

Validated on 2026-09-23 with Luckfox Pico Plus, Buildroot 2023.02.6, Linux
5.10.160, ARMv7 hard-float and uClibc 1.0.31. SDK revision:
`824b817f889c2cbff1d48fcdb18ab494a68f69d1`, compiler GCC 8.3.

The stripped executable is **399,244 bytes**, dynamically needs only `libc.so.0`,
and the tested artifact has SHA-256:

```text
066722fa0b32d70f041ae93f95e5576fb0d9dd3c8d23907031480b5fae7318f9
```

Rebuild hashes can vary with compiler versions and source changes. Functional
checks used actual SSH terminals at 160 x 52 and 80 x 25, loaded E1M1, captured
colour frames, and verified Ctrl+C cleanup. The README image is a PNG rendering
of a complete frame received from the board's SSH terminal, not a local PC game.
