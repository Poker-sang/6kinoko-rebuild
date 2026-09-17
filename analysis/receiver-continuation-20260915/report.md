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

## Live compiler and assignment fallback — 2026-09-17

User confirmed CSV r3 works before authorizing this batch. Final code/test
commit 47ebabd. Both compiler-20260917-r3-{quiet,diag} Win32 Release builds
passed all 18 CTests (stage test first, then remaining 17). Existing stone,
water, lift, thread, CSV, math and GC checks pass. DATs staged beside both
runtime EXEs and verified by size/SHA256. See compiler-r3-*-stage.log,
compiler-r3-*-remaining-tests.log, compiler-r3-*-dat.log and EXE hash JSON.

Restored 48C1F0/sq_compile, 48D0B0/sq_compilebuffer and 4A15B0/Compile through
Squirrel 2.2.2 C++ Compile on the CURRENT VM. This preserves the actual shared
constant/enum table, error callback, last-error value and debug setting.
The bounded reader now carries pointer/offset/length, replacing the broken
stack-local feed state and hard-coded old executable callback address.
4A1B90/compilestring now returns -1 on error, +1 on successful closure push;
the RetDec version always returned +1. The output prototype is a full owned
SQObjectPtr instead of a single type word with a missing data field.

Closures still use the reconstructed constructor/vtable and existing game
Execute backend. Source prototypes are ordinary reference-counted objects.
The compiler's temporary literal/string tables and persistent enum tables use
the real upstream SQTable implementation. Its actual vtable is recognized by
the mixed collector (without changing the object's vtable). Source-created
objects are not silently ignored by GC. 48BB30's fake generated FS exception
registration was also removed from the simple allocation wrapper.

Evidence: compiler-{48c1f0,48d0b0,4a1b90}.json from IDA session baf41fe8;
supplied/vendored Squirrel 2.2.2 sqapi.cpp, sqcompiler.cpp, sqfuncstate.cpp,
sqclosure.h and sqtable.h. Prior E-imports/scope and original baseline carry
forward. No new game-specific behavior or alternative execution backend.

Removal audit:
- 47 obsolete compiler implementations removed after checking external
  references and transitive reachability in the candidate region. 49D700 had
  an external reference and was retained. See compiler-removal.json and
  prune_old_compiler.py (migration helper, not needed during builds).
- Unreferenced 48F380_legacy and 492370 removed. Unreachable code following
  4920A0's unconditional forwarding return removed; active comparator unchanged.
- These were the final old callers of receiverless function_489f50. Its fallback
  definition is now deleted. The correct function_489f50_this remains and uses
  C++ pair assignment. The preserved original 6kinoko.exe.c is reference-only.
- Current call-like source counts: 489F30=361, 489F50=0, 4A9D70=222 (including
  declarations/definitions, not runtime reachability). Thus ONE of the original
  three placeholders is eliminated; TWO destructor placeholders remain.

Tests check bounded source, user lexer feed and receiver, compiler-error
callback suppression/enabling with source coordinates, failure stack balance,
current-VM constants/enums across compilestring calls, closure invocation,
syntax-error catching and use after collectgarbage. Failed r1 had insufficient
line breaks in the test's nested source; r2 passed the compiler scenarios but
its expected caught error incremented the global unexpected-error test counter.
The final test marks that deliberate error as expected. Earlier artifacts kept.

Interactive startup remains pending for the new EXEs. No user game was
launched, closed or attached. As requested, pause after handing over both
variants for first-stage/jump/enemy startup validation. Original DAT/save
files and older build/test artifacts preserved.

## SQObjectPtr destructor fallback elimination — 2026-09-17

User confirmed compiler r3 works before this batch. Code/test commit: ffe3add.
Both destructors-20260917-r1-{quiet,diag} Win32 Release builds passed all
18 CTests. Three original DATs were copied beside each EXE and verified by
size and SHA256. EXE identities are recorded in destructors-r1-exe-hashes.json;
build, test and DAT logs use the destructors-r1 prefix.

IDA evidence at original 4CCC08 shows a constructor unwind chunk restoring
ECX from its parent's EBP, adding 18h and jumping to 489F30. Such chunks are
not ordinary independent functions. The removal audit checks external symbolic
and literal-address references, scans other source/header/test files, and keeps
the transitive closure of referenced candidates. See destructor-4ccc08-asm.json,
destructor-removal.json and prune_dead_destructors.py. Prior E-imports/scope
and supplied original reference remain applicable.

Of 540 candidate definitions, 533 unreferenced generated functions/cleanup
fragments were removed, deleting 4163 lines. Seven candidates were retained;
this is NOT a count of all remaining caller functions, because other signatures
were outside this audit's candidate set. No game behavior was added.

The final receiverless function_489f30 fallback and its declaration are now
removed. Together with earlier function_489f50 removal, TWO of the original
THREE placeholders are eliminated. function_4a9d70 remains: 36 matching source
lines including declaration/definition (text count, not runtime reachability).
Its remaining wrapper ownership/native-instance/global-cleanup paths need
further original-code analysis. Literal-address atexit references are preserved.
Correct receiver-bearing C++ destruction/assignment helpers remain in use.

Offline checks do not establish interactive startup or gameplay equivalence.
No game was launched, closed or attached. User testing of both variants is
pending: enter the first stage, jump, see an enemy, then exit. Pause at handoff
as requested. All previous runtime/build/test artifacts remain preserved.

## Native instance ownership and SQVM::Remove — 2026-09-17

User confirmed destructors r1 works before authorizing this batch. Final
code/test commit 497e533 (implementation 6622f02, fixture correction 5a77a1f).
Both native-instance-20260917-r3-{quiet,diag} Release builds passed all 18
CTests: stage_native_contract first, then the remaining 17. Original three DATs
are staged beside each EXE and size/SHA256 verified. EXE hashes are recorded
in native-instance-r3-exe-hashes.json. Logs use native-instance-r3 prefixes.

Original 4AB020 and 4AB170 now use native_instance.cpp. The original sequence
creates a class instance without executing its script constructor, retains the
complete instance object pair, creates a fresh __ot table, maps ClassType<void>
and the first size(__ca)-1 class tags to the native pointer, removes the root
and class stack entries, then attaches the pointer/release hook. Failure before
creation or while attaching restores the incoming top. C++ scope ownership
uses the existing external VM reference table release/reset operations, not
SQObjectPtr destruction for these wrappers. The ClassType<void> initialization
remains lazy at its original point. No game-specific conditions were added.

IDA 6a0354c5 evidence: native-4ab020.json, native-4ab020-asm.json,
native-4ab170.json, native-4a98d0.json, native-48c7f0.json. ECX values in the
assembly identify the previously lost table/instance/array receivers. The old
4AB020 expansion was removed, and 4AB170 is a thin forwarding boundary.
The older receiverless 4A98D0/4AA2D0 bodies remain unreferenced candidates for
later removal. This batch does not claim restoration of native variable getter
category 7 in retdec_get_var_value: that separate dispatch remains outstanding.
The recovered factory is exercised directly by the contract test.

New tests exposed an independent ownership defect in generated 4916A0:
after overwriting a slot, it checked the NEW type when deciding whether to
release the OLD value. It also omitted zero-reference virtual release. Original
IDA native-4916a0.json matches supplied Squirrel 2.2.2 sqvm.cpp::SQVM::Remove.
48AA60 now passes its explicit VM to that C++ implementation; the incorrect
4916A0 expansion is removed. Index conversion, shift bounds and final null
assignment follow upstream/original code, without a custom stack policy.
This fix affects the actual existing sq_remove API callers, beyond the factory.

Tests verify absent/nonclass lookup failure and stack restoration, exact success
stack size, returned instance having only its stack reference, stored native
pointer, void/base type map entries, exclusion of the last ancestry element,
empty/single ancestry cases, skipped script constructor, and exactly one native
release callback when the last owner is removed. Existing water/moving-stone,
lift, floating-point, thread, CSV/compiler and GC tests all pass.
Failed r1 used unsupported class-field assignment in its fixture; r2 corrected
the fixture and exposed the real stack removal reference leak. Neither is handed
over for gameplay. Both failed batches and their logs remain preserved.

Progress: 8 receiverless 4A9D70 call expressions removed in this batch.
Current call-like matching source lines: 4A9D70=28 including declaration and
definition (NOT runtime reachability). Still TWO of THREE original placeholders
eliminated; the last one remains in other wrapper/global/generated paths.
Prior E-imports/scope and original reference are carried forward.

No game was launched, closed or attached. Interactive quiet/diag startup remains
pending: first stage, jump, see an enemy, exit. Pause after user handoff as
requested; no claims of complete runtime equivalence from offline tests alone.
