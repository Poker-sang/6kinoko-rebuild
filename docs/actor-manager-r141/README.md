# R141: ActorManager lifecycle and scheduling batch

2026-09-26 update: the historical empty-tree compatibility behavior below is
superseded by [original object/scene lifecycle restoration](../object-scene-chain-20260926/README.md).

Base: R140 (`06d451d`, source `671f741`). This batch completes the accepted
ActorManager scope, not migration of the renderer or the entire game.

## Coverage

| Responsibility | Implementation |
|---|---|
| 46B0A0 manager construction; 463AF0 prewarm of 512 actors | reconstructed/actor_manager.cpp |
| 463B40 create; 45E5E0 initialize; 45E120 callback and animation timing | actor_manager.cpp; squirrel/actor_initialization.cpp |
| 46AB10 acquire/reuse; 46A6F0 retire; lookup and destruction | reconstructed/actor_pool.cpp |
| 46AA60 owning list; 463580 clear; base destructor | reconstructed/actor_owner_list.cpp |
| Priority allocation, stable insertion, erasure; 463CF0 reindex | actor_priority.cpp; actor_manager.cpp |
| 463730 release tree owners; 463800 reset; 464E20 resource clear | actor_manager.cpp; existing actor_cleanup.cpp |
| 463D40 deferred release and iteration/layer ranges | actor_manager.cpp |
| 4641D0 callback/collision/motion scheduling; 4627C0 render scheduling | actor_manager.cpp |

Remaining generated C entries in this scope are legacy calling adapters,
diagnostic hooks, and accessors for original globals. Quad rendering, global
collision-state selection, animation resource loading and the script binding
registry are dependencies. The Actor tick itself now uses named fields and
keeps the existing diagnostic calls around the script invocation.

## Evidence and behavior

Adjacent JSON files are IDA MCP output from the original `6kinoko.exe`, SHA256
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
Squirrel reference semantics use the existing adapters backed by
`../squirrel-2.2.2/SQUIRREL2/squirrel/sqapi.cpp` and the source disassembly in
`analysis/remaining-mapping-20260920/source-sqstate-disassembly.txt`.

- Preserve refresh → collision callbacks → refresh → script/animation →
  refresh → collision refresh → motion. Creation during script becomes visible
  to motion after refresh; release retirement waits for the same boundary.
- Restore per-actor mask and loop-count reads shown at 464276/464281 and
  4642B7/4642C2. The previous helper incorrectly snapshotted the mask once.
- Restore original virtual pool acquisition/retirement. Initialization failure
  explicitly retires the acquired handle at 463CB6; the former helper leaked
  the live slot on that path. Borrowed C argument wrappers retain their current
  bridge ownership contract; no additional consuming release is invented.
- Capture the take before script invocation. Initialization publishes bounds
  after the callback using the callback's position and local bounds, without
  introducing scale or direction transforms.
- Priority insertion keeps equal keys stable with upper_bound; the legacy
  insert-left adapter continues to use lower_bound. Reindex returns success
  instead of the previous dangling address of a local result array.
- Pool slots use actual Actor pointers, packed 16-bit slot/generation handles,
  LIFO reuse, and the original lock boundary. The established invalid-handle
  guard remains; no gameplay selection rule is added.
- Owning-list clear decrements native references; deferred release retires
  directly. Destruction of all allocated pool slots remains distinct from
  retirement. Field +8 is named owner_references instead of type.
- Iteration buffers own storage and borrow actors. Four render ranges use
  original thresholds -1 / 0xffff / 0x10000. Existing empty-tree behavior
  (clear dirty, leave ranges untouched) and allocation-failure guards remain
  explicit compatibility boundaries; this is not a claim of byte identity.

## Verification handoff

Added `tests/actor_manager_contract.cpp` for stable priority reinsertion, four
render ranges, callback mask mutation, deferred release, new-actor motion,
reference ownership, virtual dispatch, initialization failure and activation
boundaries. Existing stage integration covers real VM callbacks, child creation,
600-slot reuse and weak proxy expiration.

Per user instruction, no game, CTest, or regression executable is run. Build and
DAT verification results, source commit and executable hash are recorded in
`artifacts.json` after compilation. All build attempts and outputs are retained.

The first R141 build (`cfcc95a`) compiled all targets successfully. Its new
manager regression executable used CMake's default output under the build tree.
The follow-up adds that target to the shared runtime/tools output policy. A
separate R141b build preserves the first attempt without overwriting it.
