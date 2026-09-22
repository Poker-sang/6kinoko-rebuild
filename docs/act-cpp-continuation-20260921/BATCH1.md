# Runtime pointers and clock — type-only continuation

Base: 4b154b5218f75c39e63d12ce16463f0272d1de8f (PR #9).

This batch keeps the existing execution path. It does not restore 46618A, add
resource loading, change failure policy, or assert that prior safety changes
are literal original instructions. See the preceding EVIDENCE-AUDIT.md.

## Changes

- Runtime word 3 is `KinokoActDocument* active_document`; word 4 is the owned
  `KinokoActSourceHolder* active_holder`, whose document remains borrowed.
  Source holder, active holder, and document ownership are distinct.
- The VM is `SQVM*`. The owned native file-search map is `FindState*`; this is
  the current source-backed representation, not an original MSVC map layout.
- The 24-byte name is the existing `StringRecord`, rather than three duplicated
  fields and a hand-written integer-pointer/short-string decoder.
- Initialization and member disposal have typed APIs throughout stage/map
  callers and tests. Delete the obsolete 44FDE0 and 450020 aliases and the
  integer-only `retdec_destroy_act_runtime` port. Disposal still does not free
  the outer storage; the same caller does so after disposal.
- Clock/EndStage methods use named records, a typed receiver and the existing
  critical-section guard. Remove the local offset enum, integer address class,
  and reference reinterpretation. The bit-preserving DWORD addition, signed
  frame division, missing-source guards, wake-time calculation and call order
  stay unchanged. File enumeration accepts runtime pointers even through C ABI.
- Preserve diagnostics and the same external VM-reference release sequence.
  The FindState wrapper contains the same map and does not change search rules.

## Evidence and limits

Original `src/decompiled/6kinoko.exe.c` 44FDE0 initializes words 3/4 and
152/156; retained IDA captures `analysis/slim-runtime-20260919/ida-act-runtime-
constructor.json` and `ida-act-runtime-destructor.json` show their pointer use
and lifetime. 452040 follows runtime+16 to a document and its layer range.
The destructor disposes members before its callers free the runtime allocation;
normal stage order is retained in original-stage-evidence.json.

Original RetDec 451620/451630 expressions have lost receiver information and
are not independent proof of the clock algorithm. We retain the already
reconstructed algorithm, supported by `analysis/act-cpp-20260908/report.md`,
without claiming a new assembly capture. Original 4515A0 clearly stores the
WinMM deadline at +100. Original 450D80 shows active byte +8, critical section
+20 and the eleven zeroed state words +108..+148.

The inherited EndStage write to the published command end slot is kept in this
batch. It may be inconsistent with the newer owned std::vector representation;
any correction must be a separate evidence-backed behavior commit.

Layout assertions retain 192 bytes and the same offsets. Unknown bytes stay
unknown; neither the entire record nor additional padding is initialized.
The new clock test uses unaligned guarded raw records and tests the actual
source, not a reimplementation. Its OS clock and unrelated container/suspend
ports are explicit controlled boundaries. Existing frame and evidence checks
are added to CI selection rather than replacing or excluding existing tests.

Build/test outcomes will be recorded after the source checkpoint. No original
DAT or game-window verification is claimed by this document.
