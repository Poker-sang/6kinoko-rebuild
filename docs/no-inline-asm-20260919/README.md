# Incremental C++ migration (2026-09-19)

Baseline: `3cffc802f4c54ebb5ac1b9c5dfb5482e0be84547`.

## Completed source batches

- Removed 242 disconnected legacy definitions (239 names, 41,874 function-body lines). This is deletion, not a claim of 41,874 lines migrated to C++. `removed-definitions.json` records individual baseline hashes and locations. The original `6kinoko.exe.c` reference is unchanged.
- Replaced Actor constructor, destructor, weak/shared control-count operations, and owned-object SetStep with source-backed C++ using the vendored Squirrel 2.2.2 API.
- Replaced Actor and copy-adjustor register entry shims with compiler-generated x86 fastcall entries whose ECX and stack layout match their original thiscall callers. The legacy copy body itself is not yet C++.
- Corrected the deleting Actor destructor to preserve its actual receiver and free that allocation only when requested by the flags.
- Added real-VM ownership/child-thread tests and independent C++ thiscall tests with compiler stack-balance checking. The C stage probe uses the typed adapter because MSVC C does not accept the C++ thiscall function-pointer syntax.

## Validation boundary

Windows CI runs quiet and diagnostic Win32 builds and preserves logs, executable revisions, and linker maps. Tests are not considered passed until their actual job result is recorded. The first run caught and led to correction of the C stage-probe declaration.

The original game assets and executable are not available in this environment. Startup, jumps, monster visibility, and visual/gameplay parity therefore still require the original-asset local smoke test specified by AGENTS.md.

## In progress

The main rebuilt C translation unit no longer contains inline assembly. The runtime compatibility file still contains an obsolete x87 entry and the legacy zero-argument memcpy frame heuristic; these must be resolved rather than replaced by invented arguments or no-op stubs. No claim of complete C++/VM migration is made.

Resource staging and EXE-relative DAT loading are unchanged.
