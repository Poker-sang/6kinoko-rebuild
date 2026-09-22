# Actor completion

This batch closes the Actor-specific migration scope, including its supporting
containers and call sites. It does not claim completion of generic SqPlus,
texture, math, or Squirrel VM migration.

| Area | Readable implementation |
| --- | --- |
| Shared layout and script properties | `actor_records.hpp`, `actor_registration.cpp`; checked member offsets |
| Construction, disposal, parent ownership | `actor_lifecycle.cpp`; typed Actor/control pointers |
| Initialization, map creation, collision proxies | `actor_initialization.cpp` |
| Reset and assignment | `actor_state.cpp`; explicit reference and field copy order |
| Update and collision script callbacks | `script_callbacks.cpp`, `collision_callbacks.cpp`; shared callback record |
| Manager, iteration and render layers | `actor_manager.cpp`, `actor_diagnostics.cpp` |
| Pool, owner list and priority index | `actor_pool.cpp`, `actor_owner_list.cpp`, `actor_priority.cpp` |
| Animation, PAT and frame storage | Existing animation/PAT C++ modules, typed Actor animation interfaces |
| Drawing, movement and collision | Existing `actor_render.cpp`, `actor_motion.cpp`, `actor_collision.cpp`, `actor_methods.cpp` |
| Cleanup and observations | `actor_cleanup.cpp`, `actor_diagnostics.cpp`; main-C Actor implementations removed |

## Original evidence

The accompanying IDA exports were obtained from the original executable with
SHA256 `2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
Squirrel metadata uses the supplied Squirrel 2.2.2 source layout.

- `45E300`: construct only the fields initialized by the original; preserve
  recycled storage in untouched fields instead of clearing the entire Actor.
- `45EB00`: clear/release parent and owner, clear script, retain saved Init
  argument before function, invoke Init, release in reverse order, and reindex
  using the priority after Init.
- `45FFE0`: acquire strong references before releasing previous references;
  replace weak ownership only when changed. Preserve script-copy order, shallow
  collision pointers, untouched layout holes, and repeated original copies.
- `46B0A0`: construct the verified manager prefix and its owned containers;
  remove the reconstruction's blanket 512-byte write beyond the shared prefix.
- Callback argument type/value are independent of the consumed temporary
  reference, as required by existing script-call interfaces.

Existing null-name callback and empty-manager compatibility paths are retained
explicitly. Opaque layout bytes are named unknown instead of inventing gameplay
semantics. Shared SqPlus integer ABI, generic texture handles, and original math
entry points remain boundaries, not unconverted Actor implementations. VM trace
calls remain present; quiet builds suppress output at the existing output layer.

## Validation policy

Actor constructor canaries, Reset/assignment ownership/order checks, priority
ordering, and existing integration fixtures are retained or expanded. These are
compiled only for this batch. Neither test programs nor the game are executed by
the agent. Build and delivery details are recorded separately after compilation.
