# R107: inline native stack bridge helpers

Source commit: `22125a7`.

This batch removed six private `*_from_stack` forwarding helpers from
`src/squirrel/squirrel_native_calls.cpp`. The original address-compatible
`function_471c10` through `function_472140` exports remain and now call the
recovered named implementations directly, preserving their ABI and stack
indices.

The readability inventory now distinguishes retained ABI-facing
`kinoko_method_*`, `kinoko_map_*`, `kinoko_actor_*`, `kinoko_script_*`, callback,
and entry exports from removable internal bridge layers. The lexical inventory
changed from 178 thin bridges to 108 thin bridges and 224 retained ABI aliases;
`named_mixed_legacy` remains zero. This classification is an audit aid and does
not claim linked reachability.

## Build artifact

- Win32 Release build tree: `build-runs/bridge-cleanup-r107`
- Delivery executable: `runtime-builds/bridge-cleanup-r107/kinoko_retdec_rebuild.exe`
- EXE SHA256: `6DF0E2B990CC94942DB96FF777921AA7EA17521120EC3212DF9FAD2E2C09EDB7`
- `6kinoko_a.dat`: `DD3AFF7E3E6BF0816C3073D113C3CCB12242A67E578D012C3B87190E7E24FC64`
- `6kinoko_b.dat`: `4D47B8E241886BE4300025324DDD7D1E3C5729232FF67E011F14163DB60130D2`
- `6kinoko_c.dat`: `80327F6F680D53AAA5539F45E11D33C1C6BA6B862648760725F44A380121852E`

The three DAT files were staged beside the executable and hash-verified by
`tools/stage_dat.ps1`. Per the test handoff, no game, CTest, or contract
executable was run by the agent.
