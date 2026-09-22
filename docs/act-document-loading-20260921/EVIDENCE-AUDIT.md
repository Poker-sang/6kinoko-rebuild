# Evidence audit and typed source-holder continuation — 2026-09-21

## Correction to the previous report

It is **not** accurate to describe all changes in PR #9 as literal recovery of
original behavior. The PR combines a type migration with separately committed
rebuild safety repairs. A justified safety repair is still new behavior on its
failure/unwind path; passing a synthetic contract does not make it an original
instruction. No new gameplay rule, substitute asset, resource search fallback,
or speculative second resource-load pass was added by the audited PR diff.
This statement concerns R127..76f1e89 only, not every earlier reconstruction.

Audit inputs are the unmodified `src/decompiled/6kinoko.exe.c`, captured
`docs/decompiler-cleanup-r126/original-stage-evidence.json`, and the production
changes from R127 `825b039` to PR head `76f1e89`. The local retained source bundle
ends at `edfd5f2`; the two subsequent commits change only documentation/workflow
files, not the audited production source. No new original EXE/IDA session was
available. RetDec's misidentified constructor/destructor labels and lost ECX
receivers are not treated as authoritative function signatures.

## Classification

| Change/claim | Actual basis | What must not be claimed |
| --- | --- | --- |
| CAct allocation size 240 | Original caller 466146/46614E allocates 0xF0 bytes. | A native 64-bit layout, or a complete original class definition. |
| Defaults resolution 16, size 1280x720, name `act`, margins 128, offsets 0, visible 1, suspended 0 | Original 427530..42760C contains those writes; 427750 contains corresponding property names. | These values were inferred from desired screen output. |
| Whole-record zeroing, including unknown words and padding | Already present in R127's rebuilt 427530. Original decompilation writes selected members only. | That original 427530 performed a 240-byte memset. |
| ACT1/version 1/relative-skip/header rejection sequence | Preserved from R127's rebuilt 428000. The retained original RetDec body is severely truncated. | That this audit independently recovered the original header algorithm. |
| Replacing the map allocator spelling with malloc | The existing `_3f__3f_2_40_YAPAXI_40_Z` implementation in `src/platform/retdec_runtime_compat.cpp` already forwards to malloc. | That original operator-new out-of-memory semantics have been proved equivalent. |
| Reader close on normal returns | Same open/read/seek/payload/trace/close sequence as the rebuilt baseline. | That all exception behavior is unchanged. |
| Reader RAII on C++ unwind, and close of a non-null slot after a failed open | Added rebuild safety. The latter also differs from R127's immediate open-failure return. | That original exception handlers or open-failure cleanup have been recovered. |
| Stage rejection of a failed ACT load | Already present in the rebuilt baseline. Original 466179 proceeds directly to the post-load virtual call. | That original stage loading had this `if (!load) return` branch. |
| Partial-stage owner cleanup and allocation unwind guards | Added rebuild safety using existing destructors. | That original malformed-ACT/low-memory behavior or unwind order is known. |
| Holder -> document virtual delete -> runtime -> owner normal release order | Captured instructions 465FB9, 465FD1, 465FE5/465FEB, 465FF4. | Evidence of an original failed-load branch. 465FCC loads the virtual target; the call is at 465FD1. |
| Source-borrow detachment before releasing holder | Added rebuild safety for the current runtime destructor, which reads its source holder. | An original 465F70 instruction, or proof of identical script callback behavior. |
| Reload runtime after document virtual delete | Original 465FD5 reloads owner+8 after 465FD1. | That the game actually replaces runtime in this callback; the replacement in the contract is synthetic. |
| Array word 3 typed as `ActArray* storage` | Current act_array.cpp owns a std::vector there; begin/end are published views. | That the original VC8 vector used this same representation. |

The same safety distinction applies to `/EHsc-`: it permits synchronous C++
unwinding through C-linkage ports; it is not evidence of the original binary's
exception tables. The option's meaning is documented by Microsoft:
https://learn.microsoft.com/en-us/cpp/build/reference/eh-exception-handling-model

## The original post-load gap remains open

The captured original instruction sequence is:

```text
466179  call sub_428000
46617E  mov ecx, [esi]
466180  mov edx, [ecx]
466182  mov eax, [edx+18h]
466185  push offset Source
46618A  call eax
```

There is no result test or conditional branch in that sequence. The retained
CAct table g285 maps slot 0x18 to 4289C0. Its RetDec body still loses the receiver
and conflates virtual dispatch with pointer values. PR #9 does not execute this
broken body, add guessed dispatch, add a no-op completion hook, or run a second
resource pass. The earlier parser's eager resource loading and layout binding
remain inherited reconstruction behavior, **not a proven replacement for
46618A**. Full callee/dispatch/lifetime evidence is required before changing it.

The safety repairs are retained and explicitly labeled, rather than silently
removing them to make the source look more original. This audit does not endorse
them as binary-equivalent. Original failed-load and script-callback comparisons
remain unverified. Existing DAT/gameplay limitations in VALIDATION.md still apply.

## This continuation: types only, no new load policy

Original 44FDE0 stores its holder argument in runtime word 0. Original 455880
stores a document pointer in holder word 0; original 455890 follows that pointer
to the document's layer begin/end. These uses support a real borrowed
`KinokoActSourceHolder*`, rather than an integer-valued runtime field.

- RuntimeRecord::source_holder is now that pointer, with offset-0 and unchanged
  192-byte layout assertions. Frame update, cleanup and destructor read it as a
  pointer; the remaining integer document slots are explicit conversion boundaries.
- `kinoko_act_runtime_initialize(KinokoActRuntime*, KinokoActSourceHolder*)`
  exposes the existing initializer with typed arguments/return. Source-holder
  creation calls it directly. Its field writes and construction order are unchanged.
- The old 44FDE0 name remains only as a thin integer-port adapter for the existing
  C contract. It was **not** claimed to have been removed from the executable.
- Existing failure cleanup, borrowed/clone distinction, allocation behavior,
  callback ordering, FindMap implementation, resource loads and trace sites stay
  unchanged. No new success path or failure recovery is introduced in this batch.

The fixture type assertions check the new API/field types. The evidence tests
verify pinned source hashes, concrete original writes and captured instructions;
they are provenance checks, not original-game behavioral tests. The original
reference hash is 504d899c97d12700aad88d88edbc072f95c600184b684c0bf15bad7471821014;
the captured-stage JSON hash is
0ec538d5c8d730e818a92894f39137c7ed925e3f3862723ba2205abc16965b7a.

Build/test outcomes for this continuation are recorded after committing it,
separately from the earlier edfd5f2 results.
