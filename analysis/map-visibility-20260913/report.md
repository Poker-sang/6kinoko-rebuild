# Map visibility investigation — 2026-09-13

Scope: repair the reported missing map/background in enemy-reset-20260913-r1-diag; retain original assets and all previous artifacts. The screenshot shows actors/HUD drawn and collision still active. No screenshot-time dump is available yet.

E-imports: IDA MCP via tools/ida_query.ps1, session 25eb0b37, original SHA256 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155. Original native x86 PE imports include file IO (CreateFile/ReadFile), USER32/GDI window/font APIs, Direct3D9/D3DX and timing; no network or crypto imports in the returned table. Full table and survey retained here. Desktop MCP connector uses a different server; local HTTP MCP provides the usable database.

E-snapshot: first script-error dump (frame 2790) has 1405 live texture slots, estimated uncompressed level-zero texture data 411,583,196 bytes. Terrain has 2014 draw records, alpha=1, scale=1, visible=1; bg2 also visible with one draw. This is NOT a capture of the reported black frame. The repeated old-instance EnemyUpdateProc errors were already investigated in ball-reentry-20260913; no evidence yet links them to black maps.

E-texture: reconstructed table has 4096 slots, increments forever, no name reuse, and 405D60 does not release anything. ACT/MCD destruction also omits texture releases. Original IDA 406370 looks up a lowercased resource name and increments its reference count; 405D60 decrements it, unbinds and releases the COM object at zero, and frees the handle. This concrete lifetime mismatch can exhaust the finite reconstructed table after repeated loads; existing actor resources then remain visible while new maps fail to obtain textures. Confirm with deterministic regression; do not claim the screenshot is conclusively explained.

E-map: original 434B60 queries the view rectangle and reuses render vectors. Rebuild ignores right/bottom and reallocates all tiles each frame. Keep this as a separate confirmed performance/parity discrepancy, not an asserted cause.
