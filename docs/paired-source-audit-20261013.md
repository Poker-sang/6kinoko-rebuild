# Paired source audit — 2026-10-13

This bounded audit compares rebuild `af5dd9ab4a329962316485d68679bd58cbe36eee`
with modern `d9f4095b`. The rebuild checkout was cloned from
`Poker-sang/6kinoko-rebuild`; no modern platform implementation is copied into
the legacy runtime. Each repository has its own
`codex/paired-source-audit-20261013` branch and remote.

## Paired findings

| Finding | Rebuild | Modern |
| --- | --- | --- |
| DAT mount allocates the declared index before checking physical file size | Fixed using the existing Win32 file owner | Fixed using the portable file service |
| ACT timeline pointer passes through int32_t | Replaced with the real pointer; Win32 behavior retained | Fixed address truncation on x64 |
| ACT layer order, swap and creation use four-byte pointer strides | Uses checked native pointer spans | Fixed wrong order/counts and reads beyond the pointer array on x64 |
| Timeline type query skips a fixed nine-byte descriptor prefix | Uses the native descriptor prefix size | Fixed type-name comparison inside the widened pointer prefix on x64 |
| Map layer binding copies pointer arrays with a fixed four-byte element size | Uses native pointer element sizes; same Win32 bytes | Fixed partial copies and an incorrectly halved texture-count limit on x64 |
| CV2 row-storage arithmetic narrows before checking its allocation limit | Backported checked dimensions and 64-bit byte count | Already fixed in the October 3 audit |
| Savedata recursion, cycles, invalid roots and premature file truncation | Backported bounded traversal, VM stack reservation and serialization before opening the destination | Already fixed |
| BGM preparation and delayed scheduling can interleave with the update worker | Backported command-wide locking with the existing recursive critical section | Already fixed |

Replay-output aliases were also checked for applicability: rebuild has no
modern replay/TAS output subsystem, so that patch is not applicable.

## Resource and pointer contracts

DAT mount still registers an opened archive before decoding its index and
publishes entries incrementally. The size check happens before allocation,
permits index padding and preserves zero-entry archives. Tests include a
six-byte file declaring UINT32_MAX index bytes and an index ending at EOF.
This is a truncation guard, not an arbitrary new archive-size limit.

The ACT loader now passes a typed timeline pointer through load, list insertion
and failure cleanup. Map layer resource binding lives in a focused
`map_layer_binding.cpp` module. Its in-memory buffer validation and copies use
the actual pointer element size; serialized DAT indices and counts keep their
original widths. Missing chips and repeated binding retain the original append
semantics. The focused contract exercises the real binder, including full
pointer copies, invalid buffer lengths, empty buffers and the texture-count
boundary.

ACT script bindings share checked layer-span helpers for order lookup, swapping
and creation. Their regression uses native pointer arrays and malformed spans,
including an incomplete pointer tail. Timeline descriptor queries retain the original
decorated name and skip two native pointer fields plus its leading dot.
The full timeline loader/query path has source review and compilation coverage;
no isolated runtime test for that path is claimed.

CV2 regressions supply short payloads that previously matched wrapped sizes,
alongside valid row padding and encoded data. Savedata regressions independently
check the wire format, a fresh small VM, 256 versus 257 nesting levels, direct
and indirect cycles, shared children, scalar roots, stack balance, preservation
of old file bytes and recovery after failure. The writer still cannot guarantee
old-file preservation after an actual operating-system write failure.

BGM play, play-with-margin, pause, fade and stop hold the same lock used by
worker retirement. Preparation and scheduling are one transaction. Existing
audio contracts compile; this batch does not claim a timing stress or listening
test.

## Validation policy

Commit source before each fresh build. Use distinct build trees under
`build-runs/` and executables under `runtime-builds/`; retain all logs,
manifests and artifacts, including failed attempts. Build Release with tracing
disabled and copy/hash-check all three original DAT files beside each EXE.
Modern additionally retains `fonts/` and `shaders/`.

Target deliveries are rebuild Windows x86 and modern Windows x86/x64 using
Visual Studio 18 2026, the installed toolchain. Local contracts are compiled,
not executed; neither game is launched. CI selections cover the focused
regressions, and modern keeps its existing native compilation matrix. Any CI
execution must be recorded separately from local compilation and user gameplay.

Build results, exact source commits, executable paths and hashes are recorded
after compilation in `paired-source-audit-20261013-artifacts.json`.

