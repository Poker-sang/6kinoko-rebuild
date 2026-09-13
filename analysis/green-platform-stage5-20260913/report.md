# World-two Stage-5 hidden green platform

Scope: user's w2 Stage 5 hidden-map platform, preserving original scripts and loading logic. Carry identity/import/source auxiliary evidence from previous reports; original IDA MCP session f7e2c558 opened through the skill scripts. No platform/map-specific movement workaround.

The original w2-c05b.act contains green rail chip 1207 at (288,448). lift.cv4 InitRail receives that ID and selects take 1833, speed 1.5, normal CallbackRail/SetRide. The new offline fixture reads the actual hidden-map ACT/MCD rail and terrain layers, original lift script and item/player PATs. It drives the real native map chip query and actor collision/motion passes with a minimal standing rider. Both terrain and lift collision masks are enabled, to cover interactions absent from a platform-only fixture. Existing orange-platform fixture shares unchanged setup.

Commit this baseline before tests. Check support identity and carried displacement; derive fixes from original native assembly if it fails.

## Scene reconstruction and controls

The user's clarification identifies an automatically moving green platform and Reisen (TYPE_USA) as the reachable form; blue platforms start on contact. It does not establish that other forms are affected or unaffected.

The baseline fixture required the actual MCD spawn transform and 48-byte init data: chip 1207 at map coordinate (288,448) spawns at (305,464), flags 0x00010020. The hidden map has rail2 then rail1 in serialized order; 46F6D0 reverses the published layer names, so stage.nut creates rail1 first. The test now follows that order and uses the actual two-layer count. A terrain support actor also participates, so the fixture cannot assume exactly two Actors.

With correct scene geometry and an explicitly cleared rider step, the original green rail script carries a minimal rider normally (before4 control). Loading the complete original player/player_ground/player_jump/player_ex/player_suwa/player_ufo/player_start function set, TYPE_USA standing/update logic, and no input also carries normally when step is explicitly cleared (before7 control). Missing helper functions in intermediate fixtures were setup errors, not runtime fixes. These controls showed the movement/rail geometry itself was not the observed failure.

## Confirmed root cause

The previous platform fixtures explicitly called SetStep(null) to establish detached support. Removing that preconditioning from the fresh Reisen rider reproduces a failure in the original lift.nut SetRide at instruction 29: "the index 'top' does not exist". The invalid initial step is treated as non-null, so SetRide tries step.top and cannot install the platform reference. Collision can still support the player vertically, while horizontal transport has no parent reference.

Original Actor registration at 462175..462178 constructs a SquirrelObject at stack var_18 using 4A94E0. 4621D9..4621FA passes that object as the defaults for step and user, then destroys it. The saved original IDA disassembly is in original-actor-defaults-asm.json. A fresh IDA MCP session 5e23452e confirms 4A94E0 writes the SquirrelObject vtable and invokes 48ABE0 on its value pair. Supplied Squirrel 2.2.2 sqobject.h likewise initializes SQObjectPtr with OT_NULL, not a zero type tag.

Reconstructed 460E00 substituted a no-op compatibility call and zero-filled v72, publishing type 0 instead of a valid null. A preceding normal SetStep operation (for example ground support) masks this default-value defect, explaining why an already-established ride state worked in the control. The real user session was not captured at the first failing callback, so the exact preceding route remains an inference supported by these controls rather than a recorded playthrough.

Runtime fix fb69562 restores that single original constructor call before registering the two defaults. No map ID, character type, rail color, movement velocity, attachment condition or script is special-cased. No new per-frame SetStep call is introduced.

## Validation and retained artifacts

Final source fb69562: both green-stage5-20260913-r1-diag and -quiet pass CTest 9/9. The native suite checks Actor.step/user null defaults independently. The new green_stage5_contract runs the original hidden-map rail/terrain data, platform initializer and TYPE_USA player standing/update helpers without clearing fresh step: 90 frames carry exactly 1.5 pixels per frame, preserve platform support and floor contact, and report no VM errors. The existing orange lift, damage mask, crystal cadence, texture, GC/native/ABI and diagnostic contracts remain passing. The original-DAT enemy reentry/death probe also passes in the quiet build.

Baseline manifests:
- before / 047e83b: fixture omitted map-support Actors in count assertion.
- before2 / 1761deb: fixture used tile position without original MCD transform/init data.
- before3 / ab43d4c: fixture used unreversed rail order/count; platform fell/reset off the selected track.
- before4 / 41d81a9: correct map, minimal rider with explicit SetStep(null), passes.
- before5 / e8a9d63: original player fixture missing JumpDown helper.
- before6 / b32d384: original player fixture missing SetSwim helper.
- before7 / 7543e68: full TYPE_USA movement helpers with explicit SetStep(null), passes.
- before8 / a162023: fresh TYPE_USA step, original SetRide fails; retained in before8-green.log.

All baselines and outputs remain; none of those fixture corrections changed game behavior. Final DATs are copied and SHA256-verified beside each game EXE. The latest win32-resources quiet save is copied to both new directories without touching the source save. Original Win32 resource integration remains part of the build. Per-run manifests record source, executable hash and checks. No game was launched; user gameplay confirmation remains pending under the session preference.

Checklist: map/color/form distinguished; original data and script control tests used; failing fresh state reproduced; original assembly/source constructor mismatch confirmed; minimal constructor restored; both suites pass; staged DATs/save and original resources retained; all prior artifacts preserved; source committed before tests.
