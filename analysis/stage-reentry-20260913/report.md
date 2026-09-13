# ACT reentry restoration — 2026-09-13

E-imports: local original x86 PE, SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Original survey/imports retained via IDA MCP HTTP session 8a448dc9.
File IO, Win32 window/font/input, Direct3D/D3DX, timing, COM and dynamic library
resolution present. Scope is local game reconstruction, no original asset edits.

Original BeginStage 450950 calls source CAct virtual slot +20 at 450B31:
427950 clones ACT properties, resources and layers, then remaps graph links.
Previous runtime is destroyed; source remains unchanged. EndStage 450D80 only
stops activity and clears transient queues. 450CB4 registers runtime script;
450CC6 invokes retained source Init, followed by runtime layer Init callbacks.
The rebuild instead aliased source and runtime. Re-publication copied the
already-mutated cursor dst_y into oy and reused changed map chip records.
Baseline checkpoint d9b7cbe, --act-reentry on packaged titlemenu/worldmap ACT,
fails immediately at active != source. All independent baseline products retained.

Implementation in progress: typed C++ clone boundary, owned strings/vectors/keys,
remapped parent/child/resource relationships, shared decoded chip data ownership,
retained texture handles, fresh script callback environments, restored runtime
script registration before the original source Init callback. No slot-specific
cursor correction, guessed map reset or altered saves.
