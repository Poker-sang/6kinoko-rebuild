# Production source replacement, 2026-09-20

This continuation implements source replacement after the earlier acceptance-only
checkpoint. It does not claim that all remaining legacy ABI compatibility code
has been removed.

## Binding operations

Production now executes Sqrat BindFunc, Object getters/destruction and Function
execution, and SqPlus CreateArray, integer-key string SetValue, NewUserData,
GetUserData/RawGetUserData, GetTypeTag, SetDelegate and CreateInstance through
normally constructed upstream objects. The old operation bodies are deleted;
remaining wrappers marshal unaligned host records and preserve the recovered
return values, reference transfers and error policy. No upstream vtable is
installed in a byte record. Three generated Sqrat virtuals (4029F0, 402A10,
453580) are deleted and nine legacy vtables point at typed x86 entry adapters.

IDA evidence: original 415810 pushes closure then environment, calls 48ACE0
with one argument/no result/the byte at 5109DB, then pops one slot. The source
Function body performs that same sequence; the host passes its existing scoped
call entry and error flag without changing global Sqrat settings. Original
4029F0 and 453580 use ECX receivers and callee stack cleanup, tested by invoking
the replacement entries through thiscall function pointers. The instance factory
uses the source exception path but catches it before the recovered C boundary.

## Retired audio implementation

The actual BgmTrack owns VorbisDecoder; its State destructor executes upstream
ov_clear. BufferRecord is a request record, not an OggVorbis_File or a C++
decoder. The private manager never dispatches through the old handle/decoder
vtable words. Removing their assignments and host exports disconnects the
superseded implementation. The unused exported handle-table constructor is also
removed; native record sizes and used field offsets stay statically checked.

[Closed-component evidence](retired-audio-island.json) records source commit
f19c0aca93172fe3c172a334ae8378b50aa00c9b and both matching link maps. The audit
includes named functions, all retained source roots, tables, address-taking,
exact numeric addresses and interior literal references. It finds no external
inbound references to the selected component: 33 functions, seven data records,
1,595 definition lines. 27 nodes were still retained in those maps. All selected
bodies, prototypes and tables are now deleted, including the old ov_clear path
47AED0, codec teardown helpers, retired BgmBuffer/handle-manager virtuals and
their private support. The immutable original decompilation remains available.
This is a source-graph boundary proof, not proof for arbitrary computed pointers.

## Validation checkpoints

- 8c54ad6: r2 diagnostic/quiet builds, 52/52 CTests each.
- 56417b8: r3 diagnostic/quiet builds, 52/52 CTests each; Sqrat virtual ABI cases.
- f19c0ac: r4 diagnostic/quiet builds, 52/52 CTests each; retired audio identities.
- 4a7b21efdb84c5975a62e9a499fabecf4c1f6422: independent r5 diagnostic/quiet
  builds, 52/52 CTests each. DAT copied and SHA256-verified beside each EXE;
  run_staged reports WORKING_DIRECTORY unset and EXIT_CODE 0 for both. The
  assistant observed first-level frames with airborne actors and monsters in
  both variants. User operated the game and separately confirmed both normal;
  injected jump/close actions were blocked by concurrent user input, so they
  are not claimed as successful automated actions. Quiet produced no trace.
  All artifacts remain under their original build-runs/runtime-builds paths.
  Hash index: [r5 artifacts](source-replacement-artifacts-20260920.json).
- Following the user's updated instruction, future local and Windows CI
  verification uses the quiet variant only. r5's already-generated diagnostic
  artifacts remain retained; this does not require another dual run.

Remaining game-specific Sqrat/SqPlus registration, native descriptors, legacy
Boost blocks and CRT exception/RTTI compatibility are separate migration work;
this change does not represent those adapters as upstream implementations.

## Further source migration (continuation)

- f4e5eb6/e125388: Actor ownership uses actual Boost counted objects; the manual
  fallback and retired control vtable were removed. r7 quiet: 52/52.
- e4560fe: source SqPlus native-instance/hierarchy/function factories. r8: 52/52.
- 8351dc0/94a19e4: actual ClassType descriptors replace fabricated descriptor
  records. The pinned closed-component audit removes 11 functions and 8 tables.
  r9 quiet: 52/52.
- c9054f2: source VarRef constructor registers metadata using a borrowed root.
  r10 quiet: 52/52.
- 94aa6af: source variable creation/handlers/string reads. r11 quiet: 52/52.
- 085c2e1: source getVarInfo metadata lookup. r12 quiet: 52/52.
- 002cc97: IDA recovered explicit Actor/Camera/MapManager copy receivers. The
  receiverless SqPlus assignment implementation is deleted. Actor assignment
  retains/releases actual Boost controls rather than manually editing counts.
  r13 quiet: 52/52, including real-VM Actor assignment/self-assignment with
  strong/weak counts, object references and the untouched +372 field checked.

Each batch is isolated under build-runs/pr7-complete-20260920-rN-quiet and the
matching runtime-builds directory, with source-commit.txt and configure/build/
ctest logs preserved. See the later r18 user-run observation below.

Remaining work includes old Sqrat registration/constructor paths still referenced
by game vtables, additional recovered property-control tables, and incomplete
CRT/game-copy compatibility. An attempted dead-code audit correctly rejected
4A95C0 because game copy functions still referenced it; those callers were fixed,
not ignored. Input's original ClassType copy calls 46EBD0 (IDA), so the retained
no-op game callback is not evidence of a completed copy implementation.

## Further checkpoints: r14–r23

- c2e1327/ee860c9 through 758ef20: source Sqrat value/function binding,
  property dispatch, weakref and scalar conversions. r14 failed two lifetime
  tests; 4f58186 fixed the borrowed-object copy ownership and r15 passed 52/52.
  r16 did not compile; f34c3aa corrected class specialization/dependent-base
  lookup. r17 and r18 each passed 52/52 in quiet builds.
- f6b000b/ae5dce8: source class creation and removal of 28 disconnected property
  control tables (retired-property-controls.json). r19 quiet passed 52/52.
- e553af2: source SqPlus instance-storage selection replaces the host's
  static/constant/member-offset branch. r20 quiet passed 52/52.
- 70e2257: native instance creation shares source ClassType::PushInstance's
  operation sequence. r21 quiet passed 52/52.
- 69962f9/68b8e62: source InitClass registration replaces four repeated host
  registration blocks. IDA 421734/421785 confirms static property-table slots.
  r22 passed 51/52: its new test incorrectly tried to change a locked class;
  Squirrel 2.2.2's sq_newslot return did not prove the slot had changed. The
  corrected test checks that an instance cannot overwrite the static table.
  r23 quiet passed 52/52. Failed artifacts are preserved.

The user launched the staged r18 quiet EXE and confirmed it worked. An observed
frame showed an airborne actor and monsters, but was a later level. No automated
jump, first-level verification, or clean exit is claimed for that run. The
existing r18 gameplay.log records the earlier launch blocked by r5's named mutex
(EXIT_CODE 1); it is not evidence of the user's subsequent successful launch.
The user chose to close old games themselves; no game was forcibly closed.

Original disassembly evidence for the next work is retained in r20:
4517C0 (CreateLayer2D takes ECX plus a by-value old string and another argument),
420A90/4216A0 (class initialization), and 46D750/46E530/46D2A0 (Input's nested
container copy). These are still migration work, not completed functionality.
