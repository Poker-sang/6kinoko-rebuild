# Incremental C++ migration (2026-09-19)

Baseline: `3cffc802f4c54ebb5ac1b9c5dfb5482e0be84547`.

## Completed source batches

- Removed 242 disconnected legacy definitions (239 names, 41,874 function-body
  lines). This is deletion, not a claim that those lines were migrated to C++.
  `removed-definitions.json` records the baseline hashes/locations, and the
  original `6kinoko.exe.c` reference remains unchanged.
- Replaced Actor lifecycle/control-count and owned-object paths with source-backed
  C++ using the vendored Squirrel 2.2.2 API.
- Replaced register-entry shims with compiler-generated x86 C++ entries and
  separated the remaining legacy frame scanner from its production ABI entry.
- Moved the active ACT loader, MCD parsing, layouts, map rendering, resources and
  Squirrel-facing ACT binding into native C++ translation units.
- Added typed legacy-layout adapters plus real Squirrel 2.2.2 object/pair
  ownership helpers instead of overlaying VM internals in decompiled C.
- Migrated another native-callback group to the Squirrel source API, covering
  truthiness, string/object arguments, integer/float callbacks and BGM/SE-style
  argument lists.
- Added Win32 contracts for ACT property parsing/mapping, Squirrel ownership and
  weak references, callback adapters, actor lifecycle, legacy register/copy
  boundaries, plus the pre-existing runtime contracts.
- The compilable source tree contains zero handwritten inline assembly and zero
  naked functions; CI enforces that boundary.

## Validation boundary

Windows CI builds quiet and diagnostic Win32 configurations and preserves logs,
executables and linker maps. The latest successful check for the final source
revision is the authoritative asset-free validation result.

The original game assets and executable are not available in this environment.
Startup, jump, monster visibility and visual/gameplay parity still require the
original-asset local smoke test specified by `AGENTS.md`.

## Remaining legacy boundary

The project still contains a large generated C host for game code that has not
yet benefited from migration. The zero-argument `_memcpy2` compatibility path
also retains its historical frame/candidate-selection heuristic; no invented
arguments or no-op replacement were introduced.

Resource staging, archive order and EXE-relative DAT loading are unchanged.
