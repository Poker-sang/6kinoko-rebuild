# First reconstruction report

## Result

The new x86 project builds an independent Win32 executable under `runtime/`. It does not link against or patch the original executable. The host creates the original `Marisaland2` window class with the original 640x480 logical client size and mounts the three original DAT files through a small, testable resource layer.

## Verified

- Static startup path was checked in IDA at `_WinMain@16`, `sub_407360`, `sub_410500`, and `sub_4109d0`.
- The DAT index decryption and record layout were checked against all three local files.
- Release x86 build completed with Visual Studio 18 CMake generator.
- CTest archive smoke test passed.
- A real entry from every archive was read using its decoded offset and size.

## Deliberate boundary

The original Direct3D scene graph, script VM, custom `.cv4/.cv2` object formats, input system, and gameplay actors remain future reconstruction work. The current GDI renderer is a replaceable startup view so that DAT access and Windows lifetime behavior can be developed independently of that larger migration.

