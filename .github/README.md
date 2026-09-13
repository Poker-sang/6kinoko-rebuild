# 6kinoko Rebuild

English | [简体中文](README.zh-CN.md)

An unofficial source-level reconstruction of the Windows x86 version of
6kinoko. The rebuilt executable reads the three original `6kinoko_*.dat`
archives from its own directory and runs the recovered native and scripted
game logic.

The game is partially playable, including entering the first stage, movement,
jumping, and enemies. Full behavioral equivalence and crash-free gameplay have
not yet been established. This repository is an engineering and preservation
project, not a finished replacement for the original game.

![Title screen rendered by the rebuilt runtime](./images/gameplay.png)

*The rebuilt runtime loading and rendering the title screen from the original
DAT resources.*

## Goals and Boundaries

- Reproduce the original Windows behavior before adding new game content.
- Preserve the original DAT and ACT/script loading paths and let the recovered
  rules naturally produce the original behavior.
- Replace generated C and handwritten x86 assembly with readable C++ only when
  calling conventions, object layouts, and behavior can be supported by
  evidence and tests.
- Use the original Squirrel 2.2.2 source when a recovered function can be
  identified reliably as runtime code.
- Keep diagnostics optional and avoid changing gameplay merely to hide a known
  crash or match an observation.

The repository does **not** include the original DAT archives, save files, or
an original game executable. You need a legitimately obtained copy of the
original game to run the rebuild.

## Current Architecture

The runtime remains a hybrid reconstruction. Most recovered game and VM logic
is still in the large generated C translation unit, while verified subsystems
are being moved to focused C++ modules. Original ACT scripts remain responsible
for stage and entity behavior.

| Location | Responsibility |
| --- | --- |
| `src/platform/windows_entry.cpp` | Windows entry point and exception boundary |
| `src/platform/diagnostics.cpp` | Optional trace output and crash dumps |
| `src/reconstructed/legacy_abi.cpp` | Compiler-generated x86 virtual calls |
| `src/reconstructed/actor_collision.cpp` | Recovered Actor collision methods |
| `src/reconstructed/actor_methods.cpp` | Actor chip queries, flags, and priority |
| `src/reconstructed/act_resource.cpp` | ACT clock, wake deadline, and stage cleanup |
| `src/reconstructed/stage_cleanup.cpp` | Global ACT ownership and sound shutdown |
| `src/reconstructed/script_callbacks.cpp` | Actor/Camera callback binding and SqPlus temporary lifetimes |
| `src/reconstructed/sprite.cpp` | Typed sprite geometry and Direct3D drawing |
| `src/squirrel/squirrel_compile_bridge.cpp` | ACT source compilation to original bytecode |
| `src/squirrel/squirrel_value_bridge.cpp` | Source-based object and error ownership |
| `src/squirrel/squirrel_generator_bridge.cpp` | Original generator suspension/resumption and array removal |
| `src/decompiled/6kinoko_rebuilt.c` | Remaining recovered game and VM code |
| `src/decompiled/6kinoko.exe.c` | Unmodified decompiler reference |
| `third_party/squirrel-2.2.2` | Vendored Squirrel 2.2.2 source and license |
| `tests/stage_contract.c` | Native-method and original-script contracts |
| `analysis/*/report.md` | Original addresses, evidence, and validation limits |

## Requirements

- Windows
- Visual Studio with the MSVC x86 toolchain and Windows SDK
- CMake 3.24 or newer
- A Win32 generator supported by the installed Visual Studio version
- The original `6kinoko_a.dat`, `6kinoko_b.dat`, and `6kinoko_c.dat` files

Squirrel 2.2.2, zlib 1.2.3, and the D3DX9 import library required by the
current build are vendored in this repository.

## Build and Run

Run PowerShell from the repository root. Adjust the generator if your installed
Visual Studio version uses a different CMake generator name.

```powershell
$BuildTree = "build-runs/release"
$RuntimeDir = Join-Path (Get-Location) "runtime-builds/release"
$ReferenceDir = (Resolve-Path "../6kinoko").Path

cmake -S . -B $BuildTree `
  -G "Visual Studio 18 2026" -A Win32 `
  -DKINOKO_REFERENCE_DIR="$ReferenceDir" `
  -DKINOKO_RUNTIME_DIR="$RuntimeDir"
cmake --build $BuildTree --config Release --parallel 4
ctest --test-dir $BuildTree -C Release --output-on-failure

powershell -NoProfile -ExecutionPolicy Bypass -File tools/stage_dat.ps1 `
  -Executable "$RuntimeDir/kinoko_retdec_rebuild.exe" `
  -SourceDir $ReferenceDir
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_staged.ps1 `
  -Executable "$RuntimeDir/kinoko_retdec_rebuild.exe" -Wait
```

`stage_dat.ps1` copies and verifies exactly the three required DAT archives.
The executable resolves resources beside itself; do not use the original game
directory as its working directory and do not use a data-directory override.
Original saves such as `marisaA.dat`, `marisaB.dat`, and `marisaC.dat` are
separate and are not staged by this script.

`KINOKO_RUNTIME_DIR` selects the executable output directory.
`KINOKO_REFERENCE_DIR` supplies archive inputs to tests and staging; it does
not change runtime resource resolution.

## Squirrel 2.2.2

The vendored Squirrel 2.2.2 source is currently used to compile inline ACT
source in a separate VM and to provide verified object/error ownership,
generator suspension/resumption, and array-removal helpers. Compiled bytecode
normally executes in the recovered game VM.

The full C++ execution backend is still experimental. It requires both the
CMake option `KINOKO_ENABLE_SQUIRREL_CPP_VM=ON` and the runtime environment
variable `KINOKO_SQUIRREL_CPP_EXECUTE=1`. Ordinary builds do not enable it.

## Diagnostics

Ordinary builds are quiet and do not take automatic render screenshots. The
remaining VM trace call sites stay in place because some recovered C code is
sensitive to stack shape and timing. Their output can be enabled without a
rebuild:

```powershell
$env:KINOKO_TRACE = "1"
$env:KINOKO_CRASH_DUMP = "1"
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_staged.ps1 `
  -Executable "runtime-builds/release/kinoko_retdec_rebuild.exe" -Wait
Remove-Item Env:KINOKO_TRACE, Env:KINOKO_CRASH_DUMP
```

These options write `retdec_trace.log` and, after an unhandled exception,
`retdec_crash.dmp` beside the executable. Both are disabled by default.
`KINOKO_TRACE=0` explicitly keeps trace output quiet. For a diagnostic build,
configure with `KINOKO_RETDEC_DISABLE_TRACE=OFF` and optionally
`KINOKO_RETDEC_TRACE_FILTER=ON`.

Additional archive, window-capture, process-dump, and debugger utilities are
documented in [tools/README.md](tools/README.md).

## Validation Policy

Every gameplay validation batch must start from a committed source revision
and use new build and runtime directories. Builds, executables, logs,
screenshots, and failed attempts are retained with their source revision.

The bounded gameplay smoke check is: enter the first stage, attempt a jump,
move until an enemy is visible, and then exit immediately. Pre-existing
intermittent access violations are recorded but are not automatically treated
as migration regressions.

## Contributing

Changes should be based on decompiled-source evidence, original disassembly,
call sites, object layouts, and focused contracts. Avoid symptom-specific game
rules that do not exist in the original program. Keep original resource inputs
outside the repository and never commit DAT archives, original executables,
save files, crash dumps, or local analysis databases.

## License and Third-Party Notices

Original code written for this reconstruction is released under the
[MIT License](LICENSE).

The MIT License does not grant rights to the original 6kinoko game, its assets,
DAT archives, executable, scripts, or other recovered copyrighted material.
This project is unofficial and is not affiliated with or endorsed by the
original creators. Third-party components remain under their respective
licenses, including the notices distributed with Squirrel 2.2.2 and zlib
1.2.3.
