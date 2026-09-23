# Luckfox Doom

**A tiny board. A terminal. Doom.**

Play Doom over SSH on the **Luckfox Pico Plus (RV1103)**. The board runs the game;
your computer displays its colour ASCII graphics and sends keyboard input.

![Doom E1M1 running on a Luckfox Pico Plus over SSH](docs/images/doom-over-ssh.png)

*Captured from a live 160 x 52 SSH terminal on the board; the received ANSI frame
is rendered here as a PNG. No desktop or display server is involved.*

- About **390 KiB** for the executable and **4 MiB** for the shareware episode.
- Automatically fits your terminal, with brighter colours and clean exit handling.
- Runs on the tested Buildroot image without rebuilding Linux or adding runtime packages.
- Based on [doom-ascii](https://github.com/wojciech-graj/doom-ascii), with its source and attribution preserved.

## Get started

Follow the **[installation guide](docs/INSTALL.md)** to build, copy, and launch the game
from Linux, WSL, or Windows. You need a working SSH connection to the board.

Once installed:

```sh
ssh -t root@luckfox doom
```

Replace `luckfox` with your board's hostname or IP address. Maximize your terminal
before launching; **160 columns by 52 rows** gives the best default detail.
Smaller windows work too. Restart the game after resizing.

Jump straight into the first level:

```sh
ssh -t root@luckfox doom -warp 1 1 -skill 2
```

## Controls

| Key | Action |
| --- | --- |
| Arrow keys | Move / turn |
| `,` / `.` | Strafe left / right |
| Space | Fire |
| E | Open / use |
| `]` | Run |
| 1-7 | Select weapon |
| Escape / Enter | Menu / select |
| Ctrl+C | Exit immediately |

Use the in-game menu to save before quitting. This port has **no audio**.
Terminal key repeat approximates held keys, so controls feel different from a native game window.

## Where everything lives

| Device path | Contents |
| --- | --- |
| `/usr/bin/doom` | Terminal-aware launcher |
| `/opt/doom/doom-ascii` | ARM executable |
| `/opt/doom/doom1.wad` | Game data |
| `/opt/doom/.default.cfg` | Controls and settings |
| `/opt/doom/.savegame/` | Saved games |

## Tested setup

Luckfox Pico Plus, Buildroot 2023.02.6, Linux 5.10.160, ARMv7 hard-float,
uClibc 1.0.31. Cross-built with the Luckfox SDK's GCC 8.3 and tested through
real SSH PTYs at 160 x 52 and 80 x 25. E1M1 loads, colour frames render, and
Ctrl+C restores terminal settings. Other images and boards are not yet verified.
See [build details and checksums](docs/INSTALL.md#verified-build).

## Credits and licensing

Built on [wojciech-graj/doom-ascii](https://github.com/wojciech-graj/doom-ascii),
upstream revision `ce9f7eeb14cf1099b2a03007f919086a3a8be1e2`.
Thanks to id Software, Chocolate Doom, doomgeneric, and Wojciech Graj.
See the [original README](README.upstream.md) and per-file copyright notices.

Source is distributed under the [GNU GPL](LICENSE). Doom game data is separately
licensed by id Software and is not included in this repository. The shareware
download helper retrieves only the first episode and verifies its checksum.
