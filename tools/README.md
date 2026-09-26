# Game and development tools

`runtime-builds/<run-dir>/kinoko_retdec_rebuild.exe` is the only game executable.
The C++ host is `src/reconstructed/runtime_host.cpp`; typed original virtual tables are in `src/reconstructed/runtime_method_tables.cpp`. The original decompilation remains evidence only in `src/decompiled/6kinoko.exe.c`.
Stage the three original DAT files beside it with `stage_dat.ps1`, then launch
it with `run_staged.ps1`.

The following command-line tools remain useful for reverse engineering and
validation. CMake writes them to `runtime-builds/<run-dir>/tools/`, together
with their debug symbols when enabled. They are not alternative game builds.

| Executable | Purpose | Source |
| --- | --- | --- |
| `kinoko_archive_inspect.exe` | List DAT entries, offsets and sizes | `inspect_archive.cpp` |
| `kinoko_asset_probe.exe` | Find a packaged asset, inspect its bytes, optionally extract it | `probe_asset.cpp` |
| `kinoko_archive_smoke.exe` | Check archive counts and resource reads; invoked by CTest | `../tests/archive_smoke.cpp` |
| `kinoko_process_dump.exe` | Capture a running process to a full-memory minidump | `capture_process_dump.cpp` |
| `kinoko_dump_inspect.exe` | Inspect minidump exceptions, registers, modules and stack addresses | `inspect_minidump.cpp` |

The archive tools share `include/kinoko/archive.hpp`,
`src/reconstructed/archive.cpp` and `src/reconstructed/asset_store.cpp`.
Those files are retained because asset extraction and the archive tests use
them independently of the game runtime.

The runtime uses the included Squirrel 2.2.2 source tree at
`third_party/squirrel-2.2.2` to compile original inline ACT scripts and for
verified object/error ownership helpers. `KINOKO_SQUIRREL2_ROOT` can select
the supplied external source tree for comparison.
Compilation uses a separate C++ VM and transfers bytecode to the reconstructed
VM; `KINOKO_ENABLE_SQUIRREL_CPP_VM` still controls only experimental execution.
zlib 1.2.3 is vendored and linked statically for original save-file compatibility.

Ordinary builds do not write traces or take automatic screenshots. Set
`KINOKO_TRACE=1` to enable the external trace sink without rebuilding, and
`KINOKO_CRASH_DUMP=1` to enable a crash dump beside the game EXE.
Trace output defaults to failures and scene transitions. Set
`KINOKO_TRACE_VERBOSE=1` together with `KINOKO_TRACE=1` only for detailed VM
ownership tracing; this can generate gigabytes and slow composite enemies.
Quiet output skips integer/name formatting inside the out-of-line trace
helpers; the VM still calls those helpers. Native calls no longer query
Windows memory protection for a diagnostic on every invocation. The independent
`exercise_game_window.py` tool captures the visible game window only when
explicitly requested. The original VM trace call sites remain in quiet builds.
The window tool reports process exit status and window responsiveness, so a
lost-focus input guard can be distinguished from a game exception.

CTest also runs `kinoko_stage_contract.exe` and `kinoko_legacy_abi_contract.exe`.
The latter checks original x86 virtual calls against a compiler-generated
C++ vtable, including receiver, argument order, return values and stack cleanup.

## Examples

Run these from the repository root; replace `<run-dir>` and `<build-tree>`.

```powershell
runtime-builds/<run-dir>/tools/kinoko_archive_inspect.exe --all ../6kinoko/6kinoko_a.dat
runtime-builds/<run-dir>/tools/kinoko_asset_probe.exe ../6kinoko data/script/demo/op.cv4 op.cv4
ctest --test-dir build-runs/<build-tree> -C Release --output-on-failure
runtime-builds/<run-dir>/tools/kinoko_process_dump.exe <pid> capture.dmp
runtime-builds/<run-dir>/tools/kinoko_dump_inspect.exe capture.dmp runtime-builds/<run-dir>/kinoko_retdec_rebuild.exe
```

`inspect_cv4.py` decodes the extracted Squirrel 2.2.2 closure stream for
inspection. `ida_query.ps1` queries an existing IDA MCP database.
`capture_loopback.py` records Windows playback audio through PyAudioWPatch.
These scripts are development tools and are not bundled into the game.
`x64dbg_query.py` accesses debugger tools that are exposed only while a target
is loaded, using the configured local MCP connection without displaying secrets.

## Offline original Actor/VM comparison

Configure `KINOKO_BUILD_ORIGINAL_ORACLE=ON` to build
`kinoko_original_vm_oracle.exe` and `kinoko_original_vm_probe.dll`. The debugger
host stops the original process before WinMain, restores the breakpoint byte,
and calls the probe with the original main thread suspended. Original Actor,
Reset, cleanup and VM instructions are unchanged; no game window/input loop runs.
The probe executes `tests/original_actor_reset.nut` using the original VM and
writes its result to `kinoko_original_vm_probe.dll.log` beside the DLL.

```powershell
$ToolDir = (Resolve-Path "runtime-builds/original-reset-20260913-r6-diag/tools").Path
& "$ToolDir/kinoko_original_vm_oracle.exe" `
  (Resolve-Path "../6kinoko/6kinoko.exe").Path `
  "$ToolDir/kinoko_original_vm_probe.dll" `
  (Resolve-Path "tests/original_actor_reset.nut").Path
```

This address-based oracle targets only the supplied original EXE. Use a new
build/runtime directory per test batch; retain the log with the source revision.
The normal game does not load the oracle DLL.

`game_math_contract` checks the recovered update-thread upward rounding mode,
its x87/SSE behavior, restoration after return/exception, and thread isolation.
`moving_map_contract` loads original w3-c02b terrain-kabe and player scripts,
then checks 1500 frames spanning motion, endpoint pauses and reversals.
The original oracle accepts `tests/original_collision.nut` to compare 300
identical collision inputs against the original x86 function with the original
update-thread rounding mode. It does not start gameplay.
