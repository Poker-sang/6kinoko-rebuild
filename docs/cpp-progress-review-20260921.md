# C++ migration progress review

Reviewed PR #9 at `b6427d5b4006caf5f9fd532dccc5a2bf1d08cd30` on 2026-09-21.
Fetched and checked out `refactor/act-document-loading-20260921`; not merged.
No local build, automated test or game execution was performed for this review.

## Assessment

Library replacement and subsystem extraction are advanced. Game-specific type,
ownership and original-behavior recovery remain in progress. This is not merely
a final rename pass, and there is no defensible whole-project completion percentage.

- Squirrel 2.2.2 is a required source-backed production target in CMake, not an
  optional experimental VM. zlib, Vorbis/Ogg, SqPlus/Sqrat and Boost replacements
  are recorded in upstream-library-audit/migration-completion-20260921.md.
- Native containers/strings and actor, input, map, render and audio modules have
  substantial C++ implementations. Some retain integer pointer ports, generated
  control flow and original address names; moving into .cpp is not semantic closure.
- The PR adds typed ACT document creation/loading, scoped reader ownership,
  partial stage cleanup, runtime pointer/clock interfaces and shared document,
  layer and key layouts. Latest batches retire 44FDE0/450020 adapters; the older
  PR description's claim that 44FDE0 remains is stale.
- The original post-load virtual call at 46618A remains absent from the rebuilt
  stage path. Eager resource parsing has not been proved equivalent. Added failure
  cleanup/borrow detachment is explicitly reconstruction safety, not recovered
  original failure or script-callback behavior.
- EndStage's inherited command-end write versus the vector owner remains an
  acknowledged representation concern (act-cpp-continuation-20260921/BATCH1.md),
  not a newly demonstrated regression in this review.
- Audio scheduling, ACT schema coverage, script/native lifecycle and final shutdown
  still require original evidence and broader typed interfaces.

## Source inventory (not runtime coverage)

Inventory excludes third_party, tests and the untouched original C reference.
Physical lines include comments and blank lines. Address definitions use a
comment/string-masked, line-anchored function-definition regex, not a C++ parser;
counts include compatibility adapters and do not imply each needs rewriting.

| Source group | Files | Lines | Address-named definitions |
| --- | ---: | ---: | ---: |
| src/**/*.cpp | 111 | 20,788 | 210 |
| src/decompiled/6kinoko_rebuilt.c | 1 | 19,123 | 185 |

C++ groups: reconstructed 71 files / 12,224 lines; squirrel 32 / 7,794;
platform 8 / 770. The main C retains 270 distinct function_xxx identifiers and
53 goto tokens; C++ retains 259 such identifiers and 57 goto tokens. These
identifier sets overlap. Ordinary pointers and genuine virtual calls are valid.

Local master main C was 19,192 lines / 187 address definitions. The PR's small
C-file reduction understates its typed C++ improvements, while the existing
20,788 C++ lines overstate semantic completion if treated as fully recovered code.

## Current validation evidence

Inspected existing GitHub run 35590695734 for exact head b6427d5, without
triggering any workflow. All-target quiet Win32 build succeeded. Selected
contracts: 34/36 passed; stage_native_contract and damage_pause_contract SEGFAULT.
ACT layer, clock, document, optimized document, stage owner and document lifetime
contracts passed. Older PR text reports 31/33 for 35ca31f; it is not current-head
validation. Earlier baseline failures are documented, but they do not prove
absence of regressions at this head. No current-head DAT/gameplay claim is made.
R125 remains user-confirmed, not agent-tested.

Run: https://github.com/Poker-sang/6kinoko-rebuild/actions/runs/35590695734

## Original evidence spot-check

IDA MCP session bff7780a opened a temporary copy using the skill's open.ps1.
Input SHA256 matches ../6kinoko/6kinoko.exe:
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Survey: x86, base 0x400000, 3,963 recognized functions (not a migration denominator).

E-imports: IDA enumerated KERNEL32 88, USER32 27, GDI32 6, WINMM 2,
d3dx9_33 5, d3d9 1, IMM32 5 and ole32 3 imports. These provide platform,
window/input, drawing, multimedia and COM anchors; dynamic imports are not
excluded by this summary. The first enumeration callback failed due to Python
scope; the corrected callback above succeeded.

Fresh disassembly confirms:

```text
466179 call sub_428000
46617E mov ecx, [esi]
466180 mov edx, [ecx]
466182 mov eax, [edx+18h]
466185 push offset Source
46618A call eax
```

No result-check branch occurs between load and virtual dispatch. This supports
keeping the gap open; it does not recover the complete callee semantics.
Squirrel auxiliary evidence: ../squirrel-2.2.2/SQUIRREL2 exists; inspected retained
analysis/remaining-mapping-20260920/source-disassembly-excerpts.json, including
SQInstance::Get's ECX receiver and source reference-count operations. No VM change
or new source-to-binary equivalence proof was performed.

## Recommended order

1. Recover 46618A / 4289C0 receiver, dispatch and publication behavior before
   declaring the complete ACT loading chain restored.
2. Finish ACT parser/layout/reader typed boundaries and resolve the EndStage
   ownership concern with original evidence in a separate behavior change.
3. Continue actor/script lifecycle, audio scheduling and application shutdown
   in bounded ownership chains; retain legitimate ABI adapters.
4. Track compiled, CI-executed, user-gameplay-confirmed and original-equivalent
   evidence separately. Refresh PR text to latest-head facts when preparing merge.
