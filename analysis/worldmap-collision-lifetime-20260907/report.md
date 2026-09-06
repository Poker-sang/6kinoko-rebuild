# World-map transition collision lifetime

Scope: follow-up to b6b6bac; user reports a freeze/crash on entering the world
map. No game executable is launched. Use existing diagnostic logs and
separate console regression tests.

## Evidence and progress

- [x] Read latest log; map crash RVA to the exact tested EXE.
- [x] Compare native cleanup against original IDA disassembly.
- [x] Reproduce missing cleanup in the console contract test.
- [x] Restore cleanup and recycled-handle lifetime.
- [x] Build/test both configurations, stage DATs; included in this backup commit.
- [x] User confirms the world-map crash is resolved.
- [ ] Subsequent stage-entry crash: tracked in ../stage-collision-update-20260907/report.md.

E-imports/scope: carry forward ../stage-entities-20260907/report.md.
IDA skill start.ps1/open.ps1 opened original session 73953b36 and rebuilt
session 1aa670a4. Original survey confirms the same SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Tested rebuilt SHA256 is
E28AC10D014F22AAC646175244636FB4BB7004C48599867BA4D1C71B632865FB.

The latest two log runs fault at RVA 0x57CAF, function_468620_this+0xBF,
reading [EDX+0x90] with EDX=0xffffffff (fault address 0x8f). The caller is
retdec_actor_manager_update+0x231. No crash dump remains in either runtime.
The old op.act collision proxy survives into the world-map update while its
map layout is no longer valid.

Original WorldMap.Update invokes ClearActor then ReleaseMap. Native
469700 -> 463800 -> 463730 decrements tree-owned Actor references and calls
the handle manager's release slot when the count reaches zero. RetDec kept
the decrement but lost that virtual call. Original 46A6F0 invalidates the
generation, runs 45F0C0/45E460 without freeing the reusable actor allocation,
then enqueues its slot. Original 45E460 releases the actor's shared owner;
468620's 45E410 weak lock then returns empty and avoids the retired layout.

Before the fix, the new tests/stage_contract.c lifecycle assertion fails:
after ClearActor, the collision proxy control block still has strong count 1.
The existing creation-only tests did not exercise this transition.

## Correction

463730 now invokes the recovered 46A6F0 release path for each last tree owner.
46A6F0 receives one packed handle and an explicit manager, validates its
generation, destroys the actor in place and appends the index to the free list.
45E460 restores callback/SquirrelObject cleanup, weak-parent release and
shared-owner release. The Actor allocation remains available to the pool.

The shared-owner helper previously tested InterlockedExchangeAdd's old value
against zero. Original 4686DA/45E4E8 use the instruction's resulting zero flag,
meaning the old count was one. Correct that test, dispose the Actor* slot at
zero strong owners, and retain/free its control block according to weak owners.
This follows original 43E850/415220 disposal semantics. Squirrel objects keep
their existing 4A9570/4A9D70 release path; upstream sqobject.h confirms those
objects have separate VM reference semantics.

46AB10's recycled branch now reconstructs the actor in place and writes the
new generation at the recycled index, as original 46AC20..46AC58 requires.
The previous reconstruction appended the generation and skipped construction,
which would leave subsequent release/lookups using stale generation values.
ClearCollision now also frees a last expired control block when dropping its
weak owner.

No new world-map exception, extra ClearCollision call or pointer-range bypass
was added. WorldMap.Update's original ClearRenderLayer -> ClearActor ->
ReleaseMap sequence remains responsible for the transition.

## Validation

The new regression first failed on the pre-fix strong-count assertion. After
the fix it checks that the collision proxy expires after ClearActor, protects
the retired layout with PAGE_NOACCESS, and executes the real collision update.
Four rounds of 600 actor creations/clears then reuse the same allocations
while the retired layout stays inaccessible. Old weak references stay empty,
pool and generation vector sizes stay equal and constant, and the VM stack
returns to its initial depth.

Both Release builds pass archive_smoke and stage_native_contract (2/2).
Both contract executables also pass with the effective original block.cv4.
stage_dat.ps1 copied and SHA256-checked the three DATs next to each game EXE.
Save/config files and the user's existing unrelated changes were preserved.
No original or rebuilt game process was launched; visual/gameplay validation
remains with the user.

Final game EXE SHA256:
- Diagnostic: 2D3AA19247CD2105BED93865D1AF2187E0CA1359AED932BA5B3BF2746245A988
- No-log: ADD410C21B295B2A257136E34B4787718751D1282489247C49A5D7D7E4907B7E

Test the same diagnostic EXE in runtime-builds/p3-save-worldmap-diag:
1. Select the same save and enter the world map; wait and walk for 10 seconds.
2. Enter a simple stage and check entity visibility, jump/attack and landing.
3. Return to the world map and reenter; then test the previously complex stage.
4. Repeat world-map entry with p3-save-worldmap-notrace.

The log identifies this exact crash; original assembly and a failing/passing
offline lifecycle test support the correction. This does not establish that
all stage-specific faults or gameplay problems are resolved.
