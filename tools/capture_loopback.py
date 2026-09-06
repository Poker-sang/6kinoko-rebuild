"""Record the default Windows playback device using PyAudioWPatch WASAPI."""

import argparse
from pathlib import Path
import sys
import subprocess
import time
import wave


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("--seconds", type=float, default=27)
    parser.add_argument("--module-dir", type=Path)
    parser.add_argument("--executable", type=Path)
    args = parser.parse_args()
    if args.module_dir:
        sys.path.insert(0, str(args.module_dir.resolve()))
    import pyaudiowpatch as audio

    with audio.PyAudio() as client:
        device = client.get_default_wasapi_loopback()
        rate = int(device["defaultSampleRate"])
        channels = device["maxInputChannels"]
        with wave.open(str(args.output), "wb") as output:
            output.setnchannels(channels)
            output.setsampwidth(client.get_sample_size(audio.paInt16))
            output.setframerate(rate)
            with client.open(format=audio.paInt16, channels=channels, rate=rate,
                             input=True, input_device_index=device["index"],
                             frames_per_buffer=1024) as stream:
                print(f"CAPTURE_READY rate={rate} channels={channels} "
                      f"unix_time={time.time():.6f}", flush=True)
                if args.executable:
                    subprocess.run([
                        "powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
                        "-File", str(Path(__file__).with_name("run_staged.ps1")),
                        "-Executable", str(args.executable.resolve()),
                    ], check=True)
                remaining = int(rate * args.seconds)
                while remaining:
                    frames = min(1024, remaining)
                    output.writeframesraw(stream.read(frames))
                    remaining -= frames
    print(f"CAPTURE_DONE {args.output}", flush=True)


if __name__ == "__main__":
    main()
