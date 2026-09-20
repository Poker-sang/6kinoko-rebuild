# Historical upstream libraries: implementation and remaining boundaries

This is the active PR #7 audit, based on master
`a76a978247d084e1b0a2c1003a8e628cd73d9be1`. The earlier PR #6 description is
not a current inventory. Original `src/decompiled/6kinoko.exe.c` is unchanged.
Vendoring a header is not counted as replacing an executing implementation.

## Implementations actually used by production

| Component | Source and live use | Not implied |
| --- | --- | --- |
| Squirrel | Existing 2.2.2 source VM, shared by both binding libraries | No second VM/bootstrap, no interpreter upgrade |
| zlib | Existing 1.2.3 target; superseded closed codec island removed separately | No save-format or archive-policy change |
| Vorbis/Ogg | `libvorbis-1.2.0` Vorbisfile plus `libogg-1.1.3`, via `vorbis_decoder.cpp` and production audio runtime | Original binary's exact libogg version is not proven |
| Sqrat | 0.8.1 `Object`, `Table`, root/ref ownership and `GetSlot` used through `upstream_sqrat.cpp` | Custom game class/field registration is not all upstream code |
| SqPlus | 20080713 `SquirrelObject`, CreateClass, table/string/closure factories, getVar/setVar scalar arms, declaring-base pointer resolution | Legacy native descriptor/string layouts are not modern C++ object layouts |
| Boost | Minimal 1.44.0 strong/weak counting source; actual constructed `sp_counted_base` subclass and Win32 interlocked backend | No full Boost tree, no arbitrary overlay of std::shared_ptr on old memory |

`stb_vorbis` is no longer a production decoder or a retained second vendor
implementation. Vorbisfile uses owned decoder state and callbacks over borrowed
input that outlives it. Existing archive selection, loop markers, BGM queue and
host output format remain outside the codec. The source/manifest identifies
these selected historical releases; this does not prove the original game was
built with every exact release and compiler setting.

The generated scalar storage record is still 20 bytes on Win32. Source VarRef
objects are created normally on the C++ stack; memcpy bridges unaligned host
values. Source assignment/narrowing executes once, and writeback occurs before
the source Return pushes a VM result. Constant representation, invalid-float
failure status and read-only handling keep the explicitly documented host
policies. String cases still require legacy pointer/24-byte string conversion.
See [scalar integration](sqplus-scalar-source.md).

## Behavior corrections are separate commits

* **Sqrat transient lookup result:** original GetSlot popped the VM result
  before constructing its owning Object. A native `_get` returning fresh
  userdata reproduced a heap-use-after-free with ASan. Retain the result before
  popping; the optional found flag distinguishes missing from present-null
  without performing another lookup. No-copy-elision builds are covered.
* **Historical codec arithmetic/boundary:** UBSan reproduced signed left shifts
  in bit readers/codewords/encoder helpers and an encoder interpolation endpoint
  outside the last band. Narrow fixes are documented in the vendor PATCHES files.
  The encoding helpers are test fixture paths, not new game encoding behavior.
* **SqPlus narrow integers:** original 4AAD07/4AACF5 and snapshot setVar return
  the stored, sign-extended char/short, not the incoming full integer. UINT
  reads/writes four bytes independent of the signed-width metadata. The old
  reconstruction failed the new regression before the separate fix.
  See [source evidence](sqplus-integer-properties.md).
* **SqPlus inherited properties:** original 4AA750 and snapshot getInstanceVarInfo
  use the declaring type to select `__ot`'s native base pointer on a typetag
  mismatch. The host had incorrectly used only the primary instance pointer.
  The source branch is factored within SqPlus.cpp and called by the host; a
  separate behavior commit routes field resolution through it. Static/constants
  bypass mapping; missing bases cannot silently fall back to the wrong object.
  See [receiver evidence](sqplus-instance-properties.md).

These are not a complete security update to old codec releases and do not
restore all MSVC exception-unwind behavior. Source errors are caught at the
new binding boundary and converted to the corresponding VM error text.

## Verification and provenance

All five newly imported libraries retain release/archive/member hashes in
UPSTREAM.json. Local adaptations retain original and adapted member hashes in
PATCHES.json with explanations in PATCHES.md. `tools/verify_upstream.py`, also
called by the migration boundary check, verifies **199 retained members**. It
accepts Git CRLF/LF checkout transformations only, rejects unrecorded source,
missing files, invalid patch baselines and unsafe paths. It verifies reviewed
manifests, not a server signature or the identity of the original executable.
Five Python tests exercise this checker, including intentional corruptions.

Portable actual-source contracts in `tests/upstream` cover Vorbis/Ogg,
Sqrat/SqPlus and Boost. The latest source implementation batch at local commit
`642b1a2` passed all three with ASan+UBSan, no sanitizer recovery and C++
copy-elision disabled. Binding tests run 72 root/independent/child VM cycles,
including concurrent independent VMs and reentrant reference releases.

Selected Win32 CTest contracts grew from 31 to **35 distinct tests**. Important
completed checkpoints (not substituted for later revision checks):

| Windows run | Exact source commit | Evidence |
| --- | --- | --- |
| 35491517815 | f6cd40d1995490603ad72283ec099234ffadcc55 | both configurations 35/35 after source object-method integration |
| 35492447740 | a00d7d3acbf3f9de1c0c74c5916c69da7d548058 | original integer regression intentionally fails before fix |
| 35492572872 | 35a5be58b7eced31b59b7dd85ca7f33b584b755a | both configurations pass after integer fix |
| 35493118260 | 5393ac83ac88fa2a38e50d2433ef44a168ec3c6e | inherited property regression intentionally fails before fix |
| 35493261228 | 0514be16782bcc8e64d0af0fcf8638ff79304fbd | both configurations pass after source receiver integration |
| 35493667851 | ec2d86c82763f7c1ff62eb5c0a498839fff81ade | both configurations 35/35, provenance checks included, temporary workflow removed |

The recorded `vorbis-snapshot.bin` contains generated Ogg bytes and decoded
PCM for mono 44100 Hz, stereo 44100 Hz and stereo 22050 Hz. Downloaded Windows
quiet/diagnostic artifacts for `2cc8a985` and `ec2d86c8` contain byte-identical
snapshots with SHA-256
`aafe9128b7d35d65b37e176dec26dd92e943f414c06a3c8234ed309f1968d010`
(133876 bytes). The earlier draft of this document recorded an incorrect size
and digest; these values were corrected from the actual artifact members.
This validates the narrow codec adaptations, **not** comparison with the
original game. Final CI revision and artifact digests are recorded in the PR;
each artifact contains source-commit.txt and CTest logs. Failed and successful
batches are retained rather than overwritten locally.

## What is still not replaced

Do not report all third-party compatibility code as gone. The generated C
still contains live container/error paths; retdec_runtime_compat.cpp still
contains referenced legacy exception/RTTI/array-helper placeholders, including
`__CxxThrowException_40_8`, `___RTtypeid` and exception-object helpers. Their
signatures/caller unwinding need recovery before native throws are introduced.
Custom Sqrat/SqPlus registration and native descriptor/ABI policies still have
host implementations. The zero-argument frame-copy operand heuristic also
remains; typed library entry points do not make that boundary disappear.

Conversely, do not carry forward stale claims that the old 41A010/41A1D0 and
424430/424640 lexical-cast/stream component is still active: it was removed in
merged commit `27e229a4ab65f33cc21a824e4216bf67a7689209`, with a closed-component
report in `docs/legacy-library-audit-20260920/retired-conversion-island.json`
(135 functions, 48 data definitions). No new Boost lexical_cast or exception
module is imported simply to replace an already-retired component. Boost RTTI
comments alone are not an executing call path. Remaining CRT exception
placeholders are a separate problem and must not be hidden by that deletion.

No original EXE/three DAT gameplay comparison was run here. CI does not verify
visual parity, actual audio output or the AGENTS first-stage/jump/monster smoke.
Keep EXE-relative DAT staging and perform that limited check before accepting
gameplay parity. This document is an implementation audit, not a completion
certificate for the entire library/engine migration.
