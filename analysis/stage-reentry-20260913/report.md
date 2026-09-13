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

Implementation: typed C++ clone boundary, owned strings/vectors/keys,
remapped parent/child/resource relationships, shared decoded chip data ownership,
retained texture handles, fresh script callback environments, restored runtime
script registration before the original source Init callback. No slot-specific
cursor correction, guessed map reset or altered saves.

Validation checkpoint a7d18cd: independent Win32 Release r1-diag and r1-quiet
builds pass CTest 11/11 each, including prior water alpha, collision, platforms,
VM ownership, texture lifetime and ABI contracts. The new ACT test loads the
packaged titlemenu/worldmap assets and repeats three independent activations;
layer coordinates and all map records match the original source each time,
despite mutations to the prior runtime. Both EXEs have SHA256-verified DAT
copies beside them and a validation.json recording their source/hash.

User runtime evidence: "尝试重进了几次，暂时没观察到问题" after testing r1-diag.
The diagnostic process remains responsive and was left open for the user.
The independent automated window capture returned occluding browser pixels,
so it is not treated as a successful game screenshot. No claim is made of
an agent-completed first-level/jump/enemy sequence or independent quiet-window
smoke test; the user is still operating the diagnostic game. Quiet output
retains existing opt-in trace/dump tools and VM trace call sites.

Squirrel cross-check: supplied ../squirrel-2.2.2/SQUIRREL2/squirrel/sqvm.cpp
NEWSLOT/SET (806/811) and Set (1216), plus retained source disassembly
analysis/evidence/raw/phase3-20260906/squirrel-sqvm.asm. Title Init bytecode
already recreates cursorPos; the defect was native ACT reuse, not a missing
script cursor reset. Existing Squirrel 2.2.2 compiler/helpers remain linked;
experimental Execute stays off.

Checklist: original identity/imports recorded; clone/register order checked
against IDA pseudocode and BeginStage assembly; baseline regression reproduced;
C++ clone restored and obsolete 427950 C removed; both builds/tests pass;
resources staged; repeated reentry verified by user; all artifacts retained.
