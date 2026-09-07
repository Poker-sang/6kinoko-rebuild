# 6kinoko Windows Rebuild

This project reconstructs the original Windows x86 game from its decompiled
code and original DAT resources. Gameplay remains driven by the packaged
scripts and recovered native methods. It is playable in part; full behavioral
equivalence and crash-free gameplay are not yet established.

## Build and Run

Use MSVC, the Win32 platform, CMake 3.24 or newer, and the original DAT files.
Squirrel 2.2.2, zlib 1.2.3 and the D3DX import library are included locally.

```powershell
cmake -S . -B build-runs/release -G "Visual Studio 18 2026" -A Win32
cmake --build build-runs/release --config Release --parallel 4
ctest --test-dir build-runs/release -C Release --output-on-failure
powershell -NoProfile -ExecutionPolicy Bypass -File tools/stage_dat.ps1 -Executable runtime-builds/default/kinoko_retdec_rebuild.exe -SourceDir ../6kinoko
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_staged.ps1 -Executable runtime-builds/default/kinoko_retdec_rebuild.exe -Wait
```

`KINOKO_RUNTIME_DIR` selects the EXE output directory. `KINOKO_REFERENCE_DIR`
selects the archive test input. The game loads the three DATs beside its EXE;
neither setting changes runtime resource resolution. Saves are separate files.

## Code Map

| Location | Responsibility |
| --- | --- |
| `src/platform/windows_entry.cpp` | Windows entry and exception boundary |
| `src/platform/diagnostics.cpp` | Optional trace output and crash dump |
| `src/reconstructed/legacy_abi.cpp` | Compiler-generated x86 virtual calls |
| `src/reconstructed/actor_collision.cpp` | Recovered actor collision methods |
| `src/reconstructed/actor_methods.cpp` | Actor chip queries, flags and priority |
| `src/reconstructed/act_resource.cpp` | ACT clock, wake deadline and stage cleanup |
| `src/reconstructed/sprite.cpp` | Typed sprite geometry and Direct3D drawing |
| `src/squirrel/squirrel_compile_bridge.cpp` | ACT source to original bytecode |
| `src/squirrel/squirrel_value_bridge.cpp` | Source-based object/error ownership |
| `src/decompiled/6kinoko_rebuilt.c` | Remaining recovered game and VM code |
| `src/decompiled/6kinoko.exe.c` | Unmodified decompiler reference |
| `third_party/squirrel-2.2.2` | Supplied upstream Squirrel sources and license |
| `tests/stage_contract.c` | Actual native methods and original script contracts |
| `analysis/*/report.md` | Original addresses, evidence and validation limits |

The C++ bridges verify the original 32-bit layouts. Script execution still uses
the recovered VM. The optional full C++ Execute backend remains experimental
and requires both `KINOKO_ENABLE_SQUIRREL_CPP_VM=ON` and
`KINOKO_SQUIRREL_CPP_EXECUTE=1`. It is not enabled by ordinary builds.

## Diagnostics

Ordinary builds are quiet and never take automatic render screenshots.
Existing VM trace call sites remain while their surrounding C code is still
sensitive to stack shape. The output sink can be enabled without rebuilding:

```powershell
$env:KINOKO_TRACE = '1'
$env:KINOKO_CRASH_DUMP = '1'
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_staged.ps1 -Executable runtime-builds/default/kinoko_retdec_rebuild.exe -Wait
Remove-Item Env:KINOKO_TRACE, Env:KINOKO_CRASH_DUMP
```

These options write `retdec_trace.log` and, on an unhandled exception,
`retdec_crash.dmp` beside the EXE. Both default to off. Trace files append;
crash dumps replace the previous dump. `KINOKO_TRACE=0` forces quiet output.
For a diagnostic build use `KINOKO_RETDEC_DISABLE_TRACE=OFF` and
`KINOKO_RETDEC_TRACE_FILTER=ON`. External window capture, process dump and
debugger tools are documented in [tools/README.md](tools/README.md).

Before validation, commit the source and select fresh build/runtime directories.
Keep all artifacts and record their source commit. Game smoke checks end after
entering stage one, attempting a jump and seeing an enemy.
