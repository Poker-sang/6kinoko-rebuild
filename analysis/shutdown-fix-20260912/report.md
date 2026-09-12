# Shutdown recursion fix

Scope: repair confirmed stack overflow from the user's WER dumps. Carry forward
skills, E-triage/E-imports and diagnosis from ../crash-check-20260912/report.md
and ../actor-state-cpp-20260912/report.md. Original EXE/DAT files remain read-only.
No README updates. Commit before testing and retain all outputs.

## Original Evidence

Skill start.ps1/open.ps1 opened session b348d386. IDA analyze_batch checked
429C70, 464E20, 464D90 and adjacent shutdown functions 465F70/470890.
429C70 follows real right-child nodes recursively, then frees the real node
while walking left. Nil flag is at +21; sibling 4634D0 uses +17. Payloads and
sentinels belong to the enclosing containers, not to the subtree erase helper.
464E20 releases texture handles from manager+68..72, clears actors via 463730,
empties the nonowning animation lookup tree at +40, frees owning animation list
at +52, clears priority tree at +88 and resets iteration state/dirty flags.
464D90 frees frame+244 payloads, 248-byte frame arrays and owning list nodes.

## Changes

actor_cleanup.cpp replaces four generated bodies behind their existing C
interfaces. Tree layouts have compile-time size/nil-offset checks. Correct
child/delete arguments replace &g1224; manager traversal no longer writes to
invented stack slots. Actual texture handles reach the existing compatibility
release helper. Existing null guards in priority-tree/list helpers remain.
Other legacy callers of the tree wrapper remain generated code; no claim is
made that all resource managers are fully reconstructed.

The game-window tool opens the process with query+synchronize access and checks
the final exit code after WM_CLOSE and termination. closed=true is accompanied
by normal_exit and the exit code; abnormal exit now fails the command.

## Validation Plan

- [x] Confirm the failure using original code and three existing dump stacks.
- [x] Replace the broken tree/manager/list cleanup in C++.
- [x] Commit source and run new quiet/diagnostic builds and contracts.
- [ ] Verify bounded gameplay/closure with final exit status and retain evidence.

Contracts build multi-branch trees of both node layouts, preserve sentinels,
exercise owning frame payload release and repeat manager cleanup. The complete
stage contract also clears its real Actor manager twice before exit. Use new
build-runs/shutdown-fix-20260912-{quiet,diag} and matching runtime-builds paths.
The independent Squirrel gameplay access violation is not yet claimed fixed.

Source 99af33e1dcf4016cbc5c3651b72918686af9adfe built in both variants and
passed CTest 3/3, including real manager clear twice. DAT staging passed.
The user subsequently reported a gameplay crash in shutdown-fix-20260912-diag
and requested focusing on that build. Its trace ends without exception context;
no corresponding dump was available. Do not claim game-level stability or
normal exit validation. Follow-up instrumentation is documented in
../first-fault-20260912/report.md. Original artifacts are preserved.
