# Eight entry audit and C++ migration — 2026-09-14

## Scope and evidence

User asked to repair the eight simplified-return candidates, then explicitly
requested C++ migration where feasible. Original PE32 SHA256:
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
E-imports is retained in ../stone-posture-20260914/report.md (file/windowing,
D3D/input/audio/COM imports; no static network/crypto imports). IDA MCP session
30d91b89 exported the eight original functions and adjacent403E50/48C400 here.
The supplied Squirrel2.2.2 sqobject.h, sqclosure.h and sqclass.cpp corroborate
ownership and destruction. No original DAT or save was modified.

The inventory's eight are source-text candidates, not eight absent features.
All eight already had receiver-bearing replacements or, for Actor::Update,
the tested motion pass. Three receiverless compatibility entries also still
have old generated call sites; this batch does NOT claim those hundreds of
old call sites are all recovered. It does not rename their no-ops to hide them.

## Per-entry result

| Original | Meaning | Result |
|---|---|---|
|45EC60|Actor::Update motion/bounds/carry|Reuse the R11 retdec_actor_update_motion body under the explicit function_45ec60(actor) entry; remove unused broken RetDec copy. C arithmetic retained.|
|489F30|SQObjectPtr destructor|Receiver-bearing helper now calls supplied C++ SQObjectPtr destruction. Old no-argument compatibility entry remains for generated callers.|
|489F50|SQObjectPtr assignment|Receiver-bearing helper now calls supplied C++ assignment, acquire before release and safe self-assignment. Old one-argument compatibility entry remains for generated callers lacking destination.|
|48BBA0|SQClosure destructor|C++ qualified destructor; no outer free here. Existing scalar destructor retains allocation ownership.|
|4992C0|SQClass destructor|C++ qualified destructor, GC unlink and member destruction; outer allocation ownership retained.|
|4A96D0|SquirrelObject size|C++ typed wrapper, original push/getsize/pop on current game VM; explicit receiver entry replaces unused no-argument shell.|
|4A99F0|SquirrelObject array reverse|C++ typed wrapper, original push/arrayreverse/pop. This is NOT clear; original48C400 confirms it.|
|4A9D70|SquirrelObject destructor|C++ wrapper lifecycle uses current VM external-reference table release, then resets value. Existing trace calls retained. Old no-argument compatibility entry remains for generated callers.|

## Confirmed adjacent defect fixed

Original403E50 receives ECX=this, calls4A9D70(this), conditionally deletes this,
and returns this. Its old reconstructed virtual slot instead called the no-op
4A9D70(), attempted to free &g1224 and returned an uninitialized value. The
new C++ __fastcall adapter preserves the original thiscall-compatible register
and stack layout without inline assembly. g16 now points at this adapter.
Tests invoke the real vtable with flags0(stack wrapper) and flags1(heap wrapper).

The obsolete4722E0/472820/472E50 legacy save-code copies formed an isolated
self-referencing group with no outside references after excluding their own
bodies/prototypes. Removed these broken copies; recovered save functions and
original baseline decompilation remain. A compile-time missing-receiver error
at the old472B4D call led to this checked cleanup; no guessed ECX was supplied.

## Validation and artifacts

Build commit201c700. Independent r5-quiet and r5-diag builds each passed18 CTests.
New cases cover table/array/string sizes, array reversal, balanced VM stack,
SQObjectPtr retained ownership/self-assignment/final release, and real virtual
SquirrelObject destruction. Existing GC/lifecycle/VM/script tests exercise the
source closure/class destructors. The orange/green lift, dry/submerged moving
stone (six placements), and BGM floating environment regressions all pass.
Seven implementations migrated to C++; Actor motion arithmetic stays in C.
Full C++ Execute backend remains OFF.

Earlier r1/r2 failed compilation (include ordering; stale legacy size call).
r3/r4 failed newly added fixture expectations (statement layout; misidentifying
array reverse as clear). These are retained and are not called game regressions.
No failed build was provided as the candidate. No test artifacts were overwritten.

Both r5 EXEs have all three DATs staged with size/SHA256 verification. User owns
interactive startup/gameplay testing and was given r5-quiet; no running game
was closed. Interactive confirmation is pending. Offline passes are not a
claim of complete behavioral equivalence or recovery of all old no-argument
callers. Those call sites remain a separate unresolved part of the request.
