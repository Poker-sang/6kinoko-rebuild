# Keystone riding: restore Actor::SetStep entry

Follow-up to 3e5e8c4. The user reports a crash when stepping onto Ten's
keystone. Neither game executable may be launched. Restore original behavior;
do not change scripts, collision rules, death rules or ability parameters.

## Scope and workflow

Read AGENTS.md, reverse-engineering and ida-reverse skills, precedent/tool
references, the RE workflow and synthesis decision checklist. Carry forward
unchanged target triage/imports from ../stage-entities-20260907/report.md
(E-triage / E-imports). Original PE32 SHA256:
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
The stale 7ca49e69 worker was unreachable. Skill start.ps1/open.ps1 opened
a temporary original copy as 23327f9d. IDA MCP queries use ida_query.ps1's
HTTP transport because the registered client transport was closed.

- [x] Resolve latest fault using the matching pre-rebuild linker map.
- [x] Compare original SetStep, adapter assembly and packaged CallbackStone.
- [x] Reproduce the ABI failure without launching either game.
- [x] Restore SetStep entry and verify native binding/lifetime contracts.
- [x] Exercise original stone callbacks with real native Actors and PATs.
- [x] Build/test diagnostic and no-log variants and stage the three DATs.
- [x] Source, tests and this report are included in the scoped backup commit.
- [ ] User gameplay confirmation; startup smoke is not run by the agent.

## E-crash-map

The diagnostic trace last modified 2026-09-07 12:33:40 is 1,421,524,181 bytes.
Read its tail, not the whole file. No retdec_crash.dmp exists beside the EXE.
The pre-rebuild diagnostic EXE still hashes to
F002197EF57D9B0BFB1D161BB164C155A432DCD8199050B9342CC41229BC4739.
Its linker map timestamp is 12:03:17.

Latest fault: C0000005, EIP 005780DC, runtime image base 00480000,
RVA 000F80DC. The preferred-image address 004F80DC is **g16**, the
SquirrelObject vtable, not code. The following stack words are 0A008000,
0DE598B0, then a return address. Together with the faulting address these
are exactly the three by-value SquirrelObject words left on the native stack.

Relevant saved frames, normalized to the preferred 00400000 image:

| Address | Symbol |
| --- | --- |
| 0048F1D1 | retdec_execute_call_native+AD1 |
| 0048BE4E | retdec_clean_vm_call+1FE |
| 0048FB11 | retdec_execute_clean_vm+631 |
| 0046ED99 | function_497680_this+139 |
| 0045FA9F | function_48ace0+BF |
| 00485701 | retdec_actor_collision_callback+81 |
| 00451603 | function_462ce0+93 |
| 00485AEC | retdec_actor_manager_update+AC |

## E-original / E-ABI

Original 4606F8 saves the Actor receiver from ECX; 4606FC takes the address
of the by-value object on the stack. For an instance/userdata it obtains
the native Actor, copies its +24/+28 handle to receiver +32/+36 with weak
ownership, and assigns script property step. Other types clear the weak
pair and assign null. 4607C3 destroys the incoming SquirrelObject and
4607D9 returns with **ret 0Ch**.

460B50 obtains the receiver and member-function pointer, constructs the
12-byte object at ESP, then 460B99 invokes that method with ECX. The rebuilt
adapter uses retdec_call_thiscall3_result, whose final ret requires the
callee to have removed those three words. The reconstructed public
function_4606d0 was only return 0; the recovered explicit-receiver _this
implementation already existed but only collision internals used it.

Repair: an x86 entry passes ECX and the address of the by-value object to
the existing _this implementation, then returns with ret 12. No change to
the helper's binding logic, the shared call adapter or the VM is needed.

Squirrel 2.2.2 sqvm.cpp::CallNative and its source-build disassembly in
analysis/evidence/raw/phase3-20260906/squirrel-sqvm.asm (CallNative+2F8..304)
confirm that the VM calls a cdecl native adapter. The adapter's inner Actor
member invocation is a separate thiscall boundary. Altering the VM's stack
cleanup would therefore fix the wrong layer.

Original xrefs include Actor registration at 461038 and collision calls
at 468B9C/469349. IDA annotations document the recovered ABI at 4606D0 and
460B99. Production DATs and the original executable are unchanged.

## E-offline-regression

tests/stage_contract.c includes the real reconstructed code but has no game
startup, graphics, audio service or window creation. A test-only assembly
probe saves/restores ESP to measure the member entry without jumping into
data on a failed return. Before the repair it reports:

```text
SetStep ESP delta: -12 (expected 0)
FAIL line 540: stack_delta == 0
```

After the repair the same probe passes, including the actual native binding.
64 script-driven bind/rebind/replace/detach cycles verify the registered
adapter, script step identity, native weak control counts, object reference
counts and VM stack stability. Existing 32-reset tests continue to pass.

The extended asset test reads item.pat and bullet.cv4 through original
archive readers. It executes actual InitStone, CallbackStone and UpdateStone
with a real native player Actor and the previously loaded player PAT. Only
the player's minimal initializer, camera bounds and media sinks are fixtures.
No replacement SetStep or stone callback is installed.

Eight placements alternate direction and execute the actual collision-pair
dispatcher, checking user.ride and both native/script bindings. Each follows
eight motion frames with the real GP_LIFT collision path. Four walk off,
verify automatic detach and the original falling transition/sound 35.
Four release the stone while bound, verify native weak-handle expiry, update
the rider and detach through SetStep(null). Final clears verify weak-reference
expiry and native instance cleanup. An initial fixture assertion tried to
access player.user after destruction; it was corrected to retain the user
table before clear, consistent with original instance cleanup.

## Verification and handoff

Both Release builds pass archive_smoke and stage_native_contract (2/2 each).
Both also pass the extended checks for w1-c01a, w3-s01a and w7-s01a,
including the original stone callback/PAT cases on every invocation.
stage_dat.ps1 copied and SHA256-verified all three DATs beside each EXE.
Saves/configs and unrelated worktree edits were preserved. Neither original
nor rebuilt game was launched. IDA annotations were saved to
C:/rs-ida/db036b27-6kinoko.exe.i64.

Final EXE SHA256:
- Diagnostic: C13A184B7DB380057BAA6145257B3686CB247E271E4D8946734EDC4EAB046ED9
- No-log: 209F942274FF9C9F1CAE10F85E6922AA19E5AB7A82A98477CAFC9B401FD00E66

Reproduce extended tests from repository root:

```powershell
runtime-builds/p3-save-worldmap-diag/tools/kinoko_stage_contract.exe analysis/stage-entities-20260907/block.cv4 analysis/stage-entities-20260907/stage.cv4 analysis/actor-animation-bounds-20260907/marisa.pat 150465603 analysis/player_ground.cv4 analysis/player.cv4 analysis/constant.cv4 C:/WorkSpace/6kinoko data/map/w3-s01a.act
```

Use the no-log variant and w1-c01a/w7-s01a for the other checks.
Confidence is high for the captured stack fault: original assembly, the
fault's object words and the before/after ABI regression agree. Actual
gameplay and startup are deliberately left to the user, not claimed tested.
Unrelated in-stage or exit-time crashes are not claimed fixed by this change.

User test: in Ten form place a stone, jump/land on it, stand and move on it,
jump off or walk off, and repeat in both directions. Re-place after release,
then leave/reenter the stage. Compare diagnostic and no-log executables.
If a fault persists, retain the diagnostic trace/dump and note the exact
action (landing, standing, leaving or stone disappearance).

P0 synthesis: claims have static and offline execution anchors (R41/R4*);
the stack-cleanup hypothesis is confirmed (R2); live gameplay remains outside
this turn's execution scope (R7). This is ordinary game reconstruction, not
malware analysis; no IOC claims or external scope were introduced (R8/R23).
