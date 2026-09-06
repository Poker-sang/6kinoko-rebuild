# Game and development tools

`runtime-builds/<run-dir>/kinoko_retdec_rebuild.exe` is the only game executable.
It runs the reconstructed original logic in `src/decompiled/6kinoko_rebuilt.c`.
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
