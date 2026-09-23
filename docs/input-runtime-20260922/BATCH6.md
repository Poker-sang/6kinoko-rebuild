# Input frame R1

Source committed before build: 03fde63e0f3454aa7b52d7d0613c42d4ed5dcb17.
Build: build-runs/input-frame-r1-quiet.
EXE: runtime-builds/input-frame-r1-quiet/kinoko_retdec_rebuild.exe.
SHA256: e3c30f6ca68d2cafc05ddf231f0c65a9f64aab2ff76fbf2e093fedad647c4322.

Win32 Release quiet configure/full build exited 0, including input_frame_contract.
Three DAT files staged beside EXE and verified by size/SHA256.
No game, CTest, contract executable or local automated test ran.
All earlier artifacts retained; existing compiler warnings remain.

## Six-batch handoff

| Batch | Native migration | Build/run directory | Source commit |
| --- | --- | --- | --- |
| 1 | Registered keys and modifiers | input-keys-r1-quiet | 1347dda |
| 2 | Physical device assignment/state | input-device-r1-quiet | 1cb51aa |
| 3 | Cluster ownership and merging | input-cluster-r1-quiet | 945f52c |
| 4 | Manager copy and virtual lifetimes | input-copy-r2-quiet | f1178f3 |
| 5 | Configuration and assignment | input-config-r1-quiet | 986b803 |
| 6 | Initialization, frame and script publication | input-frame-r1-quiet | 03fde63 |

Each batch was committed before its independent full build; DATs were staged
and size/hash verified, then its source/handoff pushed to
codex/input-runtime-continuation. Prior rendering commits from
codex/sprite-math-cleanup are preserved on this branch. Fetched origin/master
is dfd2211 (including merged PR #9), confirmed an ancestor of the branch.

The unrelated dirty docs/decompiler-cleanup-r126/original-stage-evidence.json
was not edited or staged. No full-repository restoration percentage is claimed:
this handoff covers input paths, not a new audit of remaining legacy functions.
Game behavior remains unverified by the agent. Regression sources are compiled,
not executed. Remaining boundaries include shared SqPlus integer callback slots
and the host's opaque script object; explicit adapters isolate those boundaries.
