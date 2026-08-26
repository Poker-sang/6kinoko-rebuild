# 6kinoko rebuild

Clean-room reconstruction workspace for the 32-bit Windows game in `D:\6kinoko`.

## Inputs

- Original executable: `D:\6kinoko\6kinoko.exe`
- RetDec output: `src/decompiled/6kinoko.exe.c`
- RetDec configuration: `analysis/6kinoko.exe.config.json`
- `D:\6kinoko` is reference-only. Runtime tests copy the three required DAT files
  into a new run directory next to a copied executable.

The original executable and data files are reference inputs. Build outputs are written under this project and are never written back to `D:\6kinoko`.

## Current target

Produce a 32-bit Windows build that preserves the original startup/window/resource behavior first. New gameplay/content changes will be added after the runtime boundary is stable.

## Build

The first target is an x86 Win32 executable. From a Visual Studio developer shell or a machine with the Visual Studio CMake generator installed:

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32 -DKINOKO_REFERENCE_DIR=D:/6kinoko
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

Outputs are placed in `runtime/`:

- `kinoko_rebuild.exe`: Win32 host with the original class name, title encoding, 640x480 client area, and a replaceable renderer boundary.
- `kinoko_archive_inspect.exe`: prints decoded DAT index entries.
- `kinoko_archive_smoke.exe`: mounts all three DAT files and reads a real indexed asset.

Run a copied executable with a copied DAT set:

```powershell
powershell -File .\tools\run_staged.ps1 `
  -Executable .\runtime-current-x86-20260826d\kinoko_retdec_rebuild.exe `
  -SourceDir D:\6kinoko -Wait
```

The script creates a fresh directory under `runtime-runs/`, copies only
`6kinoko_a.dat`, `6kinoko_b.dat`, and `6kinoko_c.dat`, verifies their size and
SHA256, then starts the copied EXE from that directory. It never copies
`index.dat` and refuses to reuse a runtime directory or overwrite an existing
EXE. `Esc` closes the window and `F5` remounts the three archives.

The `--data-dir` and `KINOKO_DATA_DIR` options remain available on the
reconstructed host for archive diagnostics, but are not part of the normal game
runtime test.

The current renderer is an isolated GDI runtime view. It proves the Windows startup and archive boundary; it is not yet a reimplementation of the original Direct3D scene/gameplay. The next replacement point is `IRenderer` in `include/kinoko/renderer.hpp`.

## Analysis policy

Use the decompiled C as the primary reference, validate uncertain control flow and imports against the original PE in IDA, and inspect DAT data only when the startup/resource path requires it.
