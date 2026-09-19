# Typed native runtime continuation

Baseline: merged PR #4, `b2d945eeea78d1593bbd16f4cca7c51c92e1fa29`.

## Direct source API batch

- Replaced 1,693 address-named calls across 47 entry names with public Squirrel
  2.2.2 operations or the existing named, semantically required receiver/pop
  adapters. The C host includes the vendored public API, not a copied VM layout.
- Removed 53 disconnected source-backed ABI definitions and their declarations.
- `squirrel_api_types.h` is representation-only: Win32 address decoding, float
  bit transfer and borrowed `HSQOBJECT` construction. It owns no references.
- Intentionally retain nullable external-reference adapters, synthetic stack-slot
  return values that are actually consumed, and receiver/underflow scopes.
  Standard void APIs are substituted only when the former result is discarded.
- Added a contract compiled as C against the actual source VM. It covers high-bit
  addresses, bit-exact floats, borrowed integers, table stack balance, external
  reference lifetime, userdata tags and source compilation/execution.

## Invariants

No change to EXE-relative DAT search, archive ordering, game scripts, save format,
VM registration order, or trace call placement. Quiet and diagnostic Windows x86
validation must be recorded per source revision. Unit tests do not prove gameplay
or visual equivalence; original EXE/DAT assets are not available here.
