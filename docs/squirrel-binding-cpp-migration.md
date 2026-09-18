# SqPlus host binding migration (2026-09-18)

This batch starts at `f7d2f9dd46a5c4da5a8385d77abd876738c49ff6`, after PR #1.
The interpreter, collector and public API already used the vendored Squirrel
2.2.2 source at that baseline. This batch moves the **game-side SqPlus binding
layer**, not a second interpreter, into compiled C++.

## Changed boundary

| C++ module | Recovered definitions replaced | Responsibility |
| --- | ---: | --- |
| `src/squirrel/squirrel_class_binding.cpp` | 15 | Class construction/inheritance, metadata and native registration |
| `src/squirrel/squirrel_native_variables.cpp` | 9 | Variable lookup, conversion, table and instance property access |
| `src/squirrel/squirrel_method_dispatch.cpp` | 15 | Native receiver resolution, argument validation and method dispatch |

These are **39 actual function definitions / 1,154 old function-body lines**,
not merely extension renames. The compatibility exports retain their numeric
names for the remaining C callers; their implementations use named C++ records,
scoped external references and direct Squirrel source APIs.

`include/kinoko/squirrel_binding.h` is the C ABI surface.
`include/kinoko/squirrel_binding_detail.hpp` defines the private fixed-layout
views and stack/ownership helpers. `kinoko_native_binding_type` is a narrow host
service keeping descriptor identities in the original embedding; it is not a
new type registry or a replacement for the original game classes.

### Replaced definitions

Class/registration (prefix `function_`):

```text
4a9250 4a9370 4a9490 4aa540 45f4f0 45f640 45fab0 45f3e0_this
460920 4609c0 460a60 4607e0 460d10_this 460d10_actor
460e00_register_actor_method
```

Property access:

```text
function_4aa5e0 retdec_get_var_info retdec_get_var_value
retdec_set_var_value retdec_resolve_instance_var
function_4aab60 function_4aabd0 function_4aaf30 function_4aafa0
```

Methods/arguments (prefix `function_`):

```text
45f560 45f5a0 45f5e0 45f5e0_at 45f850 45f8c0 45f9d0
460540_this 460540 460b00 460b50 460bc0 460c10 460c70 460cc0
```

### Unreachable old wrappers removed

An additional **24 definitions / 770 old function-body lines** were deleted,
not counted as C++ migrations. Before deletion, each name occurred only in its
own declaration and definition across the built source, headers, tests and
host tools. The historical `src/decompiled/6kinoko.exe.c` remains unchanged.
Explicit-receiver C++ forms such as `function_4a9730_this` remain available.

```text
function_4a8ea0 function_4a8f90 function_4a91c0 function_4a92e0
function_4a9540 function_4a9600 function_4a96c0 function_4a9730
function_4a97b0 function_4a9840 function_4a98d0 function_4a9a40
function_4a9ac0 function_4a9b40 function_4a9bb0 function_4a9c10
function_4a9d30 function_4a9dc0 function_4a9e30 function_4aa000
function_4aa110 function_4aa2d0 function_4aa970 function_4aac40
```

## Compatibility rules retained

- This is still **MSVC x86**. `SquirrelObject` is 12 bytes, variable metadata is
  20 bytes, a native method descriptor is 8 bytes and the class builder is
  48 bytes. Fixed-width pointer words exist only for the recovered ABI.
  Unaligned records are copied with `memcpy`, not accessed through fabricated
  C++ object layouts.
- Keep the original `__ot`, `__ca` and `__SqTypes` keys and descriptor identities.
  Inherited mapping/ancestor containers are reused, not silently cloned.
  Foreign native receivers are resolved through the original `__ot` map.
- A captured method descriptor follows all user arguments. Its second word is
  a byte adjustment to the native receiver. The last registration parameter
  is `sq_newslot`'s **static flag**, not an argument count.
- Preserve external Squirrel reference ownership. The object-taking native
  method consumes its by-value 12-byte wrapper; caller-side RAII release at
  this boundary would release it twice. Temporary wrappers do use scoped
  release.
- Preserve signed 8/16/32-bit property reads, truncating writes, read-only /
  constant / static flags, strict method argument types, numeric property
  coercions and the low-byte native Boolean result. Constant floats convert
  an integer value rather than reinterpreting its bits.
- Preserve the original string pointer, prefix-buffer and 24-byte MSVC string
  layouts. They are not modern `std::string` objects.
- The quiet table lookup uses source `SQTable::Get` so a missing property does
  not overwrite the previous error. The raw lookup still produces the
  original `getVarInfo: Could not retrieve UserData` error.

## Safety fixes and diagnostic cleanup

The old native-method diagnostics read offsets 28 and 36 from userdata that
contained only an 8-byte method descriptor. That access and raw VM/frame-offset
property diagnostics are removed rather than reproduced in C++.

Method/variable userdata sizes and stack indices are checked before accessing
unchecked Squirrel 2.2.2 getters. Existing native argument helpers reject
payloads shorter than four bytes. Invalid numeric helper conversions no longer
return uninitialized local storage. Valid conversion behavior is preserved,
including zero writes from failed integer-property conversion and no write on
failed float-property conversion.

General diagnostic sinks and attachable tools remain intact. This batch does
not globally compile away VM trace sites and does not introduce new screenshot
capture, rendering suppression or game-logic fallbacks.

## Validation

`tests/squirrel_binding_contract.cpp` links the **real vendored VM** and runs
8 complete root VM lifetimes, including a real child VM in every pass. Only
narrow game-host services are supplied by the test. It exercises:

- Class inheritance, metadata identity, type masks, ownership, canaries and
  unaligned records; native table callbacks and integer/float/Boolean properties.
- Compiled scripts invoking all six method wrapper shapes, strict argument
  rejection, receiver offsets and foreign `__ot` receiver mapping.
- By-value external reference consumption, short userdata, invalid indices,
  constant/static/read-only behavior, string representations and error/stack
  preservation. Child property callbacks must restore the parent's VM context.

Test snippets follow the **2.2.2 parser's statement rules**. A one-line `if`
followed by another statement needs an appropriate statement boundary; the
contract uses line breaks instead of changing the original compiler to accept
newer syntax. Compile failures include the actual VM error.

The Windows workflow builds all targets in quiet and diagnostic configurations
and includes this contract alongside the existing 12 asset-free tests. See the
PR's checks and each artifact's `source-commit.txt` for the precise tested
revision. The local Linux lexer probe only checks script syntax; it is **not**
a substitute for Win32 ABI tests.

## Not changed / remaining work

DAT names, archive search order, EXE-relative asset staging, loading callbacks,
stage setup, input, physics, rendering and the vendored compiler/interpreter
are unchanged. No original loading behavior was synthesized to fit a symptom.

This is not an all-C++ completion claim. Eight inline assembly blocks remain in
the rebuilt C translation unit, with additional low-level runtime shims. Some
retain unrecovered register/receiver contracts and require original-executable
recovery before replacement. Other game-host and engine functions remain in C.

No original EXE/DAT assets or interactive Windows gameplay session are available
in this environment. Asset-free build/tests do not establish visual or gameplay
parity. Before merging, use the existing staging tools and perform the bounded
`AGENTS.md` check: enter the first stage, jump, see an enemy, then quit, with
assets beside the EXE; retain the associated logs and binaries.
