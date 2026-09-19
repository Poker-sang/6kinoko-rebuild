# PR #4 C++ continuation (2026-09-19)

Starting revision: `644b75ab29cb62264f665181fcc8b85bbb9d9933`.
Final migration checkpoint before cleanup: `34780a9b5a23b1e8d9d225395cb44042e67ccd80`.

Read `AGENTS.md` before changes and preserved its two important runtime
constraints: use the vendored Squirrel 2.2.2 source, and keep the original DAT
lookup relative to the produced EXE rather than inventing a data-directory
override.

## Continuation size

From the requested starting revision through the final migration checkpoint:

- 6,734 added lines
- 5,353 removed lines
- 12,087 changed lines across 28 files

This count is intentionally separate from the earlier PR #4 deletion of
disconnected RetDec functions.

## Completed in this continuation

- Moved 106 live ACT/MCD/layout/map/resource/Squirrel-binding definitions out of
  the giant decompiled C translation unit into native C++ units. The original
  bodies represented 5,008 function-body lines.
- Split ACT property parsing/mapping into an independently testable C++ unit,
  including truncation, bounds, ownership-output, coercion, duplicate-key and
  field-offset contracts.
- Added typed Win32 legacy-memory views so C++ code can preserve original object
  layout without reproducing Squirrel VM internals through pointer arithmetic.
- Added typed Squirrel object-pair helpers backed by the real 2.2.2 API and tests
  for external references, weak references, class/user diagnostics and stack
  balance.
- Retired another group of legacy native-callback definitions from
  `6kinoko_rebuilt.c`; their C++ implementations now use Squirrel 2.2.2 stack
  and type APIs for truth conversion, string/object arguments and captured
  callbacks.
- Isolated the zero-argument legacy frame entry from its historical operand
  scanner and test the production register-forwarding ABI independently under
  checked and optimized callers.
- Removed all handwritten inline assembly and naked entries from compilable
  source. The source boundary audit checks 195 source/header files and also
  verifies the untouched original `6kinoko.exe.c` reference hash.

## Validation boundary

Windows x86 CI builds both quiet and diagnostic configurations and exercises the
asset-free contracts, including the real vendored Squirrel VM. The temporary
migration checkpoint workflow is not part of the final tree.

The original EXE/DAT assets are not available in this execution environment, so
the AGENTS.md gameplay smoke test (first stage, jump, monster visible, exit) is
still a local/original-assets validation step. This document therefore does not
claim visual or gameplay equivalence solely from asset-free CI.

## Remaining legacy boundary

This PR deliberately does not fabricate behavior for unresolved code. The
`_memcpy2` compatibility boundary still preserves the historical
frame/candidate-selection heuristic; its ABI forwarding and deterministic
scanner behavior are covered separately, but that is not proof that every old
call site has fully recovered explicit copy operands.

A substantial generated C host remains for non-Squirrel/non-ACT game code. The
goal of this PR is the requested large, behavior-preserving migration milestone,
not a claim that every decompiled function in the game is now idiomatic C++.
