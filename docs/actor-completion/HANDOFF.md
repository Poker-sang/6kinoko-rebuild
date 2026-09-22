# Actor completion — R4 quiet

- Source commit: `ea1d9cf15b792efc96b8c1c14c2aea30b26d7b03`.
- Build tree: `build-runs/actor-completion-r4-quiet`.
- Executable: `runtime-builds/actor-completion-r4-quiet/kinoko_retdec_rebuild.exe`.
- EXE SHA256: `303EC18F8D38657DC319F781A871E292F88CA33CBE925F104EF226F7C9FDABB5`.
- Visual Studio 18 2026, Win32, Release, `KINOKO_RETDEC_DISABLE_TRACE=ON`.
- Full default target build completed successfully; warnings remain in legacy
  and third-party code. Contract executables were compiled and linked, not run.
- No game launch, automated local tests, or CTest execution by the agent.
- All three original DAT files were copied beside the EXE and verified by size
  and SHA256. No reference-directory working-directory/data-dir override.

| DAT | Bytes | SHA256 |
| --- | ---: | --- |
| 6kinoko_a.dat | 163424746 | DD3AFF7E3E6BF0816C3073D113C3CCB12242A67E578D012C3B87190E7E24FC64 |
| 6kinoko_b.dat | 44681244 | 4D47B8E241886BE4300025324DDD7D1E3C5729232FF67E011F14163DB60130D2 |
| 6kinoko_c.dat | 11796163 | 80327F6F680D53AAA5539F45E11D33C1C6BA6B862648760725F44A380121852E |

## Preserved build history

| Version | Source | Outcome |
| --- | --- | --- |
| R1 | d999b82 | Compile failed: diagnostics needed `_funcproto` for Squirrel closure objects |
| R2 | 55c2294 | Game linked; isolated contracts exposed callback dependencies and two stale fixture APIs |
| R3 | e696584 | Full build succeeded; final review identified a missing diagnostic declaration and stale comments |
| R4 | ea1d9cf | Full build succeeded after final cleanup; delivered version |

Each attempt has its own English build/runtime directory. All artifacts and
logs are preserved. R2/R3 game executables also have their verified DAT set.

The completed Actor scope and original evidence are listed in `README.md`.
Generic SqPlus, math and texture dependencies remain separate project work;
they are not represented as another pending Actor cleanup batch. Runtime
equivalence of this batch remains for user verification.
