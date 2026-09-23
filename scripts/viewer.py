"""Lossless pixel viewer. The game and audio synthesis still run on the board."""
import collections
import struct
import subprocess
import sys
import threading
import time

MAGIC = b"LFDOOM1\n"
FRAME_BYTES = 8 + 768 + 320 * 200


def read_exact(stream, size):
    data = bytearray()
    while len(data) < size:
        part = stream.read(size - len(data))
        if not part:
            raise EOFError("The board closed the pixel stream")
        data.extend(part)
    return bytes(data)


def frames(stream):
    # Startup diagnostics from before the backend initializes may precede magic.
    prefix = bytearray()
    while not prefix.endswith(MAGIC):
        prefix.extend(read_exact(stream, 1))
        if len(prefix) > 65536:
            raise ValueError("No Luckfox pixel stream header; update the board binary")
    while True:
        data = read_exact(stream, FRAME_BYTES)
        sequence, milliseconds = struct.unpack_from("<II", data)
        yield sequence, milliseconds, data[8:776], data[776:]


def run(target, audio=False):
    try:
        import pygame
    except ImportError:
        raise OSError("Install the viewer dependency: python -m pip install pygame-ce")
    command = "cd /opt/doom && exec ./doom-ascii -iwad doom1.wad -config .default.cfg -fixgamma -scaling 1 -pixel-stream"
    if audio:
        command += " -audio"
    flags = {"creationflags": subprocess.CREATE_NO_WINDOW} if sys.platform == "win32" else {}
    process = subprocess.Popen(["ssh", "-T", "-o", "BatchMode=yes", target, command],
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, **flags)
    latest = collections.deque(maxlen=1)
    errors = collections.deque(maxlen=20)
    done = threading.Event()

    def receive():
        try:
            for frame in frames(process.stdout):
                latest.append(frame)
        except (OSError, EOFError, ValueError) as error:
            errors.append(str(error))
        finally:
            done.set()

    def diagnostics():
        for line in process.stderr:
            errors.append(line.decode("utf-8", errors="replace").rstrip())

    reader = threading.Thread(target=receive, daemon=True)
    reader.start()
    threading.Thread(target=diagnostics, daemon=True).start()
    try:
        pygame.display.init()
        window = pygame.display.set_mode((960, 600), pygame.RESIZABLE)
        pygame.display.set_caption("Luckfox DOOM — connecting…")
    except Exception:
        process.terminate()
        try: process.wait(timeout=3)
        except subprocess.TimeoutExpired: process.kill(); process.wait()
        pygame.quit()
        raise
    special = {pygame.K_UP: 0xad, pygame.K_DOWN: 0xaf, pygame.K_LEFT: 0xac,
               pygame.K_RIGHT: 0xae, pygame.K_RETURN: 13, pygame.K_ESCAPE: 27,
               pygame.K_TAB: 9, pygame.K_BACKSPACE: 127,
               pygame.K_LCTRL: 0x9d, pygame.K_RCTRL: 0x9d,
               pygame.K_LSHIFT: 0xb6, pygame.K_RSHIFT: 0xb6,
               pygame.K_LALT: 0xb8, pygame.K_RALT: 0xb8}
    for index in range(12):
        special[getattr(pygame, f"K_F{index+1}")] = (0xbb + index if index < 10 else 0xd7 + index - 10)
    held = set()
    last = None
    previous = None
    report_time = time.monotonic()
    running = True
    clock = pygame.time.Clock()

    def key_event(key, down):
        process.stdin.write(bytes((key, down)))
        process.stdin.flush()

    try:
        while running and not done.is_set():
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.WINDOWRESIZED:
                    if window.get_width() < 320 or window.get_height() < 200:
                        window = pygame.display.set_mode(
                            (max(320, window.get_width()), max(200, window.get_height())), pygame.RESIZABLE)
                elif event.type in (pygame.KEYDOWN, pygame.KEYUP):
                    key = special.get(event.key, event.key if 0 < event.key < 128 else None)
                    if key is not None:
                        down = event.type == pygame.KEYDOWN
                        if down and key not in held:
                            held.add(key); key_event(key, 1)
                        elif not down and key in held:
                            held.remove(key); key_event(key, 0)
                elif event.type == pygame.WINDOWFOCUSLOST:
                    for key in held:
                        key_event(key, 0)
                    held.clear()
            if latest:
                last = latest.pop()
                sequence, ms, palette, pixels = last
                surface = pygame.image.frombuffer(pixels, (320, 200), "P")
                surface.set_palette([palette[i:i+3] for i in range(0, 768, 3)])
                # Integer nearest-neighbour scaling preserves every source pixel.
                scale = max(1, min(window.get_width() // 320, window.get_height() // 200))
                size = (320 * scale, 200 * scale)
                window.fill((0, 0, 0))
                window.blit(pygame.transform.scale(surface, size),
                            ((window.get_width()-size[0])//2, (window.get_height()-size[1])//2))
                pygame.display.flip()
                now = time.monotonic()
                if previous is None:
                    previous = (sequence, ms)
                if now - report_time >= 1 and ms > previous[1]:
                    fps = (sequence - previous[0]) * 1000 / (ms - previous[1])
                    pygame.display.set_caption(f"Luckfox DOOM — {fps:.1f} source fps — lossless")
                    previous = (sequence, ms); report_time = now
            clock.tick(240)
        if done.is_set() and process.poll() not in (None, 0):
            raise OSError("\n".join(errors))
        return 0
    finally:
        # EOF makes the remote engine exit; avoid leaving a game behind.
        try:
            process.stdin.close()
        except OSError:
            pass
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            process.terminate()
            try: process.wait(timeout=3)
            except subprocess.TimeoutExpired: process.kill(); process.wait()
        reader.join(timeout=1)
        pygame.quit()
