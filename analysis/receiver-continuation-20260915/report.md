# Receiver continuation — 2026-09-15

Scope and E-imports carry forward from ../remaining-three-20260915/report.md.
Original game/DAT/save files unchanged; original PE32 SHA256 remains
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
IDA skill start/open restored the expired session as 4798af8a on a temp copy.
Evidence: original-0x*.json from IDA MCP; supplied Squirrel 2.2.2
sqvm.cpp::Execute/Suspend, sqapi.cpp::sq_wakeupvm and sqbaselib.cpp thread APIs.

## Thread chain

Restore the explicit VM throughout suspend, newthread, thread.call/wakeup/status.
Use C++ local SQObjectPtr ownership and assignment to the parent's _lasterror.
Retain the reconstructed VM constructor/vtable and live interpreter; do not
enable the source Execute backend. Reacquire parent VM after nested child calls.

Original Execute stores _suspended_root at +152, target at +156, traps at +160,
varargs at +164, and immediately returns on native suspension. Recovered code
had stored target at +152, cleared +156, and continued execution. Resume now
restores the suspended frame in the same current dispatcher. Remove the old
495360 implementation only after confirming its last caller was replaced and
there are no remaining function references (baseline source retains original).

Validation completed in fresh thread-20260917-r6 quiet/diag builds; see below. Existing
stone/lift/BGM fixes unchanged. Added coroutine result/error/vararg/trap tests.
All older artifacts preserved. User controls interactive game startup.

## Validation and remaining work — 2026-09-17

Final code/test commit: cbb9f6e. Both Win32 Release variants in
build-runs/thread-20260917-r6-{quiet,diag} pass all 18 CTests.
Runtime EXEs in runtime-builds/thread-20260917-r6-{quiet,diag} each have
three DATs staged beside them, verified by size/SHA256. See r6 logs and
r6-exe-hashes.json. Stage contract output/capture is disabled in both builds;
these tests do not replace interactive startup of the two runtime EXEs.

Failure history:
- 20260915 r1 (32f53d3): child stack corruption from main-VM selection.
  f885d8f scopes explicit child receivers and restores the previous VM.
- 20260915 r2 (f885d8f): second thread script failed; diag fastfailed 0xc0000409.
- 20260917 r3 (600a328): diagnostic revealed missing newthread binding.
- r4 (a674ec6): root still contained newthread, but its string refcount was -1.
- Original 48C580 retains the new name and releases the saved OLD name.
  RetDec released the newly overwritten fields instead, acquiring no net
  reference for a fresh name. Child Init re-registration destroys old closures
  and therefore invalidates names still held by the root table.
- 1ff1a08 delegates 48C580 to source C++ sq_setnativeclosurename. IDA evidence:
  original-native-name-20260917.json and original-init-20260917.json, session
  0dea0d99. Child Init really registers base functions; do not suppress it.
- r5 stage contract passed. r6 adds direct name ownership, same-name assignment
  and replacement checks; both full suites pass. Former fastfail not observed.

Tests also cover suspend/wakeup values, varargs, traps, child error transfer,
idle wakeup errors and forbidden suspension through nested native calls.
Existing stone/water, orange/green lifts, math and GC contracts pass.

NOT COMPLETE: receiverless 489F30, 489F50 and 4A9D70 still exist. Current
source has 396, 18 and 243 call-like text occurrences respectively, including
prototypes/definitions; these are NOT reachable-call or correctness counts.
The thread chain and obsolete 495360 dispatcher have been addressed, but
ReadCSV's by-value receiver, old compiler paths and cleanup thunks remain.
Do not cosmetically rename/delete stubs to obtain zero counts.

No game started, closed, switched or attached. Original game/resources and
prior stone R11/lift/BGM fixes preserved; all older test artifacts retained.
Interactive testing remains with the user: both variants, first stage, jump,
observe an enemy and exit. Pause after handing over paths as requested.

## ReadCSV continuation — 2026-09-17

User confirmed thread r6 works in-game before authorizing this continuation.
Code/test commit bf3461b; final builds csv-20260917-r3-{quiet,diag}.
Both variants pass all 18 CTests: stage_native_contract first, then the other
17 without repeating that test. Logs: csv-r3-*-stage.log and
csv-r3-*-remaining-tests.log. DATs staged and verified beside each EXE;
see csv-r3-*-dat.log and csv-r3-exe-hashes.json.

403000 now accepts the original four-word C callback ABI: path followed by
the three-word by-value SquirrelObject (vtable/type/data). 471160 already
passes this ABI. The C++ owner consumes the external reference through the
existing receiver-aware SquirrelObject destructor on success and failure.
Local fields/rows use SQObjectPtr RAII; new row tables retain reconstructed
vtable identity for mixed-VM GC. Raw table insertion mirrors 4A97B0 -> 48CB10,
without invoking an invented _newslot callback.

The C++ parser follows original 40C010 rather than a generic CSV library:
quotes toggle newline preservation, commas split even inside quotes, # starts
comment suppression, CR is kept only within quotes, and only an unquoted LF
commits a noncomment row. Comment suppression does not clear accumulated
fields, and EOF does not commit a trailing row. CharNextA preserves the original
ANSI multibyte traversal. Textual values retain the original 256-byte
strcpy_s contract; integer/float values use atoi/atof, and boolean means the
first byte is lowercase t. Definition names and types stop at the first empty
cell; a count mismatch fails before inserting rows. Duplicate row keys replace
rows; missing values default via the original conversions. Original MessageBox
captions retained for non-table, missing file and definition mismatch.

g874 controls replacement of the final four filename bytes with .cv1 and the
original rolling XOR decode (key 0x8b, step 0x71, step decrement 0x6b). Actual
bytes still come through function_407370 and the existing file/archive reader,
not a new data-directory bypass. Original reader implementations 414850 and
414930 are bypassed only in this recovered chain; they are not globally fixed.

Evidence: csv-{40bf10,40c010,40c2e0,40c370,40c4b0,40c790,40c6d0,414850,
414930,4a97b0,4a9d70,471160}.json from IDA MCP session baf41fe8. Carry forward
original-0x403000.json/original-csv.txt and prior E-imports/scope.

Validation covers parser quirks, typed and missing cells, duplicate rows,
no EOF flush, definition errors, both plain/encrypted file modes, the actual
471160 four-word callback, unchanged caller stack and external table ownership.
All existing thread, GC, moving-stone/water, lift and floating-point tests pass.
Failed batches preserved: csv r1 test source lacked statement line breaks;
csv r2 exposed C++ bool assignment choosing Squirrel's integer overload.
The final code explicitly constructs SQObjectPtr(bool); neither failed build
was delivered for interactive testing.

Progress: 807 lines of broken ReadCSV expansion replaced; 21 receiverless
4A9D70 call expressions removed from this function. The three fallback symbols
are STILL NOT eliminated globally. Current call-like text counts (including
prototypes/definitions) are 489F30=396, 489F50=18, 4A9D70=222. These are not
runtime reachability counts. Old compiler paths and remaining wrapper callers
need further analysis; do not claim full function coverage or equivalence.

No game was launched/stopped/attached. User startup testing of both final
variants remains pending (first stage, jump, see an enemy, exit). Pause after
handoff as requested. All prior builds, fixtures and logs retained.
