# Evidence-audit continuation validation — 2026-09-21

Read EVIDENCE-AUDIT.md first: inherited reconstruction and added safety behavior
must not be described as literal recovery of original game instructions.

## Committed code and actual Windows run

Production commit: `35ca31f19d6f0535f4c6ffc63248a5599104fc28`.
Parent: `76f1e89b1a1e99a59b1366f52f7ea828a12609b4`.
This commit was pushed before its Windows build/tests. It changes the runtime
source-holder field and constructor interface to pointer types, retaining field
layout, initialization order, current safety behavior and the thin 44FDE0 port
for the existing C fixture. It does not restore 46618A or add another resource
load, failure branch, fallback asset, or script behavior.

Run: https://github.com/Poker-sang/6kinoko-rebuild/actions/runs/35586646101
Job: `106291423615`, build (quiet).
The downloaded artifact's source-commit.txt contains the production SHA above;
this is the push build, not a PR merge-ref build.

All targets compiled and linked successfully on Windows 2022, MSVC x86, Ninja
Release, KINOKO_RETDEC_DISABLE_TRACE=ON. The asset-free CTest suite executed
**31/33 successfully**. These four ACT-specific tests all passed:

- act_document_contract
- act_document_optimized_contract
- stage_owner_contract
- act_document_lifetime_contract

The broad stage_native_contract and damage_pause_contract still failed with
access violations, at RVAs 0003FBC7 and 0009308F respectively. These match the
previous migrated run's failure RVAs. The earlier independent R127 control and
its matching function-offset analysis remain documented in VALIDATION.md. This
new run does not fix those failures, does not skip them, and is not a green CI
result. No original-game equivalence follows from the passing contracts.

## Source checks

After local commit `1c1eee06817e3666756344080f8a6fb82e3dafcd`, the local source
checks executed successfully: migration boundaries (617 source/header files),
322 upstream source members/documented patches, 17 legacy-island tests, 5
upstream provenance tests, and 6 new ACT evidence tests. git diff --check was
also clean. The local commit is on the retained edfd5f2 source bundle; remote
35ca31f instead has the documentation/workflow-only 76f1e89 parent. They must
not be described as the same Git commit. The 11-file continuation was uploaded
and its remote diff checked separately; Windows results above are from the
actual remote 35ca31f checkout.

The six new Python evidence tests check immutable reference hashes, original
scalar defaults, the distinction between selective writes and inherited whole-
record zeroing, the captured post-load instruction sequence, normal cleanup
ordering, and the runtime holder's original word-zero store. They are source
provenance tests, not original-game behavior tests. They ran locally; the
existing Windows workflow executes the other source checks and CTest contracts,
not the new test_act_evidence.py pattern.

## Preserved artifact

Artifact: `10632537801`,
`windows-x86-quiet-35ca31f19d6f0535f4c6ffc63248a5599104fc28`.
ZIP size: 22913266 bytes.
ZIP SHA256: `9329670a3c3bf2db8bf9222d4b755bfb3abd9687f9d7e8bea3749dd1fa56c00a`.

Executable within the artifact:
`runtime-builds/ci-quiet/kinoko_retdec_rebuild.exe`.
Size: 867840 bytes.
SHA256: `c754837a74d33efc7075c00b83b1c8a6d4a5bda6b05d28fdc694c5c0b4272d9e`.
The ZIP and executable hashes were recomputed from the downloaded bytes.
Logs, link maps, generated test fixtures and previous artifacts remain intact.

## Still unverified

No original DAT was obtained, staged, size/hash-checked or included. No game
window, stage-one movement, jumping, monsters, rendering/audio comparison or
original IDA/debugger execution was performed. The original post-load 46618A
callee and failure/unwind/callback equivalence remain unresolved. The safety
repairs identified in EVIDENCE-AUDIT.md remain explicitly separate from proven
original behavior. No additional behavior repair was introduced in this batch.
