# 6kinoko Rebuild

English | [简体中文](README.zh-CN.md)

Reconstructing a game executable that reads the original `6kinoko_*.dat` archives, based on original decompiler evidence and Squirrel 2.2.2 source. The current goal is original Windows behavior; the ultimate goal is cross-platform support.

![Title screen rendered by the rebuilt runtime](images/gameplay.png)

*The rebuilt runtime loading and rendering the title screen from the original DAT resources.*

## Current Status

- [PR #13](https://github.com/Poker-sang/6kinoko-rebuild/pull/13) is merged at `76443ccbcb15b93112fcb677f57b1c00c3615ac8`.
- The main runtime host is C++17. This round of internal pointer, recovered field layout, ownership, and redundant legacy interface cleanup is complete. See the [migration ledger](../docs/internal-types-20260926/README.md) for scope and reasons for retained boundaries.
- The latest artifact confirmed working by the user is `internal-types-59`, built from `8047e62c1c75a1aabc611a6bff812c5148ec3433`. The user reported that it ran correctly. The game and 65 contract executables were compiled; the agent did not run the game or automated tests for that batch.
- The game currently requires **Windows x86 and MSVC**. Platform backends and x64 migration are not complete. Retained layout assertions, real virtual calls, protocol integers, and unknown bytes must not be removed without evidence.

## Build

Requires Windows, Visual Studio 2022 with Desktop development with C++, the Windows SDK, and CMake 3.24 or newer. Supply the three original DAT files yourself. Source dependencies such as Squirrel are in `third_party/`; the top-level CMake configuration is authoritative.

Run from the repository root. Commit source changes first and use a new name for each batch; the script refuses to overwrite existing artifacts.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\build_staged.ps1 `
  -Name win32-release-next `
  -SourceDir C:\WorkSpace\6kinoko `
  -Generator "Visual Studio 17 2022"
```

Replace `SourceDir` with your original resource directory. This builds quiet Win32 Release, compiles the contract executables, and copies and verifies the three DAT files. It **does not run the game or tests**.

| Path | Contents |
| --- | --- |
| `build-runs/win32-release-next/` | CMake build tree, source commit, build logs, and `artifacts.json` |
| `runtime-builds/win32-release-next/kinoko_retdec_rebuild.exe` | Game executable |
| `runtime-builds/win32-release-next/6kinoko_[a-c].dat` | Original resources verified by size and SHA256 |
| `runtime-builds/win32-release-next/tools/` | Contracts and development tools |

## Running and Validation

Run the game yourself:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\run_staged.ps1 `
  -Executable .\runtime-builds\win32-release-next\kinoko_retdec_rebuild.exe `
  -Wait
```

The game must load all three DAT files beside its EXE. Do not use the reference directory as the working directory or bypass staging with `--data-dir` / `KINOKO_DATA_DIR`. The staging script does not copy `index.dat`, nor does it copy or overwrite original `marisa[A-C].dat` saves.

Record successful builds, compiled contracts, and passing executions separately. Gameplay validation is performed by the user; local automated tests are authorized for this workflow repair batch. Retain all historical builds, logs, and failed artifacts.

## Source Guide

| Directory / file | Responsibility |
| --- | --- |
| `src/reconstructed/runtime_host.cpp` | C++ runtime host and global services |
| `src/reconstructed/runtime_method_tables.cpp` | Recovered named virtual tables |
| `src/reconstructed/`, `include/kinoko/` | Game runtime chains, field records, and ownership interfaces |
| `src/squirrel/`, `third_party/squirrel-2.2.2/` | Script bridges and source VM |
| `src/platform/` | Current platform support; other runtime modules also retain Windows dependencies |
| `src/decompiled/6kinoko.exe.c` | Original decompiler evidence, not compiled into the game implementation |
| `tests/`, `tools/` | Regression fixtures, resource checks, and diagnostics |

`6kinoko_rebuilt.c` and the old `retdec_asm_stubs` are no longer runtime implementations. The EXE, some CMake targets, and diagnostic macros retain `retdec` for naming compatibility; this does not mean the old C host is still compiled.

## Documentation

- [Migration status and next steps](../MIGRATION.md)
- [Completed scope and retained-boundary ledger](../docs/internal-types-20260926/README.md)
- [Final build handoff](../docs/internal-types-20260926/HANDOFF.md) and [artifact verification record](../docs/internal-types-20260926/artifacts.json)
- [Tools and diagnostics](../tools/README.md)
- [Maintenance instructions](../AGENTS.md)

Older batch documents in `docs/` preserve the evidence, builds, and validation status at the time. Their pending items, paths, and estimates do not automatically describe current remaining work.

## Contributing and Licensing

Base changes on original decompiler output, disassembly, call sites, and object layouts. Do not add game rules absent from the original. The repository does not distribute the original game EXE, DAT archives, or saves; supply legitimately obtained resources yourself. Do not commit resources, saves, dumps, or local analysis databases.

Original project code is under the [MIT License](../LICENSE). This license does not grant rights to the original game, assets, scripts, or other recovered material. This unofficial project is not affiliated with or endorsed by the original creators. Third-party components such as Squirrel 2.2.2 and zlib 1.2.3 retain their respective licenses; see the notices in `third_party/`.
