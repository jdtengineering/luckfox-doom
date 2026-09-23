#!/usr/bin/env python3
"""Start SSH Doom with optional binary-safe audio playback on this computer."""
import argparse
import shutil
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", default="root@luckfox")
    display = parser.add_mutually_exclusive_group()
    display.add_argument("--pixels", action="store_true", help="Use Sixel graphics")
    display.add_argument("--viewer", action="store_true", help="Use a separate lossless pixel window")
    parser.add_argument("--audio", action="store_true", help="Stream music and effects to ffplay")
    args = parser.parse_args()
    if args.target.startswith("-"):
        parser.error("Invalid SSH target")
    if not shutil.which("ssh"):
        parser.error("OpenSSH must be installed on this computer")
    if args.audio and not shutil.which("ffplay"):
        parser.error("Install FFmpeg (including ffplay) on this computer for audio")
    processes = []
    audio_log = tempfile.TemporaryFile()
    # Redirecting streams alone still leaves Windows children attached to the
    # game console, where they can change its input/VT modes asynchronously.
    background = {"creationflags": subprocess.CREATE_NO_WINDOW} if sys.platform == "win32" else {}
    try:
        if args.audio:
            # The background SSH session cannot prompt for a password.
            subprocess.run(["ssh", "-o", "BatchMode=yes", args.target, "true"], check=True)
            command = ("n=0; while [ ! -p /tmp/doom-audio.pcm ]; do "
                       "n=$((n+1)); [ $n -le 60 ] || exit 1; sleep 1; done; "
                       "exec cat /tmp/doom-audio.pcm")
            audio = subprocess.Popen(["ssh", "-T", "-o", "BatchMode=yes", args.target, command],
                                     stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                     stderr=audio_log, **background)
            processes.append(audio)
            player = subprocess.Popen(["ffplay", "-nodisp", "-autoexit", "-nostats", "-loglevel", "error",
                                       "-f", "s16le", "-ar", "22050", "-ch_layout", "stereo",
                                       "-probesize", "32", "-analyzeduration", "0", "-i", "pipe:0"],
                                      stdin=audio.stdout, stdout=subprocess.DEVNULL,
                                      stderr=audio_log, **background)
            processes.append(player)
            audio.stdout.close()
        if args.viewer:
            import viewer
            return viewer.run(args.target, args.audio)
        command = "doom-pixels" if args.pixels else "doom"
        if args.audio: command += " -audio"
        return subprocess.call(["ssh", "-t", args.target, command])
    except (subprocess.CalledProcessError, OSError) as error:
        print(f"Cannot start game/audio: {error}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        return 130
    finally:
        failed = any(p.poll() not in (None, 0) for p in processes)
        for process in reversed(processes):
            if process.poll() is None:
                process.terminate()
                try: process.wait(timeout=3)
                except subprocess.TimeoutExpired: process.kill(); process.wait()
        if failed:
            audio_log.seek(0)
            message = audio_log.read().decode("utf-8", errors="replace")
            if message:
                print("Audio diagnostics: " + message, file=sys.stderr)
        audio_log.close()


if __name__ == "__main__":
    sys.exit(main())
