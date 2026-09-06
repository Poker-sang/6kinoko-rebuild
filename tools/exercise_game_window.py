"""Exercise a specific game PID and record its real window with FFmpeg."""

import argparse
import ctypes as c
from ctypes import wintypes as w
import json
from pathlib import Path
import re
import subprocess
import time


user32 = c.WinDLL("user32", use_last_error=True)
user32.SetProcessDPIAware()
user32.GetWindow.argtypes = [w.HWND, w.UINT]
user32.GetWindow.restype = w.HWND
user32.GetForegroundWindow.restype = w.HWND
user32.SetForegroundWindow.argtypes = [w.HWND]
user32.IsWindowVisible.argtypes = [w.HWND]
user32.GetWindowThreadProcessId.argtypes = [w.HWND, c.POINTER(w.DWORD)]
user32.PostMessageW.argtypes = [w.HWND, w.UINT, w.WPARAM, w.LPARAM]


class Keyboard(c.Structure):
    _fields_ = [("vk", w.WORD), ("scan", w.WORD), ("flags", w.DWORD),
                ("time", w.DWORD), ("extra", c.c_size_t)]


class Mouse(c.Structure):
    _fields_ = [("x", w.LONG), ("y", w.LONG), ("data", w.DWORD),
                ("flags", w.DWORD), ("time", w.DWORD), ("extra", c.c_size_t)]


class Payload(c.Union):
    _fields_ = [("keyboard", Keyboard), ("mouse", Mouse)]


class Input(c.Structure):
    _fields_ = [("type", w.DWORD), ("payload", Payload)]


user32.SendInput.argtypes = [w.UINT, c.POINTER(Input), c.c_int]
KEYS = {"z": 0x5A, "x": 0x58, "up": 0x26, "down": 0x28,
        "left": 0x25, "right": 0x27, "escape": 0x1B}


def window_for(pid):
    found = []
    callback_type = c.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)

    @callback_type
    def collect(hwnd, _):
        owner = w.DWORD()
        user32.GetWindowThreadProcessId(hwnd, c.byref(owner))
        if owner.value == pid and user32.IsWindowVisible(hwnd) and not user32.GetWindow(hwnd, 4):
            found.append(hwnd)
        return True

    user32.EnumWindows(collect, 0)
    if len(found) != 1:
        raise RuntimeError(f"Expected one visible main window for PID {pid}: {found}")
    return found[0]


def press(hwnd, key):
    if user32.GetForegroundWindow() != hwnd:
        raise RuntimeError("Game window lost focus; no key was sent")
    vk = KEYS[key]
    extended = 1 if key in ("up", "down", "left", "right") else 0
    for flags in (extended, extended | 2):
        event = Input(1, Payload(keyboard=Keyboard(vk, 0, flags, 0, 0)))
        if user32.SendInput(1, c.byref(event), c.sizeof(event)) != 1:
            raise c.WinError(c.get_last_error())
        time.sleep(0.12)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("pid", type=int, nargs="?")
    parser.add_argument("--executable", type=Path)
    parser.add_argument("--key", choices=KEYS)
    parser.add_argument("--capture", type=Path)
    parser.add_argument("--record", type=Path)
    parser.add_argument("--seconds", type=float, default=1)
    parser.add_argument("--events", default="", help="Seconds:key pairs separated by commas")
    parser.add_argument("--close", action="store_true")
    args = parser.parse_args()
    if not 0 < args.seconds <= 60:
        parser.error("--seconds must be within (0, 60]")
    if args.executable:
        launcher = Path(__file__).with_name("run_staged.ps1")
        launch = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
                                 "-File", str(launcher), "-Executable", str(args.executable)],
                                capture_output=True, text=True, check=True)
        print(launch.stdout, end="", flush=True)
        args.pid = int(re.search(r"^PID (\d+)$", launch.stdout, re.MULTILINE)[1])
    elif args.pid is None:
        parser.error("A PID or --executable is required")
    deadline = time.monotonic() + 10
    while True:
        try:
            hwnd = window_for(args.pid)
            break
        except RuntimeError:
            if time.monotonic() >= deadline:
                raise
            time.sleep(0.1)
    user32.SetForegroundWindow(hwnd)
    time.sleep(0.3)
    print(json.dumps({"pid": args.pid, "hwnd": hwnd, "foreground": user32.GetForegroundWindow()}), flush=True)
    command = ["ffmpeg", "-hide_banner", "-loglevel", "error", "-f", "gdigrab",
               "-draw_mouse", "0", "-framerate", "60", "-i", f"hwnd={hwnd}"]
    recorder = None
    if args.record:
        recorder = subprocess.Popen(command + ["-t", str(args.seconds), "-c:v", "libx264",
                                     "-preset", "ultrafast", "-crf", "16", str(args.record)])
    start = time.monotonic()
    try:
        if args.key:
            press(hwnd, args.key)
        events = [event.split(":", 1) for event in args.events.split(",") if event]
        for when, key in events:
            if key not in KEYS or not 0 <= float(when) <= args.seconds:
                raise ValueError(f"Invalid key event: {when}:{key}")
            time.sleep(max(0, start + float(when) - time.monotonic()))
            press(hwnd, key)
        time.sleep(max(0, start + args.seconds - time.monotonic()))
    finally:
        if recorder and recorder.wait(timeout=15) != 0:
            raise RuntimeError("FFmpeg recording failed")
    if args.capture:
        subprocess.run(command + ["-frames:v", "1", str(args.capture)], check=True)
    if args.close:
        user32.PostMessageW(hwnd, 0x10, 0, 0)


if __name__ == "__main__":
    main()
