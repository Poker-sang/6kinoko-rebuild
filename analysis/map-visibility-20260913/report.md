# Map visibility investigation — 2026-09-13

Scope: repair the reported missing map/background in enemy-reset-20260913-r1-diag; retain original assets and all previous artifacts. The screenshot shows actors/HUD drawn and collision still active. No screenshot-time dump is available yet.

E-imports: IDA MCP via tools/ida_query.ps1, session 25eb0b37, original SHA256 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155. Original native x86 PE imports include file IO (CreateFile/ReadFile), USER32/GDI window/font APIs, Direct3D9/D3DX and timing; no network or crypto imports in the returned table. Full table and survey retained here. Desktop MCP connector uses a different server; local HTTP MCP provides the usable database.

E-snapshot: first script-error dump (frame 2790) has 1405 live texture slots, estimated uncompressed level-zero texture data 411,583,196 bytes. Terrain has 2014 draw records, alpha=1, scale=1, visible=1; bg2 also visible with one draw. This is NOT a capture of the reported black frame. The repeated old-instance EnemyUpdateProc errors were already investigated in ball-reentry-20260913; no evidence yet links them to black maps.

E-texture: reconstructed table has 4096 slots, increments forever, no name reuse, and 405D60 does not release anything. ACT/MCD destruction also omits texture releases. Original IDA 406370 looks up a lowercased resource name and increments its reference count; 405D60 decrements it, unbinds and releases the COM object at zero, and frees the handle. This concrete lifetime mismatch can exhaust the finite reconstructed table after repeated loads; existing actor resources then remain visible while new maps fail to obtain textures. Confirm with deterministic regression; do not claim the screenshot is conclusively explained.

E-map: original 434B60 queries the view rectangle and reuses render vectors. Rebuild ignores right/bottom and reallocates all tiles each frame. Keep this as a separate confirmed performance/parity discrepancy, not an asserted cause.

## Implementation and validation

User clarification: the background first disappears after an in-stage map switch; leaving and reentering then loses terrain too, while entities remain. This strengthens the resource-exhaustion explanation but does not replace a black-frame capture.

Baseline source cbe8617: standalone --texture-lifetime-probe fails immediately because 405D60 never calls Release. Build/tools retained in map-texture-before-20260913. Candidate 8023c75/r1 exposed compile-time legacy vtable signature mismatches; all r1 products/logs retained. Corrected source 5f09862/r2 declares the receiver-bearing slots explicitly.

C++ texture_store implements original name-based reference sharing, final reference COM release/unbind and handle-slot reuse. It keeps the existing capacity instead of increasing it. ACT/MCD destruction releases owned handles; actor manager already releases its owned PAT handles, so PAT acquisition now shares the same cache. Sprite/frame handles remain borrowed. No Squirrel gameplay/error semantics, source assets or map ordering were changed.

C++ map_render replaces three naked assembly entries (434B40, 434F40, 46EED0) and the lost-receiver 434B60 entry. The existing update/draw behavior is retained; the invalid dead C bodies were removed. The full-map per-frame allocation discrepancy is deferred to a separate change so it does not complicate this ownership repair.

Squirrel cross-check: supplied ../squirrel-2.2.2/SQUIRREL2/squirrel/sqvm.cpp SQVM::Set and the retained source-object disassembly analysis/evidence/raw/phase3-20260906/squirrel-sqvm.asm show the receiver/type dispatch corresponding to the old script-error line. This does not establish a new VM defect. Existing vendored Squirrel 2.2.2 compiler/verified C++ helpers remain enabled; experimental full Execute replacement remains off.

Both r2 diagnostic and quiet builds passed CTest 5/5 and --enemy-reentry against their staged original DATs. Texture tests cover 5000 sequential and shared-owner cycles, retained-owner validity, final unbinding/COM release, slot reuse, failed loads, ACT/MCD destructor releases and all four map receiver ABI entries. Existing tests cover GC, actor/camera/script callbacks, stage/native behavior, shutdown and diagnostics. Original enemy scripts preserve four ball generations and ordinary/stomp death behavior.

The two runtime directories contain SHA256-verified DAT copies beside their EXEs. Per-run validation.json records the source checkpoint and hash. Quiet builds retain the existing opt-in trace/dump tooling and no automatic capture; VM trace call sites remain. No game/window was launched: the user explicitly deferred actual gameplay verification during this turn. Therefore this delivery does not claim startup/playthrough validation or prove that every disappearing-map/crash case is solved.

Follow-up IDA disassembly requests after the initial successful decompilation returned Session not found; those failed request files remain as failed evidence, not assembly validation. The successful original pseudocode and import/survey evidence above was obtained through IDA MCP.

The snapshot texture byte figure is width*height*4 (RGBA-equivalent), not a measured allocation total or GPU usage. It establishes 1405 occupied monotonically allocated slots at the first script error, before the reported final black-map state.

Checklist: original identity/imports retained; original acquisition/release contracts compared; deterministic failure reproduced before correction; typed C++ ownership and ABI introduced; both build modes and regression suites passed; DATs staged; all old products preserved; gameplay explicitly left to user.
