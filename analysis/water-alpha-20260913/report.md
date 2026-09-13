# Water alpha restoration — 2026-09-13

Scope: original C:/WorkSpace/6kinoko/6kinoko.exe and packaged DAT assets;
restore the original behavior in this repository. No asset edits or water-specific opacity.

E-imports: original x86 PE SHA256 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
IDA MCP local HTTP session d96e09e6 (skill start.ps1/open.ps1 and tools/ida_query.ps1).
Survey/imports JSON retained: file IO, Win32 window/font/input, Direct3D9/D3DX,
timing and COM. Direct network/crypto imports absent in this view; dynamic
LoadLibrary/GetProcAddress present. Registered connector uses a different server;
local HTTP MCP is the working analysis transport.

E-script: stage.cv4 InitWater/InitWaterLine selects takes 9700/9710 or 9720/9730;
no opacity assignment in these functions. tools/inspect_cv4.py uses supplied
../squirrel-2.2.2/SQUIRREL2/squirrel/sqopcodes.h. Cross-checked SET with supplied
sqvm.cpp:811 and SQVM::Set:1216, and retained source disassembly
analysis/evidence/raw/phase3-20260906/squirrel-sqvm.asm. No new VM defect found.
Existing vendored compiler/helpers remain enabled; experimental Execute stays off.

E-original: 464F80 loads PAT type-2 ARGB into frame+244 -> appearance+4.
45F283..45F2EB sets this color and modulates Actor ARGB through vtable+12.
42B2D0 multiplies each unsigned byte and divides by 255, then broadcasts to all
four vertices. Rebuild loaded the color but retdec_actor_render ignored it.

E-baseline: checkpoint c32881a fixture initially selected the wrong PAT and
failed before the render check. Corrected fixture 524529c uses data/map/map.pat:
`kinoko_stage_contract --water-alpha C:/WorkSpace/6kinoko` fails with
`take=9700 PAT=80ffffff rendered=ffffffff`. All baseline artifacts retained.

Implementation: sprite_color.cpp restores original color assignment/modulation
and typed frame appearance. Three lost-ECX C color methods are replaced by explicit
fastcall C++ methods in their original vtable slots, with field-offset assertions.
Actor rendering resets PAT base color every draw before multiplying Actor ARGB.
No guessed alpha, map-layer changes, gameplay changes or new automatic logging.

Validation pending: independent diagnostic and quiet builds, CTest, staged DATs,
and bounded first-level/jump/enemy window smoke tests. All products retained.
