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


## Further checkpoints: r24–r31

- d7600fa/19e3c57: callback references now use source Sqrat::Function lifetime
  operations, including script destruction. r24 and r25 passed 52/52.
- 85f3a4e: original C2DLayout registration uses source binding; original spelling
  is coS_z. r26 passed 51/52 because the new test script had invalid syntax.
- efd3356: removed 14 functions/6 globals (1181 lines) in the closed C2DLayout
  registry component. r27 passed 46/52; six moving-map tests crashed.
- 3ac6a8c: global Sqrat handles now retain external VM references rather than
  SQObjectPtr internal references. Necessary lifetime correction, but r28 still
  passed only 46/52. GC was not the sole root cause of those failures.
- 52431a9: the linker placed unrelated globals between separately declared
  handle words. Writing a pair overwrote a different handle. Six pairs now use
  explicit two-word arrays. r29 passed 52/52 with the same GC/registration order.
- 396f28e: original CActLayer registration uses source NoConstructor and binding.
  Removed invented layer thisAct field; original script-table thisAct remains.
  r30 passed 52/52.
- 572bac4: removed 9 functions/7 globals (643 lines) in the closed CActLayer
  registry component. r31 passed 52/52. All failed artifacts are preserved.

These are automated quiet-build checks, not additional gameplay observations.
Complete replacement is still pending: remaining registries, legacy shared
controls, owning container copies and CRT compatibility need further recovery.


## Further checkpoints: r32–r37

- 93b669f/c25ed99: C2DMapLayout registration now uses source Sqrat. Restored
  read-only left/right and original asymmetric fractional chip setters: f_left
  also truncates into left, f_top does not update top. r32 failed the shared new
  contract because adjacent Squirrel statements lacked newlines; r33 passed 52/52.
- 305bf9f: removed the retired map registry (22 functions, 10 globals, 1296
  lines). CActResourceChip uses source registration and native ChipInfo access;
  SetChipFlag preserves the original bit-zero-only condition. r34 passed 52/52.
- 988cd99: removed the chip registry (19 functions, 22 globals, 836 lines).
  CActResource2D/CActRenderTarget source property bindings replace detached
  instance fields. Original load/unload ABI, suffix probing and ownership are
  restored; the texture reader still maps DDS/BMP/PNG to CV2. r35 passed 52/52.
- 4fd8e23: six resource publication virtual entries now use source instance
  creation with explicit receivers. Ordinary slots permit the native name
  fallback; raw script slots reject an empty name. r36 passed 52/52.
- 02c8081: removed the texture registry (38 functions, 22 globals, 2595 lines).
  Original C2DLayout/C2DMapLayout registration virtuals use explicit receivers,
  distinct native wrappers and the original layer pointer aliases. Removed the
  redundant no-op layout value publisher. r37 passed 52/52.

DAT staging was verified for r33, r35 and r37. The user-owned r18 process is still
running (observed PID 17656); it was not closed and no new gameplay result is
claimed. IDA evidence for these entries is retained under r31/r33–r36.


## Further checkpoints: r38–r42

- 7e6ad94: removed 22 functions/15 globals (362 lines) in the retired resource
  publication component. r38 passed 52/52.
- b2ee5fc: ACT source compilation now uses real Sqrat::Script in the current VM,
  preserving that VM's constants/enums. Source Run shares its operation sequence
  with the embedding's explicit-environment entry. r39 passed 52/52.
- c3113b7: embedded bytecode also executes through Script; current VM receiver
  scope covers compilation/error callbacks. r40 passed 52/52.
- 6168f06: original ACT registration and local CompileFile use source operations
  and std::map for environment ownership. File compilation refreshes callbacks;
  original bytecode files execute twice (416A8D then 416AE8), inline bytecode once.
  The extension string is now a complete 28-byte record rather than separately
  linked globals. r41 passed 52/52, including those behavioral contracts.
- 38df4df: removed the old file-compiler component (5 functions, 1 global,
  180 lines). Layer construction uses source Table and empty Instance records,
  and script deletion uses explicit ECX receiver cleanup. r42 failed to compile:
  act_document.cpp needed the source Squirrel type header. af56250 adds it.

IDA's e8d7ff83 worker became unreachable; the required start/open scripts restored
original analysis as session 3c59c915. Restored evidence is under r41. No gameplay
claim is added for these batches; work has continued without closing the user's
r18 game.

## Further checkpoints: r43–r45

- af56250: r43 passed 52/52 after the missing Squirrel type header fix; DAT staged.
- d8ef184: removed the retired script ownership/registry component (9 functions,
  2 globals, 1179 lines). r44 passed 52/52.
- 845ca41: Input copy now retains its SqPlus object through source operations,
  deep-copies device/key vectors, and preserves the shallow device-pointer deque.
  Existing device vtables, allocator bytes and iterator proxy are preserved.
  The original device deleting virtual now receives ECX explicitly. r45 passed
  52/52, covering independent storage, wrap/growth, shrink, empty and self-copy.
  DAT staged with size/SHA256 verification.

The old Boost factory seeds are still source-reachable through ACT property
reader virtuals, so none were deleted on linker absence. r45 source-graph.json
records those paths. Original assembly evidence for Input and layer registration
is retained under r44/r45. No additional gameplay run is claimed.

## Further checkpoints: r46–r51

- r46 (0cc56d7) recovered original CActLayer registration order and resource
  publication virtuals. Build succeeded; the new fixture used the wrong embedded
  script data offsets. r47 (8854f36) corrected the fixture and passed 52/52.
- r48 (772995a) unified global/runtime ACT script registration through the source
  Sqrat path, including recovered callback order. Passed 52/52. User subsequently
  reported portrait mismatches in r45, r47 and r48; the old suite did not cover
  this visual correspondence.
- r49 (861ad1c) replaced ACT script read/write with native C++ streams, real
  Sqrat::Script compilation and sq_writeclosure. Corrected heap filePath cleanup.
  Build succeeded, but the new assertion script omitted a Squirrel statement
  newline and failed. No r49 test-pass claim.
- d0ed615 removed five audited unused Sqrat object helper bodies (94 lines).
- r50 (c623101) fixed the portrait regression introduced in r35 (988cd99): the
  recovered LoadTexture auto-size behavior requires the deserializer to clear
  resource byte +96, as original 446A84 does. Added original nine-portrait DAT
  coverage; corrected the r49 assertion newline. Passed 53/53; DAT staged.
- eb5447d recorded the root cause and lifecycle migration rules in
  portrait-regression-20260920.md before further migration.
- r51 (1041ae4) removed eight functions/four data records (501 lines), now
  superseded by the real script serialization path. The pinned c623101 source
  audit proves no modeled outside references; map absence alone was not used.
  Passed 53/53; DAT staged and verified. Launch through run_staged.ps1 displayed
  the title and then a Stage 4 transition. User input was detected, so UI input
  stopped. This is not a completed first-level gameplay validation and does not
  establish visual correctness across all transformation modes.

All batches, including failures, are retained. No user-owned game was closed.
Historical-source and migration-boundary checks passed after the r51 removal.

## Further checkpoints: r52–r56

The user confirmed the portrait fix in actual gameplay before these migrations.

- r52 (12edf87): CActResource2D property read/write now uses native C++ schema
  ownership and explicit stream receivers. Original 447570 preserves known
  descriptors, marks absent entries, consumes mismatched/unknown values, and
  reads values in sorted map order. Full and compact wire forms, heap strings,
  reordered headers and omitted fields are covered. Passed 54/54.
- r53 (87f181a): CActRenderTarget uses the same recovered wire operations but
  an independent schema, matching 4493F0/449AB0. Removed the closed texture
  schema component (2 functions, 3 data records, 396 lines). Passed 54/54.
- r54 (d997b99): chip resource properties use original ID/name/MCD-file fields
  (42F2A0); the texture auto-size transition is deliberately class-specific.
  Removed the retired render-target schema component (2 functions, 3 data
  records, 394 lines). Passed 54/54.
- 039c4c9: C2DLayout's 17 properties use native IO; original 42C084 sets the
  transform-dirty byte after reading. Removed the old chip schema component
  (4 functions, 4 data records, 408 lines).
- r55 (8c66930): replaced the reconstructed single-integer chip reference count
  with the real Boost 1.44 counted base and a native MCD destruction callback.
  This differs from the Actor slot's free-only deleter. Source/clone teardown
  tests confirm the final owner frees textures exactly once; weak ownership
  tests confirm no resurrection or second disposal. Passed 54/54.
- r56 (bcdc480): original chip loading virtual 42FAE0 now loads into a temporary
  source-owned MCD and replaces only on success. Preserves live clones, old data
  on failure, and original empty/default prefix and separator rules. The active
  ACT loader uses this same path. Removed the retired layout schema component
  (2 functions, 3 data records, 394 lines). Passed 54/54; DAT staged and verified.

The supported property wire types remain the existing loader's 0–3; malformed
input is bounded rather than following unsafe original allocation behavior.
No new gameplay claim is made for r52–r56. All tests are quiet variants; every
build and failed artifact from earlier batches remains available. Historical
source hashes and migration boundaries passed after r56. The overall migration
is still incomplete: remaining legacy paths are not declared replaced merely
because these batches pass.

## r57 and incoming PR updates

- r57 (a877ff7, including 0824d7e): C3DLayout reads use native schema ownership.
  Original trans/roll field aliasing is retained; the contract tests cover it.
  Removed the obsolete copy ABI shim and its spy-only test, the retired 3D
  schema (396 lines) and chip ownership factory (20 lines). Passed 53/53;
  the reduced test count reflects removal of the obsolete shim test.
  DAT files staged and verified. No gameplay validation claimed for r57.
- Merged incoming PR #7 commits through 374e709 without conflicts. These add
  guarded migration checkpoint automation and preserve source snapshots and
  baseline whitespace diagnostics. Both local migration commits are retained.
- Historical provenance (199 members) and migration boundaries (440 source/header
  files) passed after this merge. The operand-selection heuristic remains pending.

## r58: map serialization and preparation

Source commit 1077d77 includes 1e3b909. Recovered original 434760/434920/434A50
and 435860/435B20 through IDA against the original executable. Map property IO
now owns native C++ descriptors; original schema has nine entries (no blend).
Native std::sort replaces decompiled sorting and preparation builds flat caches
from the source MCD, ordered by unsigned chip ID. Record ordering uses signed X
then signed Y. Original bottom-bound initialization from last X is retained.
The source ACT destructor frees the new cache buffers, and clones reset caches.

The reader preserves original min(serialized_size,32) consumption and append
semantics, setting per-record ordinal, visible byte and alpha. Unsafe counts are
bounded; on a truncated record input existing records remain intact (deliberate
failure safety, rather than original partial append). Writer emits 12-byte
records after preparation and preserves original missing-resource behavior.

Pinned source-only audit retired-map-serialization.json proves removal of a
closed component: 18 functions, 4 data records, 1693 lines, including the old
Boost property factory and MSVC vector/sort implementations. It makes no linker
or runtime reachability claim. Other still-referenced map helpers remain pending.

Quiet Win32 build passed 54/54 tests, including the new map serialization
contract for signed ordering, sparse IDs, cache replacement, compact/full wire
forms, append behavior, truncation and empty extents. DAT staged and SHA256
verified. Historical provenance and migration boundaries passed. No new gameplay
claim; no user-owned game was touched. All batch artifacts are retained.

## r59: map binding and user validation

39cbcab includes ff9e200: the original 434380 map SetLayer virtual now uses
native C++ vectors and the source MCD records. It preserves one-shot suppression
at byte +460, type rejection, map record order, chip reference reset and original
texture-reference append behavior. The source MCD loader already preloads its
textures, so binding does not acquire texture handles a second time. Flat caches
reuse the r58 helper without the writer's record sorting or extent changes.
The existing active archive binding orchestrator is unchanged in this batch.

ed65a16 makes addressless RetDec records conservative audit roots, retaining
those records and all dependencies rather than inventing ranges. Initial audit
unit tests exposed an old fixture with its original-range comment before the
prototype, not beside the definition. ab82fa6 corrects the fixture; all 17 audit
unit tests passed. Both failed and successful test batches are retained.
The pinned source-only audit retired-map-binding.json retired 174 functions and
42 data records (10602 lines), with no outside symbolic or original-address
literal references under the documented lexical model.

r59 quiet Win32 build passed 54/54 contracts; DAT staged and verified. Launched
through run_staged.ps1 as PID 23428, with no subsequent UI input. The user then
requested that further runtime testing be left to them and explicitly reported
"R59没问题". Record this as user validation, not an agent-completed first-level
smoke test. Future binaries will be built/staged for the user; no further game
input or automated test runs are performed unless the user requests them.

## r60: dynamic CreateLayer2D

2c7f34c includes 1c3c9f5 and 5880b5c. CreateLayer2D now uses native C++ RAII
for its layer/key/layout allocations and the historical Sqrat source for root
lookup, instance publication and lifetime. The shared 42B4A0 constructor is also
used by the archive parser. Original locking, active gate, next layer ID,
parent table requirement, native ownership and publication order are retained.
Invalid-parent checking happens before allocation; failed allocation cleans up
through native destructors. The obsolete 4517C0 body (148 lines) and its external
declaration were removed after a pinned source audit. CreateLayerString remains
on its old path; CStringLayout is not silently replaced with a 2D layout.

The quiet Win32 build succeeds and DAT files are staged and hash-verified.
New dynamic_layer_contract source compiles and covers active/inactive creation,
long names, IDs, owned key/layout records and published property aliases.
Per the user's testing handoff, neither this test nor any runtime/game test was
executed for r60. The user subsequently confirmed "R60没问题"; this is user
validation, not an agent-executed test.


## r61: source-backed layer ordering

Source f9ee133 includes 8501614. GetLayerOrder and SwapLayer now bind through
native Sqrat closures using the upstream Squirrel API. IDA 451F30 preserves
inactive zero and missing -1. 451F80/455990/455A20/455C40 establish locking,
index bounds, ancestor rejection, child-vector rebuilding and root preorder.
Native std::map/std::vector own the temporary hierarchy; no decompiled STL or
recursive generated traversal is invoked. ABI child buffers retain native
ownership and are prepared before committing changes. Malformed cyclic,
missing-parent or duplicate-layer graphs fail without mutation, instead of
original unchecked traversal; allocation failures also leave order unchanged.

retired-layer-ordering.json removes five disconnected functions (129 definition
lines) under the pinned source-only audit. Shared helpers still called by old
CreateLayerString remain retained. The dynamic-layer contract source now covers
hierarchical ordering, subtree movement, ancestor rejection, bad indices and
inactive/missing results. It was compiled, not executed. The quiet build
succeeded; all three DAT files were staged and SHA256-verified. Runtime testing
remains delegated to the user.


## r62: native file enumeration ownership

Source 7a3b2b3 includes 3ded1c5. Original 452150/452220/452270/4522C0 and 455730
were recovered in IDA. ActingPlayer now stores an actual std::map owning Win32
find handles through RAII. IDs increment after successful FindFirstFile calls;
lookup failure returns false/null, explicit close erases even if FindClose
fails, and runtime destruction releases all outstanding searches. The four
script methods use upstream Squirrel calls and native Sqrat closures. The old
344-byte tree-node schema and manual tree cleanup are removed. The runtime's
+84 field now points to the actual C++ container; it is not an upstream vtable
placed over legacy bytes. At ID wrap, replacement also closes the prior handle
instead of leaking it. Constructor contracts now check observable enumeration
and cleanup behavior rather than old red-black-tree node bytes.

retired-file-enumeration.json proves deletion of 19 functions, 1154 definition
lines. Quiet build and DAT staging succeeded; tests were compiled, not run.

## r63: key and 2D layout clone virtuals

Source bf2cba5 includes 6137c1d. Original 4265E0 now has an explicit receiver,
RAII allocation and length-preserving native string copying, including embedded
NULs. Layout cloning still dispatches through the source layout's virtual; a
null clone result remains a key with null layout, matching the original.
42B580/42B5C0/42B3F0 confirm C2DLayout copying covers offsets 8..312 while both
constructor vtables remain intact. Trailing layout/key padding is not copied.
The archive clone orchestrator remains separate in this batch.

retired-key-layout-clones.json removes four functions (95 definition lines),
including the obsolete constructor and the key clone's receiverless 406CC0
call. Other 406CC0 callers and the final memcpy operand heuristic remain open.
The dynamic-layer contract adds deep string/layout ownership and exact copying
bounds. Quiet build and DAT staging succeeded; test source compiled but no test
or game was executed by the agent.


## r64: resource clone ownership

Source ca1ba87 includes 92dd8a7. IDA confirms 42F9D0/42FA50 copies chip ID,
three strings and shared MCD ownership; 446AF0/446B70/449B70 copy texture fields
and mark cloned resources as borrowed. These virtuals now allocate through
native RAII, use explicit receivers for length-preserving strings, and share
MCD data through the actual Boost 1.44 control used by archive clones. Native
texture references are retained to match the reconstructed handle store.
The common resource destructor now frees the base name, which the earlier
source implementation omitted for heap-backed names.

retired-resource-clones.json removes seven functions (153 definition lines),
including old constructors, string assignment callers and manual Boost count
manipulation. Contracts cover virtual/whole-ACT sharing, independent long names,
texture/render-target type and crop preservation, and final handle release.
Quiet build and DAT staging succeeded; contracts were compiled, not executed.
Explicit unload of the new borrowed texture clones needed the r66 follow-up.

## r65: disconnected source-replaced property bindings

Source d6d8519 removes 21 obsolete Sqrat property binders and string getter
helper functions (348 definition lines). The pinned source-only audit
retired-disconnected-bindings.json covers CActLayer, C2DLayout, map layout and
texture property paths already registered through source-backed bindings.
No outside symbolic or interior-literal references were found. This is removal
of replaced library scaffolding, not a claim that remaining active templates
have been replaced. Quiet build and DAT staging succeeded; no tests executed.

## r66: explicit unload of borrowed native clones

Source 1e4c172 fixes a lifecycle boundary found during source review of r64:
the original borrowed flag suppresses an original owner release, but the native
clone also retains a handle-store reference. Track that additional reference in
a mutex-protected C++ owner set. Both explicit Unload and destruction consume it
once; ordinary borrowed fixtures keep their previous behavior. Failed retains
and allocation cleanup remove pending ownership entries. The new contract
source unloads twice, then destroys the clone and verifies final release through
its remaining owner; it was compiled, not executed.

r66 quiet build succeeded and three DAT files were staged and SHA256-verified.
The user can test r66 cumulatively; earlier binaries and logs remain retained.
R61-r66 together retired 56 functions (1879 definition lines). No game or local
automated test was run after the user's handoff. Overall replacement remains
incomplete: CStringLayout, active property-template readers, remaining explicit
string receivers/operand heuristic and CRT/RTTI compatibility still need work.


## User confirmation after r66

The user reported "已确认无误" after the r66 delivery. Record this as user
validation of r66; the agent did not execute gameplay or automated tests.


## r67: CActLayer clone virtual and key cleanup

Source 69f1280 includes 155a217. IDA 41EA50/41ECA0 confirms constructor-owned
layer storage, independent child-vector allocation with shallow parent/child
links, deep key/event cloning through their virtuals, SetLayer on main keys
only, and source-backed Sqrat Table/Instance reference assignment. The new
act_layer_clone.cpp uses native RAII and standard containers; it never executes
the old receiverless string/vector/list assignment scaffolding. This changes
the standalone clone virtual, not the archive clone orchestrator.

41EA50 obtains script text through 415EA0 and writes it through 415F60 after
assignment. IDA disassembly resolves the apparently constant zero length in
pseudocode: it is the returned string's length loaded at 41EAEE. Plain scripts
copy through their first NUL; compiled scripts yield the exact original 63-byte
comment, retain the compiled byte and set the dirty byte. The destination script
filename is cleared. Callback environments retain constructor state. Malformed
unbounded script buffers and list cycles are rejected instead of unchecked
original reads/traversal; partial clones are destroyed through native owners.

Source review also found the native key-list cleanup freeing key+24 (the string
length) instead of the heap address at key+8. A shared key destructor now frees
the correct buffer and is used by both list cleanup and failed clone insertion.
The existing dynamic-layer fixture already exercised long names; its source is
extended with main/event lists, child-vector independence, layout rebinding,
shared Sqrat pairs, both script modes and source survival after clone teardown.

The pinned source-only retired-layer-clone.json audit removes 27 functions and
six data records (750 definition lines), including 416700 and 41ECA0's old string
assignment callers and disconnected binding dependencies. No outside symbolic
or interior-literal references were found under the documented audit model.
Quiet Win32 build and DAT staging succeeded; all added contract source compiled,
but no test or game was executed by the agent, per the user's handoff.

Lessons: track owning pointers separately from length/capacity in legacy byte
records, and exercise destruction of heap-backed strings, not just successful
construction. For decompiler constant arguments, inspect the caller's actual
push/register sequence before translating: temporary-string lengths can be
lost even when the callee is understood. Original odd compiled-script behavior
is preserved rather than replaced with an inferred intent.

## r68: restore instance dispatch and retire legacy wrappers

IDA 452010 stores its argument at ActingPlayer+76 and returns true. The old
4556C0 reconstruction omitted the indirect member call, returning the status
from sq_getinstanceup instead. SetRenderTarget now uses a source-backed Sqrat
closure and real Squirrel instance conversion, stores the borrowed native
pointer, and accepts null to clear it as the original zero-initialized argument
does. It neither retains nor destroys the target. The dynamic-layer contract
now checks both the actual stored pointer and null clearing.

The already-recovered integer method bridges (445530/455330) moved to native
C++, retaining the existing trace calls and explicit thiscall ABI boundary.
They remain in the ACT module to avoid introducing game-host dependencies into
the standalone Squirrel API contracts. Audits retired-instance-dispatch.json
and retired-integer-dispatch.json verify deletion of six old functions,
including two disconnected conversion-only wrappers.

Source edac6e1 built successfully as quiet Win32 r68. Contract sources compiled;
no automated tests or game session were executed. Three DAT files were staged
and hash verified. Overall replacement is still incomplete.

## r69: original CActTimeLine ownership and serialization

IDA 4255E0 identifies the second layer-list binder as CActTimeLine, not CActKey.
4253D0 allocates 28 bytes, copies beginTime/timeLength at +4/+8, and deeply copies
an eight-byte-pair vector at +12/+16/+20. 425350 confirms the two integer schema
offsets. 425420 appends serialized pairs; 425510 writes count then both words;
420900 frees the vector. Its raw RTTI name hashes to 9902F2C0.

Native timeline methods now use std::map for the property schema and std::vector
for bounded read/clone staging, retaining only the flat buffers required by the
original object ABI. Layer loading accepts this original second-list type;
previously every nonempty second list failed. Both archive and standalone layer
cloning preserve its distinct size and deep storage. Cleanup recognizes the
timeline before treating +4 as a layout address. Reader batches reject malformed
sizes and preserve the existing vector on truncation rather than partially
appending. This safety difference does not alter valid-file layout/order.

The shared property reader/writer now correctly assign known bool bytes and
write bools as one byte. Existing texture/layout schemas contain no bools;
this prepares the remaining layer/property migration without changing them.

Correction/lesson from r67: its second-list fixture used a synthetic CActKey,
which tested clone dispatch but did not establish the actual serialized type.
The fixture now uses a real timeline with nonzero beginTime, independent pair
storage, full/compact wire forms, append semantics and truncation checks. Never
infer a polymorphic list's element layout from the other list or its C helper
name; recover its factory/binder and allocation size first.

Quiet Win32 r69 source 7591a5d built successfully and DATs were hash verified.
Tests were compiled only; no game or automated test execution was performed.
This does not claim that the remaining generic factory or CStringLayout is complete.

## r70/r71: actual upstream Boost hash_range

Boost 1.44's original functional/hash headers and their dependencies are now
retained from the same SHA256-pinned release archive as the counted base (198
manifested Boost members in total). The vendoring tool can refresh one release
and permits only the same CRLF/LF checkout conversion as verify_upstream;
other local modifications still fail closed. UPSTREAM.json records exact
archive-member hashes. No Boost source was patched for this migration.

The native Win32 bridge calls boost::hash_range<char const*> directly. All
407210 call sites now use it and retired-boost-hash.json proves the old loop
can be deleted. Its callers with incompletely recovered argument plumbing,
including 405990, still require migration; replacing hashing does not repair
those callers. Byte ranges, signed char behavior and 32-bit size_t are retained.
The compiled-only contract adds original C2DLayout/CActTimeLine IDs, an empty
range and a binary range containing a negative char and embedded zero.

R70 (688300a) failed at compilation because C++17 removes std::unary_function.
R71 (e027d10) enables MSVC's _HAS_AUTO_PTR_ETC solely for boost_hash.cpp, using
the genuine standard-library compatibility declaration rather than a shim.
Its quiet Win32 build succeeded and all three DAT hashes were verified. R70's
failed artifacts remain intact. No automated tests or game sessions ran.


## r72/r73: native ACT writers and removal of the frame-copy heuristic

Layer (41F990), key (426740), and CStringLayout (43FBE0) writers now use native
std::map schemas and actual Boost 1.44 hash_range for nested types. They preserve
original list order, one-byte booleans, and ignoring individual layer-list
writer results. CStringLayout's serialized alignment aliases addEdge at +128:
43F97D uses the bool descriptor despite the runtime alignment integer at +132.
The collapsed CStringLayout vtable and text renderer are NOT fully migrated.
retired-act-writers.json proves removal of four functions and three data items.

Actual file/package read virtual slots now use explicit ECX adapters. The
compiled-only timeline fixture covers real Windows file handles and encrypted
package payloads, not only the fake memory stream. R72 built the main EXE but
failed the standalone method-entry contract link because the new adapters
introduced game-host dependencies into the common ABI library. Their final
location is the ACT implementation. R72 artifacts remain; no test ran.

IDA 406CC0 is substring assignment with receiver, source, position, count.
The new native StringView operation preserves self-assignment capacity and
terminator, clamps counts, and explicitly receives all four arguments. The
three remaining template callers at 4203B8, 427048 and 429498 copy var_54 to
var_38; 445327 copies record+368. Their surrounding legacy functions still
need migration and are not claimed repaired by this local argument correction.
415315 terminates with a noreturn bad_alloc throw: RetDec erroneously attached
an unrelated following string constructor. That bogus tail is removed; abort
prevents falling through if the old exception compatibility entry returns.
Invalid substring positions leave the target intact, following existing native
string adapter policy rather than throwing through obsolete exception metadata.

retired-substring-assignment.json audits removal of 406CC0 and its private
4067F0 erase dependency. With the only production caller gone, _memcpy2, the
EBP capture adapter, frame candidate scanner, and their four obsolete test
variants are deleted. New substring contracts cover independent ownership,
embedded zeros, bounded counts, self-assignment and retained heap capacity.
No VM trace call site is removed by this change.

R73 (8b0ba35) completed the full quiet Win32 build, including all remaining
contract targets; its three DATs were staged and SHA256 verified. No automated
test or game session was run. Overall library replacement remains incomplete.
