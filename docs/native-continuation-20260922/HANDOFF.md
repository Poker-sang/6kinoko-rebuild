# Eight-batch handoff — 2026-09-23

| Batch | Scope | Source commit | Build/runtime directory |
| --- | --- | --- | --- |
| 1 | String append | b17aa4e | native-string-append-r1-quiet |
| 2 | String capacity | 3c0e288 | native-string-capacity-r1-quiet |
| 3 | String ownership and public APIs | 465733c | native-string-owner-r1-quiet |
| 4 | Texture-handle vector | 9cbf519 | native-handle-vector-r1-quiet |
| 5 | Animation/sound map | a6159a3 | native-index-map-r1-quiet |
| 6 | Map layer/event containers | e2e48ee | native-map-containers-r2-quiet |
| 7 | Map runtime ownership transfer | ab4ae74 | native-map-copy-r1-quiet |
| 8 | ACT draw storage | cb5bb11 | native-act-draw-r1-quiet |

Each source was committed before an independent Win32 Release quiet full build.
Each final build succeeded, then stage_dat.ps1 copied the three original DATs
beside the executable and verified size/SHA256. Each batch's source and handoff
were pushed to codex/input-runtime-continuation / PR #10. BATCH1..BATCH8 record
full source revisions and EXE hashes. Failed map-container R1 artifacts remain.

Latest executable:
C:/WorkSpace/6kinoko-rebuild/runtime-builds/native-act-draw-r1-quiet/kinoko_retdec_rebuild.exe
Source: cb5bb11. This is the final compiled source, distinct from handoff-only commits.

No game, CTest, contract executable or local automated test ran during these
batches. Contract sources were compiled only. No runtime compatibility success
or whole-project completion percentage is claimed. Existing compiler warnings
remain. The user owns gameplay validation; no immediate test is requested.

Original IDA evidence uses sessions 62a98cd6 and (after restart) da1983eb,
against the same original executable hash recorded in README.md. The native
STL owners intentionally do not reproduce VC8 allocator growth policies.
String invalid-size handling and ACT resize capacity returns retain the prior
reconstruction's documented native contracts; the shared legacy SqPlus/script
callbacks still convert their integer slots at explicit boundaries.

Work scripts are retained under analysis/native-continuation-20260922/
native-continuation-tools, outside build trees. rewrite.py is an archival
editing aid, not a general C++ parser or validation tool. All previous game
build/runtime artifacts remain untouched. The unrelated dirty
original-stage-evidence.json was neither edited nor committed.
