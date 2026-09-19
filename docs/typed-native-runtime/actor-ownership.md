# Shared Actor records and ownership

`actor_records.hpp` is the single schema used by Actor construction, destruction,
step relationships, animation updates, native methods and manager cleanup. It
retains the original Win32 sizes and member locations with compile-time layout
checks. Unknown regions are opaque byte arrays, not invented game properties.

`native::RecordView` reads and writes actual member types through `memcpy`. It
never treats a C allocation as a newly constructed C++ object. Nested views use
a pointer-to-member of a real schema object; there is no null-object member
arithmetic, guessed displacement, packed misaligned reference or owning cast.
The underlying byte storage is borrowed, and a view never frees it.

## Ownership boundaries

- The manager owns live Actors and the animation list. Animation lookup nodes
  borrow list entries. Each animation owns its frame storage; each frame owns
  its allocated payload. Texture indices contain owned resource handles.
- Actors borrow the manager, animation and current frame. Changing an animation
  does not duplicate or transfer the animation's allocation.
- The native owner control block retains the original strong/weak counter
  semantics. A step relationship holds a weak native reference. These counters
  are not Squirrel VM references and must not be replaced with `sq_addref`.
- The seven 12-byte script slots retain the existing external Squirrel object
  ownership convention. By-value script arguments transfer one external
  reference, consumed once even when native-instance lookup fails.
- Deferred Actor release sets the manager's pending-cleanup flag. It does not
  immediately free the Actor from an active iteration or invent a second queue.

Cleanup preserves texture-release order, Actor destruction before borrowed
animations disappear, lookup-node/list distinction and retained container
sentinels/capacities. Foreign native control blocks are dispatched with their
verified zero-argument `__thiscall` method signature rather than a simulated C
register context. The two boundary table slots have named fields; their ABI is
still verified, not claimed to be an entirely new C++ engine class hierarchy.

## Preserved behavior

Animation lookup still updates the take and timing fields before a failed lookup.
Callback take changes defer ticking. Terminal frames, looping, signed duration,
upper-only sync clamping, double-precision bounds intermediates and half-pixel
adjustments retain the previous behavior. No new lower-bound clamp or supported
layer limit was inferred from storage size. Sign extension of chip flags, 16-bit
shape fields and the game's existing misspelled callback name are unchanged.

`native_record_view_contract` covers 16 byte alignments, nested members, high-bit
addresses, script slots and surrounding sentinels. `actor_records_contract`
executes native animation/method/cleanup code and checks timing, failed lookup,
mirroring, state copying, payload lifetimes and cleanup order. The original
`actor_lifecycle_contract` remains enabled for VM/native reference transitions.

## Remaining scope

This is not a full migration of every scene, render, input or save object. Those
subsystems still contain independently recovered records, raw native addresses
and compatibility entry points. The legacy zero-argument frame-copy operand
scanner is deliberately unchanged: no explicit arguments have been invented
without original call-site evidence. Original-asset gameplay validation is
still required; layout contracts cannot establish visual or gameplay parity.
