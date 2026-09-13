# 6kinoko Rebuild

English | [简体中文](README.zh-CN.md)

An unofficial source-level reconstruction of the Windows x86 version of
6kinoko. The rebuilt executable reads the three original `6kinoko_*.dat`
archives from its own directory and runs the recovered native and scripted
game logic.

On September 13, 2026, the user played through the first world using source
revision `33bff37` and reported that the severe gameplay bugs previously seen
were gone, with no crashes during that playthrough. This includes disappearing
FairyOtedama balls after reentry and enemies flying upward indefinitely after
death. Later worlds and full equivalence with the original remain unverified.
This is an engineering and preservation project, not a finished replacement.

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
| `src/platform/diagnostics.cpp` | Optional trace output, crash dumps, and first script-error snapshots |
| `src/reconstructed/legacy_abi.cpp` | Compiler-generated x86 virtual calls |
| `src/reconstructed/actor_collision.cpp` | Recovered Actor collision methods |
| `src/reconstructed/actor_methods.cpp` | Actor chip queries, flags, and priority |
| `src/reconstructed/actor_animation.cpp` | Animation timing, frame selection, and bounds |
| `src/reconstructed/actor_cleanup.cpp` | Actor-manager animation and container cleanup |
| `src/reconstructed/act_resource.cpp` | ACT clock, wake deadline, and stage cleanup |
| `src/reconstructed/stage_cleanup.cpp` | Global ACT ownership and sound shutdown |
| `src/reconstructed/script_callbacks.cpp` | Actor/Camera binding, original class-default cleanup, and failure retirement |
| `src/reconstructed/sprite.cpp` | Typed sprite geometry and Direct3D drawing |
| `src/squirrel/squirrel_compile_bridge.cpp` | ACT source compilation to original bytecode |
| `src/squirrel/squirrel_value_bridge.cpp` | Source-based object and error ownership |
| `src/squirrel/squirrel_gc_bridge.cpp` | Source-based GC chain/sweep and VM/array destruction |
| `src/squirrel/squirrel_generator_bridge.cpp` | Original generator suspension/resumption and array removal |
| `src/decompiled/6kinoko_rebuilt.c` | Remaining recovered game and VM code |
| `src/decompiled/6kinoko.exe.c` | Unmodified decompiler reference |
| `third_party/squirrel-2.2.2` | Vendored Squirrel 2.2.2 source and license |
| `tests/stage_contract.c` | Native-method and original-script contracts |
| `analysis/*/report.md` | Original addresses, evidence, and validation limits |

## Current Checkpoint

- Source: `72614f7`; quiet runtime:
  `runtime-builds/original-reset-20260913-r6-quiet/kinoko_retdec_rebuild.exe`.
  Diagnostic counterpart: `runtime-builds/original-reset-20260913-r6-diag/kinoko_retdec_rebuild.exe`.
- Original cleanup resets the Actor **class defaults**, not the old instance's
  `user` and `step`. Restoring that receiver lets an executing script finish
  after Reset without losing its state. The temporary callback-identity guard
  from `33bff37` has been removed; original failure handling is restored.
- Direct execution of original Actor/VM functions before game WinMain confirms
  that old fields survive 32 nested Reset calls while new instances are created.
  This is an offline comparison, not automated gameplay.
- Both restored builds passed four CTest tests and original DAT enemy tests:
  four ball generations across resets and ordinary/stomp death, with no script
  failures in that contract.
- The user's crash-free first-world playthrough was on the earlier `33bff37`
  candidate. Live validation of this original-method correction remains pending.

Local build directories and executable artifacts are not distributed by Git.

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
Visual Studio version uses a different CMake generator name. Use a new build/runtime directory name for each validation batch.

```powershell
$BuildTree = "build-runs/release"
$RuntimeDir = Join-Path (Get-Location) "runtime-builds/release"
$ReferenceDir = (Resolve-Path "../6kinoko").Path

cmake -S . -B $BuildTree `
  -G "Visual Studio 17 2022" -A Win32 `
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
generator suspension/resumption, garbage collection and destruction, and
array-removal helpers. Compiled bytecode
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

Diagnostic files are written beside the EXE:

| Setting | Output or behavior |
| --- | --- |
| `KINOKO_TRACE=1` | `retdec_trace.log` |
| `KINOKO_CAPTURE_FIRST_CHANCE=1` | Records the first relevant native exception, even if later handled |
| `KINOKO_CRASH_DUMP=1` | Timestamped `fault-…-unhandled.dmp`; first-chance capture can also produce `fault-…-first.dmp` |
| `KINOKO_CAPTURE_SCRIPT_FAILURE=1` | First script-error `fault-…-script.dmp` and metadata in `fault-….log`, even with trace output disabled |

A script snapshot does not mean that the process crashed. It records VM state
before error unwinding and leaves normal error handling in place. Full-memory
dumps can be large and briefly pause execution when saved.

Ordinary builds disable these features by default. Diagnostic builds can enable
them at build time using `KINOKO_CAPTURE_FIRST_CHANCE=ON`,
`KINOKO_CAPTURE_SCRIPT_FAILURE=ON`, and `KINOKO_RETDEC_DISABLE_TRACE=OFF`.
`KINOKO_RETDEC_TRACE_ERRORS_ONLY=ON` limits traces to script/actor failures and
exception markers; `KINOKO_RETDEC_TRACE_FILTER=ON` keeps broader diagnostic output.

For normal play, use the quiet build, or disable all four switches before
launching the diagnostic EXE. Using the same EXE directory keeps its saves:

```powershell
$env:KINOKO_TRACE = "0"
$env:KINOKO_CAPTURE_FIRST_CHANCE = "0"
$env:KINOKO_CRASH_DUMP = "0"
$env:KINOKO_CAPTURE_SCRIPT_FAILURE = "0"
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_staged.ps1 `
  -Executable "runtime-builds/original-reset-20260913-r6-diag/kinoko_retdec_rebuild.exe" -Wait
Remove-Item Env:KINOKO_TRACE, Env:KINOKO_CAPTURE_FIRST_CHANCE, `
  Env:KINOKO_CRASH_DUMP, Env:KINOKO_CAPTURE_SCRIPT_FAILURE
```

Deleting old logs frees storage; it does not disable future logging or snapshot
creation. Close the game before deleting logs it still has open.

Additional archive, window-capture, process-dump, and debugger utilities are
documented in [tools/README.md](../tools/README.md).

## Validation Policy

Every gameplay validation batch must start from a committed source revision
and use new build and runtime directories. Builds, executables, logs,
screenshots, and failed attempts are retained with their source revision unless
the user explicitly requests cleanup. Such cleanup must target the requested
artifacts and preserve resources and saves.

Live gameplay testing is currently performed by the user. Agent validation uses
static analysis, builds, and offline contracts. If an agent-run gameplay smoke
check is requested, its bounded scope is: enter the first stage, attempt a jump,
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
[MIT License](../LICENSE).

The MIT License does not grant rights to the original 6kinoko game, its assets,
DAT archives, executable, scripts, or other recovered copyrighted material.
This project is unofficial and is not affiliated with or endorsed by the
original creators. Third-party components remain under their respective
licenses, including the notices distributed with Squirrel 2.2.2 and zlib
1.2.3.
