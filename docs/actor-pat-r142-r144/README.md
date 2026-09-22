# Actor drawing, PAT animation and frame ownership: R142–R144

Base: R141b, source `72da95e`, handoff `bb4542a`. User requested 2–3 complete
batches before stopping. Only the final combined version is built for handoff;
each logical source batch is committed first. No game or test executable is run.

## R142: Actor drawing

Actor::Render (45F0F0), Actor::Draw (45F350) and camera projection (466320)
are expressed in actor_render.cpp. PAT frame vertices, base/working positions,
pivots, texture extents and owned appearance share one layout with compile-time
offset checks. Frame color no longer has a separate local schema.

Preserve active/visible checks, scale*256 culling extent, facing/pivot order,
degree-based rotation, ARGB multiplication by 255, blend selection and restore.
The original has two position stores: Actor translation, then camera translation.
Restore that order instead of combining offsets before addition. X uses floor,
Y uses ceil as confirmed at 46637E/466397. Null-camera and missing-texture guards
are inherited reconstruction boundaries, not claimed original guards.

Device submission is still the existing quad renderer host; this batch does not
replace the D3D device or broader map/UI renderer. Trace calls remain at the host
boundary. New render contract source covers negative coordinates, facing,
shared-frame color reset, culling and restoring blend state after failed submit.

## Evidence

Adjacent JSON files are IDA MCP decompile/disassembly output for original
`../6kinoko/6kinoko.exe`, SHA256
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
The x87 disassembly is used when decompiler temporaries alias the sine/cosine
results. Existing Squirrel 2.2.2-backed object and callback adapters are reused;
this work does not introduce a second VM or new script reference ownership.
