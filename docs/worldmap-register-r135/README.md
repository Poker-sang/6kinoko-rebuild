# R135: restore virtual layout registration during ACT publication

## Report and scope

R134 was reported by the user to enter the world map with a responsive window,
stationary character, ineffective controls, and title BGM still playing. This is
user feedback, not an agent runtime observation. No game or tests were run here.

## Original evidence

Reference: `../6kinoko/6kinoko.exe`, SHA256
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
IDA MCP session `b22af54a`; current captures are in this directory.

- `450950:450C5D` registers the layer; `450C62` selects its first key through
  `452040`; `450C79` calls the selected layout's virtual Register at slot +36.
  The root script Register/Init follows at `450CB4/450CC9`. Full begin-stage
  decompilation: `../act-r132-regression-r133/0x450950.json`.
- `452040` selects the first key only when layer+196 is zero and key count at
  +184 is nonzero. It does not search for a supported concrete layout class.
- Map Register `4341F0` creates separate outer/script instances, then writes
  layer+52 = map+320 and layer+56 = map+328 (`434351/434360`).
- R134's ACT publication manually constructed wrappers and bypassed Register.
  Its map alpha/blend aliases therefore remained unbound after virtual clone.
  R131's final compatibility SetLayer pass happened to supply these aliases.

## Change

ACT publication now dispatches the selected layout's actual Register method,
including map/2D/string property association, with the original first-key rule.
As at `450C79`, the Register return value does not introduce a new stage-abort
branch. R134's lazy resource binding remains intact; Register does not force
resource binding. No BGM override or scene-specific workaround is added.

## Script evidence and limits

Static bytecode decoding uses `tools/inspect_cv4.py` and Squirrel 2.2.2 opcode
names; it does not execute the scripts. Locally preserved raw Control/Move CV4
assets are excluded from git. Control was extracted from C offset 631371,
size 17651, XOR 0x27; Move from A offset 24753178, size 12888, XOR 0x2f.

The existing `analysis/world-atlas-20260919/effect-bytecode.txt` shows
UpdateAreaEffect subtracting from w4_fog1.alpha at instruction 0054, then
comparing it at 0058. WorldMap Update invokes this function before movement
and input handling. An unbound indirect getter yields null, so this arithmetic
cannot provide the original update behavior. This statically explains a route
to stalled updates, but does not prove the precise failing call in the user's
run. WorldMap Resume calls CheckResult/ProcMaskLayer before SetBgm; the exact
reason title music persisted is not established by these captures. Recovery
of the complete reported symptom remains for user runtime verification.

## Regression source and build

`tests/map_lazy_binding_contract.h` now additionally exercises actual cloned
ACT publication with initially null aliases, separate wrappers, bidirectional
alpha/blend access, source isolation, preservation of lazy resource binding,
and exclusion of timeline layers. These assertions are compiled, not executed.

Build: independent `build-runs/worldmap-register-r135-quiet`, Release Win32,
trace disabled. Output: `runtime-builds/worldmap-register-r135-quiet`.
The source commit and artifact hashes are recorded in `artifacts.json` after
compilation and DAT staging. No game execution, ctest, or local test execution.
