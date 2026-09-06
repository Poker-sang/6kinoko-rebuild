# Stage collision update

Follow-up to b98339a: user confirms world-map entry works, but entering a
simple stage crashes. No game launches; inspect the existing log and use
console tests.

## Evidence and work items

- [x] Locate the new crash in the matching EXE and compare original assembly.
- [x] Reproduce the missing collision candidate allocation offline.
- [x] Restore paired buffers, collision dispatch and frame ordering.
- [x] Verify both builds, stage DATs; included in this backup commit.
- [ ] User verifies stage entry and gameplay.

Carry forward skill/scope and E-imports from the preceding stage-entities and
worldmap-collision-lifetime reports. IDA session 73953b36 is the original,
1e0c5e58 is the tested rebuild. The tested diagnostic SHA256 is
2D3AA19247CD2105BED93865D1AF2187E0CA1359AED932BA5B3BF2746245A988.

Latest log: data/map/w3-s01a.act creates 3+1 enemy-layer actors and five map
events before crashing at RVA 0x8AFA2, retdec_actor_manager_update+0x82.
Rebuilt assembly has mov eax,[ebx+0x7c]; mov [eax+esi*4],edi, with EAX=0.
This is inlined 462E80 writing the first collision candidate.

Original 463D74/463D82 allocates both ActorManager+100 and +124 vectors to
twice the active actor count. The reconstructed refresh allocated only +100.
Original 462E80 visits each candidate pair in order and calls 462CE0 with
the two Actor pointers. The RetDec body passed g1224 placeholders instead.
462CE0 uses callbackGroup/callbackMask at +320/+324, inclusive float rectangle
overlap via 4045C0, and callbacks at Actor+120 with the other Actor instance.
464285 also refreshes the actor list after script updates and before motion;
that refresh was missing in the reconstruction.

The log also records an ACT-inline main script GET failure during stage
loading. This is a separate unresolved observation, not evidence for the
null candidate-buffer crash.

## Correction and verification

The added console regression first failed because ActorManager+124 was null
after creating three actors with callback masks and refreshing the manager.
The original second vector allocation is now restored alongside the update
vector. Both vectors retain capacity and grow to twice the required count.

462E80 now passes actual pairs in its original nested-loop order.
4045C0 restores inclusive float rectangle comparison. 462CE0 restores each
direction's mask test and calls the existing 45E020 helper with the other
Actor's Squirrel instance. The second direction reads masks again after the
first callback, matching 462DEA. Failed callbacks restore stack depth and
clear that callback, matching the original recovery path.

464285's missing refresh is restored between script stepping and motion, and
the motion pass reloads the vector/count after that refresh. A new script test
creates a child and calls another Actor's Release in a single frame. This
exposed another missing edge: the marked actor left the tree without handle
release. Original 463DC2..463DD5 now erases and releases it. Traversal captures
the in-order successor before erasure so reparented nodes are not revisited.
45DBC0's Release bridge explicitly preserves its Actor receiver.

Passing test cases:
- Existing DAT registration, named creation, 600-actor growth and scene cleanup.
- Candidate-buffer allocation and full three-actor collision traversal.
- Inclusive touching bounds, disjoint rectangles and inactive candidates.
- Directional callback masks and mask changes during the first callback.
- Script callback receiver/other-actor identity and original pair order.
- A child created during Step moves in the same frame but is first stepped
  on a later frame; a released actor's shared owner is expired before motion.
- VM stack depth remains stable.

Both Release build trees pass archive_smoke and stage_native_contract (2/2).
Both standalone contract tools also pass with the effective original block.cv4.
No game executable was launched. stage_dat.ps1 copied and hash-checked all
three DAT files alongside both EXEs. Saves/configs were preserved.

EXE SHA256:
- Diagnostic: EEC958469F0593AEF094BAE089533BE48D340A2FAA08932CA8F55F50597A1DB1
- No-log: 0E4017F967D9FF988E6722B4362935DCDA5DCA004033E640AE7E3283940BCDA3

User test: run runtime-builds/p3-save-worldmap-diag/kinoko_retdec_rebuild.exe,
enter the same w3-s01a stage and wait briefly before moving. Then check actor
visibility, jumping, attacking and contact with terrain/enemies. Report whether
a remaining failure occurs on entry or after an action. Preserve the diagnostic
log. Full gameplay and the separate ACT-inline GET failure remain unverified.
