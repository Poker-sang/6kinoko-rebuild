# ACT migration validation — 2026-09-21

## Status

The creation/load boundary and partial-stage ownership changes are committed and
built as quiet Win32 Release. Four ACT-specific contracts passed. The selected
asset-free suite is **31/33**, not fully green: two broad stage contracts crash
in both the new code and a separately built pre-migration control. Original DAT
staging/hash verification and gameplay were **not executed**; no original DATs
or local Windows/IDA session were available to this run. The generated ACT test
fixture is not an original game asset.

## Source checkpoints

- Baseline: `825b0397384944a34a828a23b77bf3432ab0ae05` (R127).
- `f447b3fc5ea2ea48e7007ad06aa71ff739c44774`: typed document creation/load,
  named 240-byte schema, stage/map callers, scoped reader.
- `b16b9ced1a856824e35aea69cc65b89e1394aad6`: synchronous C++ reader unwind
  across C-linkage ports (`/EHsc-`, not asynchronous SEH recovery).
- `7eee77c89a01c9e32eca11dda4346f786c1c0243`: partial stage ownership,
  canonical cleanup and detachment of source borrows before freeing them.
- `edfd5f29568017a4a3fcec9df089c240251ad8c8`: current tested code, including
  the corrected owned-array storage type and test-fixture corrections.

Each production batch was committed before its build/tests. The temporary
write-enabled checkpoint workflow and read-only baseline-control workflow are
removed after preserving their source bundles and results. Normal Windows CI
retains the ACT contracts and read-only repository permissions.

## Executed quiet build and contracts

Run: https://github.com/Poker-sang/6kinoko-rebuild/actions/runs/35583106946

Source: `edfd5f29568017a4a3fcec9df089c240251ad8c8`.
Windows 2022, MSVC x86, Ninja, Release; `KINOKO_RETDEC_DISABLE_TRACE=ON` and
`KINOKO_RETDEC_TRACE_FILTER=ON`. All build targets completed successfully.

| Contract | Result | Exercised scope |
| --- | --- | --- |
| act_document_contract | PASS | Actual typed I/O implementation; checked build, guarded unaligned records, constructor defaults, 12 truncated header prefixes, format/seek errors, result forwarding and reader unwind. |
| act_document_optimized_contract | PASS | Same implementation/assertions with /O2. |
| stage_owner_contract | PASS | Actual stage/source/cleanup units with explicit host mocks; rejected load, constructor/load/list allocation exceptions, unpublished and published ownership, virtual-callback runtime replacement and source-borrow detachment. |
| act_document_lifetime_contract | PASS | Real CFileReader, property/script/layer parser, source holder/runtime and deleting destructor; generated 674-byte ACT with a 7-byte relative skip and heap-owned layer name. All 674 shorter prefixes are rejected and destroyed once. Exclusive file opens in the destructor verify reader-before-document closure. Successful publication, repeated clear and caller destruction also pass. |

The full selected suite passed 31/33. `stage_native_contract` and
`damage_pause_contract` remain failing with access violations. They were not
skipped, converted to success or counted as passed. `actor_records_contract`
passes after registering the fixture's flipped take and checking the actual
mapped-slot return convention. The CStringLayout script separators were fixed
without changing the Squirrel compiler or removing any property assertions.

Post-commit source checks also passed: migration boundary scan of 617 source/
header files; 17 legacy-island tests; 5 upstream provenance tests. The checker
verified 322 historical upstream source members/documented patches. These
source checks do not establish original game behavior.

## Pre-migration control for the remaining crashes

Run: https://github.com/Poker-sang/6kinoko-rebuild/actions/runs/35583349316

The runner checked out R127 and committed only the CStringLayout fixture's
statement separators, producing local control commit
`a8da6e85997d34dbc5e39c618a8d7d821b38d10f`. Before building it verified that
`src`, `include`, `third_party` and `CMakeLists.txt` were unchanged from R127.
The control source bundle and exact test-only patch are retained in its artifact.
Both broad tests still crash. Their link maps identify the same functions and
in-function offsets as the migrated version:

| Test | Control RVA | Migrated RVA | Matching symbol and offset |
| --- | --- | --- | --- |
| stage_native_contract | 0003FB37 | 0003FBC7 | std::vector<int>::_Emplace_one_at_back, +0x7 |
| damage_pause_contract | 00092C0F | 0009308F | function_4077c0, +0x1F |

This establishes that these observed failures also occur without the ACT
migration; it does not diagnose every underlying cause or prove whole-game
regression freedom. They remain follow-up issues, outside the evidence-backed
ACT cleanup change.

## Preserved artifacts and SHA256

Quiet artifact 10631680867 contains EXE/tools, logs, link maps, source revision
and generated fixtures. Its ZIP SHA256 is
`f2f182dbfabd8ff3c7eb5682a1bd5fb721bcee7690c518b6149f73c4f3f65d4a`.

| File | Bytes | SHA256 |
| --- | ---: | --- |
| runtime-builds/ci-quiet/kinoko_retdec_rebuild.exe | 867840 | 68072315a7a9c14ae1915fc235b73188d44edfb38c12265363e613646e775b7b |
| tools/kinoko_stage_contract.exe | 1190400 | e90115298f77b3d7c61fa79efa5b54ecba018fc7ea2d7a972d1672035ec666cc |
| tools/kinoko_act_document_contract.exe | 46080 | 41868cb8cb3da0f3a196a3a3d98cc4d8bcd1dc384efb4622da01e273385c8a11 |
| tools/kinoko_act_document_optimized_contract.exe | 24064 | 262ce62755c7726f6328f9ac591f621fda84861782374e30f79a7d0e45fe229d |
| tools/kinoko_stage_owner_contract.exe | 17408 | 6b2fd291b4417f1a0e91435997c5d5e93377575737aaa10f0611f78c8466ef2c |
| act-document-generated.bin | 674 | 32458af29fde278dd0b5915a19997a58a72b94559043cefc567a3fe9163da3d9 |

Source checkpoint artifact 10631725323 (run 35583081348) retains the tested
commit's Git bundle; ZIP SHA256:
`ad1006583fe9dffeb9cf1ce37753d9f2e457d172d15de4517c271921bd9112fb`.
Control artifact 10631765938 retains its source bundle, test-only patch, EXE,
logs and map; ZIP SHA256:
`771998c323ae8f67b8bb039d523a08ed407bea6ebde18806d229c5c9cd58cc57`.
Earlier unsuccessful compile/unwind runs and their artifacts remain preserved.

Unmodified evidence hashes:

- src/decompiled/6kinoko.exe.c:
  `504d899c97d12700aad88d88edbc072f95c600184b684c0bf15bad7471821014`.
- docs/decompiler-cleanup-r126/original-stage-evidence.json:
  `0ec538d5c8d730e818a92894f39137c7ed925e3f3862723ba2205abc16965b7a`.

## Explicitly unresolved

`46618A` proves a document virtual call through slot 0x18 before holder creation.
The retained table maps it to `4289C0`, but the current C body has a lost receiver
and damaged resource dispatch. The current parser already loads resources, so
adding a second load pass is not an evidence-backed restoration. That behavior
has not been changed. Normal 465F70 cleanup order is evidence for normal cleanup,
not for an original failed-load branch; the original capture has no rejection
branch between 428000 and 46618A.

No DAT files were obtained, copied, hashed, substituted or packaged. No game
window, first-stage jumping/monsters, audio or frame-by-frame comparison was
run. The EXE must still receive the original three DATs in its own directory
through stage_dat.ps1 before a real game run; CI's no-assets directory is not a
runtime data-directory workaround.
