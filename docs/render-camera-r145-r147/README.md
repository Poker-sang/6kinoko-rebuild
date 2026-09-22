# Map rendering, camera and shared draw submission: R145¨CR147

Base: R144 source 6c89b74, documentation e3b9fbc. User authorized all three
batches. Commit before build; compile only, no game or automated tests run.

## Evidence

IDA MCP session 5adc1098, original EXE SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Adjacent JSON files are raw MCP evidence. E-imports: imports.json contains
Win32 file/window/thread/CRT, D3D9/D3DX, WinMM and COM imports; no static
network/crypto/process-injection API family. LoadLibrary/GetProcAddress exist.
survey.json records PE32 image metadata. No original executable was run.

## R145: map visible rendering

Shared typed layout, placement and 232-byte quad schemas; render slots borrow
textures and native_buffer owns their storage. 434B60 now uses 435220's cached
forward/backward visible query, shared with the already recovered collision
consumer. Preserve query order, invisible slots, owner/cache validation and
lazy SetLayer. Apply world translation before scaling; zero scale stays zero.
41EF50 now has its actual this receiver and parent-chain implementation; map
world-position uses the original virtual method. Other ACT consumers share it.

434F40 saves/restores sampler ADDRESSU/V, selects wrap, disables depth,
selects standard blending then the layer mode, submits every textured slot,
unbinds texture and returns success regardless of individual draw HRESULTs.
Removed the previous unverified blend save/restore and first-failure break.
46EED0 retains camera rectangle conversion, +32 right/bottom and negative
camera offset. Map layer list ownership and duplicate insertion are unchanged.

Inherited reconstruction boundaries: invalid texture dimensions/handles and
missing chip data are rejected. The existing MCD representation still supplies
static chip textures; original animated-chip resource caches (435D00/435E80)
are a separate resource feature, not newly claimed implemented here.

## R146: camera

One verified 88-byte schema supplies native consumers and script property
offsets. Camera initializer and copy use real pointers; copied script slots
retain the existing SqPlus external-reference helpers (Squirrel 2.2.2-backed),
while update_vm is borrowed. Initialization no longer clears the whole 512-byte
backing allocation: 466270 resets position, center, offsets and bounds only,
leaving dimensions and callback intact. Startup storage remains zero-initialized.
Camera Update keeps the exact closure-only dispatch at 466470; no native
following/clamping algorithm is invented. The game-loop order remains input,
global callback, mask read, camera callback, Actors, map and stage updates.
Projection 466320 is shared, with separate translation and X-floor/Y-ceil.
New camera contract checks reference-call order, retained dimensions/callback,
negative projection and copied borrowed VM. Compiled only at final handoff.

## R147: shared quad submission and state

405800 is a typed Quad submit path shared by maps, Actor/PAT, ACT layouts and
text glyphs. Only the C integer-slot compatibility shim remains. Preserve XY
half-pixel subtraction, Z+0.5, RHW=1, untouched UV/color, texture binding,
FVF 324 and a two-triangle strip with 28-byte stride. Restore original behavior
of ignoring texture/FVF setup HRESULTs and returning the draw HRESULT.
402770 blend transitions now name D3D states and values, preserving partial
writes and the cache. Alpha/depth methods preserve flags and modulation.
Render queue stores borrowed pointers and calls real typed virtual draw methods;
map layer rendering likewise uses its layout's Update/Draw virtual slots.
Named map entry points replace address names. Duplicate/order semantics remain.
New fake-device contract checks setup failure still submits, exact vertices,
UV/color preservation, blend transitions and repeated-state writes. No D3D
window is required; this contract is compiled but not executed by the agent.
