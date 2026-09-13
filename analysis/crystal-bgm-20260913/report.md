# Blue crystal countdown

Scope: restore original countdown cadence without changing original audio/assets/timing. Binary identity/imports carry forward from map-visibility-20260913. IDA MCP ec2d2f0e survey confirms the same original native x86 SHA256.

The effective stage.cv4 comes from archive 2 (the archive probe applies original override order). SetSwitchBlue initializes a 900-frame counter and 44 timestamps. UpdateStage compares the last timestamp against elapsed milliseconds, plays SE 120, then pops the timestamp. Other SE cues (42/43/119) remain separate. This is a scripted SE cadence, not BGM playback frequency.

Original IDA 48DD10 / 48DAC0 and supplied Squirrel 2.2.2 sqapi.cpp::sq_arraypop, sqarray.h::Pop show the explicit array receiver, value push and vector pop/shrink. The generated 48DAC0 has an uninitialized receiver. Native 4A25A0 also returns 1 unconditionally instead of SQ_ERROR on pop failure. Native top still relies on the ambient VM helper instead of its supplied receiver.

The new offline --crystal-countdown probe executes the original packaged stage script and captures all SE-120 calls over 900 frames. It independently checks them against the original script's generated timestamp array, without substituting a new timer or sorting/fixing any source timestamp.

## Restoration and validation

Baseline c12050b, crystal-countdown-before-20260913, exits with c0000005 during the original script's pop call. The retained map places the failing RVA 0x28e2 in inlined native pop code. Unlike normal gameplay's accidental stack state, the isolated call exposes the invalid receiver immediately.

Runtime correction 1ebcb67 uses the supplied Squirrel 2.2.2 sq_arraypop API in C++, with explicit VM/index/push-value arguments. This invokes the source SQArray::Pop and vector shrink/destruction. It also restores native pop's SQ_ERROR result and source array_top with the explicit VM. Invalid dead 48DAC0 C was removed. SQObjectPtr/SQArray/VM field offset assertions protect the existing x86 layout. The supplied source-build SQVM::Push disassembly at analysis/evidence/raw/phase3-20260906/squirrel-sqvm.asm (Push symbol near line 12941) confirms the explicit receiver, top increment and ownership assignment used by the source implementation.

No BGM/DirectSound code, source timestamp, counter, sound ID or asset was modified. Existing VM tracing and diagnostic switches remain. The full experimental C++ Execute backend stays disabled; this is a narrow source-library replacement.

Candidate r1's extra array fixture encountered the known old compiler restriction on adjacent declarations after a table literal. fb120a5/r2 separates statements. R2 already matched all 44 countdown frames, but its deliberately caught empty-array errors were counted by the test observer as unexpected VM failures. 7b2872a/r3 scopes the existing expected-error flag to those two calls and checks their exact original error strings. The runtime implementation is unchanged across these fixture corrections. All candidates, staged DATs and logs remain.

Final source checkpoint 7b2872a. Both r3 diagnostic and quiet builds pass CTest 7/7, including the original stage script countdown and the preceding damage-pause/texture-lifetime contracts. Generic array tests verify top without removal, mixed scalar/reference values, object lifetime, self-reference, empty pop/top error returns, 512-element shrink order, and API pop with/without returning the removed value. The quiet original-DAT enemy reentry regression also passes.

The original effective stage script (archive 2 override) produces exactly 44 SE-120 events over 900 UpdateStage calls. Every captured frame matches its timestamp array; first events are at frames 31,91,150,209 and the last events at 859,862,865,868. Thus the initial interval is 60 frames and final intervals 3 frames. The original unusual interior timestamp is kept, not sorted or corrected. At expiry the schedule is empty, counter zero and blue-switch state toggled as the script specifies. Full trigger output is in r3-countdown-frames.log. Sound output is intercepted for measurement; this is not a live audio recording.

Three DATs are copied and hash-verified beside each final EXE. The existing damage-pause quiet save is copied without altering the source. Per-run validation.json records the source/hash/results. Actual gameplay/audio listening remains with the user under the session's earlier preference; no game was launched.

Checklist: original script identified via archive override rules; IDA/source/disassembly contracts compared; invalid receiver reproduced; source-based C++ repair; full original countdown timing and generic ownership/error tests passed; both build modes passed; DATs staged; saves copied; prior products preserved; source committed before test batches.
