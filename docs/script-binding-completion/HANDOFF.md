# Script Binding — R7 quiet

- Source commit: `3b8f3645437de09742c83b9f5e8fba953f3c6680`.
- Build tree: `build-runs/script-binding-r7-quiet`.
- Executable: `runtime-builds/script-binding-r7-quiet/kinoko_retdec_rebuild.exe`.
- EXE SHA256: `ED5C5DCD3FEAB759BF38A6E7F4E97594B2DB14A2F116DFF1662F8D39FB81416C`.
- Visual Studio 18 2026 / Win32 / Release; trace output disabled.
- Full default-target build exited 0, including contract compile/link. Legacy
  and third-party warnings remain; this is not a warning-free build claim.
- No game, contract executable or CTest was run by the agent.
- All three DAT files were copied beside the EXE and verified by size/SHA256.
  No reference-directory working-directory or data-dir override is used.

| DAT | Bytes | SHA256 |
| --- | ---: | --- |
| 6kinoko_a.dat | 163424746 | DD3AFF7E3E6BF0816C3073D113C3CCB12242A67E578D012C3B87190E7E24FC64 |
| 6kinoko_b.dat | 44681244 | 4D47B8E241886BE4300025324DDD7D1E3C5729232FF67E011F14163DB60130D2 |
| 6kinoko_c.dat | 11796163 | 80327F6F680D53AAA5539F45E11D33C1C6BA6B862648760725F44A380121852E |

## Preserved attempts within this batch

| Revision | Source | Outcome |
| --- | --- | --- |
| R1 | 0b49ea8 | Pointer migration exposed fixture and implementation conversion errors |
| R2 | d0dfcb4 | Host setter names collided with existing native property callbacks |
| R3 | 081da5e | A combined class-construction statement needed corrected parentheses |
| R4 | c90bd9c | Legacy integer-return printer needed its explicit void-callback ABI cast |
| R5 | 77c2c1e | Full build succeeded; DAT staged and verified |
| R6 | 3c554e8 | Native argument VM interfaces included; full build succeeded; DAT verified |
| R7 | 3b8f364 | Pointer-valued results included; full build succeeded; delivered version |

Every source version was committed before compilation. Build trees and runtime
directories use the matching `script-binding-rN-quiet` name and are preserved.

Final static identifier inventory: all superseded names in the 110-entry
mapping are absent from active `src`, `include`, and `tests` (excluding the
immutable original decompilation). Seven saved original IDA exports contain
valid address/code evidence. This is a source inventory, not a runtime proof.

The user performs gameplay verification. In particular the root-cache release
correction and new release-hook contract have not been executed by the agent.
