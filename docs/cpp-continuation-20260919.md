# PR #4 C++ continuation (2026-09-19)

Starting revision: `644b75ab29cb62264f665181fcc8b85bbb9d9933`.

Read `AGENTS.md` before changes. The complete starting source was recovered from
Migration checkpoint run 11, with the bundle HEAD checked against the PR head.

The starting boundary checker passes: 179 source/header files, no inline
assembly or naked entries, and the original decompiler reference is intact.
This is a source audit, not evidence of executable/gameplay equivalence.

The starting Windows workflow run 115 concluded `action_required`; its tests
have NOT run. This authorized checkpoint also requests normal PR validation.

This continuation will record NEW function migrations separately from the
existing PR's disconnected-code deletions. Do not count old changes, generated
transport files, or formatting-only changes as new C++ migration work.

Preserve EXE-relative DAT loading and archive order, use the vendored Squirrel
2.2.2 source, and retain quiet/diagnostic validation. Original EXE/DAT assets
are not available in the working environment; do not claim game parity from
asset-free contracts. Each source batch and its validation revision will be
recorded below.
