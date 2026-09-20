# Portrait atlas regression and migration lessons

User reported that r45, r47 and r48 show incorrect portraits for more than half
of the transformation modes (miko shows the two-head portrait; Tenshi shows the
one-head portrait). This is a regression, not expected migration behavior.

## Cause and fix

Commit 988cd99 (r35) changed ACT resource loading from a texture-handle-only
helper to the recovered original LoadTexture lifecycle. The new loader correctly
resets the source rectangle to the full image when resource byte +96 is set.
However, the existing reconstructed deserializer retained the constructor's
initial value of 1. Original CActResource2D::Read at 446A84 explicitly executes
`mov byte ptr [esi+60h], 0` after reading the property values. That transition
was missing. The old simplified loader did not inspect this flag, hiding the
incomplete deserialization until the new loader began honoring it.

The original data/system/playerimage.act contains nine named resources backed
by only three atlases. Each has a 136 x 480 region, with source X equal to 0,
137 or 273. Resetting these coordinates collapses distinct portraits onto the
start of each shared atlas. No transformation IDs or asset names need remapping.

Fix c623101 clears +96 after successful texture-resource deserialization,
before LoadTexture. Fresh, programmatically constructed resources retain their
auto-size behavior. No game-specific mapping or portrait exception was added.

Evidence: original EXE SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155;
IDA session 3c59c915, original 446A30/446A84; local evidence under
analysis/portrait-regression-20260920 (assembly, original asset, decoded regions).

## Validation

r50-quiet was built from c623101 in its own build/run directories. All 53 CTest
tests passed. The added portrait_regions_contract loads the actual original
PlayerImage ACT and checks all nine resource names, IDs, atlas names, four crop
coordinates and the deserialization flag. This is a headless data/state contract,
not a claim of observed gameplay or successful GPU rendering. User's running
game was not closed or changed. The r49 serialization test's unrelated Squirrel
statement-separator error was fixed in the same checkpoint and now passes.

## Rules for subsequent migration

1. Audit the complete object state sequence: constructor, deserializer, load,
   clone/rebind and destructor. Record flag changes even when they are absent
   from serialized property schemas. A locally accurate method is insufficient
   when callers supply a state the original never supplied.
2. When replacing a simplified helper with a full original lifecycle method,
   explicitly compare newly activated branches and their prerequisites.
3. Include real shared-atlas assets with distinct nonzero crops in resource
   contracts. Checking only ownership, method ABI and default fields missed
   this regression.
4. Keep headless contract results separate from visual/gameplay evidence.
   Passing the old suite did not establish portrait correctness.
5. Fix state transitions at their original ownership boundary. Do not compensate
   with character-specific mappings or remove valid auto-size behavior globally.

These findings were recorded before resuming the remaining library migration.

The user subsequently confirmed that the portrait issue is fixed in actual
running gameplay. This supplements the headless contracts; it does not imply
that the remaining third-party migration is complete.
