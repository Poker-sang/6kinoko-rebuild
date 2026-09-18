# PR #2 continuation: native calls and ACT objects in source C++

## Review decision and integration base

Retain PR #2 (`f2a6d0f993f227bfe69d5f607095946f4d64664d`) and continue on
`refactor/sqrat-native-cpp-20260918`. Its actual bridge implementations were
reviewed against the preceding recovered C bodies, not accepted solely from
the PR description or a passing check.

The useful parts are the distinct Sqrat/SqPlus layouts and external reference
ownership, normal versus raw slot behavior, non-growing stack trim, and native
field coercions. In particular, calling `SQDelegable::SetDelegate` directly
preserves the prior failure/error state; replacing it mechanically with the
public delegate setter would not be equivalent. These choices and the two
real-VM contracts from PR #2 are retained.

Master `826b98421aa304f9ddc5c22efbac9a61eb6db5ba` already includes PR #3's
SqPlus class/property/method work. It is integrated without dropping either
branch's implementation or tests. The conflicts were in the main C include
list, CMake target/test registration and the CI test selection. PR #3's work
is **not** counted again below. `AGENTS.md` was read before this batch.

## Actual new migration

| New module | Recovered definitions moved | Responsibility |
| --- | ---: | --- |
| `squirrel_native_calls.cpp` | 22 | Property dispatch, weakref, thiscall/cdecl/native callback adapters |
| `squirrel_game_objects.cpp` | 8 | Instance publication, ACT callback/root ownership, memory bytecode reader and execution |
| `squirrel_table_values.cpp` | 4 | Source object/pair/string construction and owned copies |

This is **34 additional C function definitions / 617 original body lines**.
With PR #2's preceding 45 definitions / 844 body lines, the PR contains **79
migrated definitions / 1,461 old body lines** relative to the merged master.
There is no additional dead-wrapper deletion counted as migration in this
continuation. The rebuilt C file is 194,035 lines versus 195,506 at that master.

Native call functions (prefix `function_`):

```text
41e260 41e2c0 431650 445730 4552e0 4555a0
46b490 46b500 46b610 46b6f0 46c6b0_pair
46ce70 46cec0 46cf10 46cf60
4716b0 471a60 471720 471d30 471e50 471f70 472030
```

Object/lifecycle functions:

```text
function_4029b0 function_402a50 function_45e020_this
retdec_create_bound_instance retdec_create_unbound_instance
retdec_copy_act_callback retdec_bind_act_resource_root
retdec_execute_embedded_act_script
retdec_squirrel_object_from_pair retdec_squirrel_object_string
retdec_squirrel_object_copy retdec_squirrel_object_from_string
```

Address-named exports now exist only as compatibility entry points. These
three new modules use the actual vendored 2.2.2 APIs and existing typed ABI
adapters, with no `function_48xxxx` VM API calls or inline assembly.

## Behavior retained rather than modernized by assumption

- **Four different layouts:** 12-byte SqPlus objects; 20-byte Sqrat wrappers;
  20-byte ACT callbacks `[VM, environment, closure]`; and a 28-byte call state
  embedding two 12-byte SqPlus objects. Raw embedding records are byte-copied,
  including unaligned storage, not treated as live modern C++ objects.
- **Ownership transfer:** native callbacks consume one or two externally owned
  12-byte objects passed by value. They are deliberately trivial records, not
  RAII parameters that would release them again in the caller. Temporary
  callback records and root handles retain their original ownership roles.
- **Calling conventions:** cdecl explicit receivers are separate from x86
  thiscall methods. Draw calls preserve float bits, argument order and the
  old default-zero conversion behavior. Integer/Boolean method families keep
  their different return rules: the cdecl two-argument Boolean uses the entire
  returned word, while the pre-existing SqPlus family uses its low byte.
- **Arguments and errors:** strict string/integer callbacks remain strict;
  the existing one-integer thiscall adapter still permits numeric coercion.
  Pair conversions remain positive-index-only. Optional null cdecl callbacks
  still no-op or return zero where the recovered functions did so.
- **Nested call state:** property dispatch intentionally ignores an inner
  `sq_call` status and lets the outer frame clean up. The 28-byte callback
  state pops closure/result on success, retains the closure on failure, and
  destroys its temporary in either case. A generic stack guard would change
  these conventions and is not used there.
- **Instance construction:** `sq_createinstance` does not execute the script
  constructor. Bound instances publish through `sq_newslot`, not raw set.
  Error handling still releases a newly acquired output handle and trims only
  excess stack entries. In this source version, `sq_newslot` returns success for
  non-table/class parents without publication; the helper preserves that success
  and its owned instance output. The native pointer and class are unchanged.
- **Embedded bytecode:** the existing script record selects its same data,
  size and environment. The `0xFAFA` tag is required. The real source
  `sq_readclosure` uses the bounded memory callback; the loaded closure is
  duplicated before calling it with that environment, not the global root.
- **Strings:** bounded text retains the old first-NUL truncation, including
  embedded NUL and empty values. It does not silently turn into a binary
  string API. The extra malloc/copy buffer is removed; failed extraction
  still leaves the caller's output pointer untouched.

## Safety and diagnostic changes

Resource root assignment now snapshots and acquires the incoming handle
**before** releasing the old one. Passing a resource's own saved root could
previously release the last strong owner before reusing that root. The new
contract exercises that exact alias case with a weak reference and unaligned
storage. The supplied VM remains the reference-table owner, as before; this
change does not invent cross-VM sharing rules.

The new callback adapters check frame bounds and minimum captured-userdata
size before Squirrel 2.2.2's unchecked accessors, and check index arithmetic
without signed overflow. Truncated memory streams and nonpositive requests
are bounded without writing beyond the requested output. Self-copying an
already owned object does not erase its handle before reading it.

The migrated ACT/native wrappers no longer dereference VM/frame internals or
read script path storage solely for verbose diagnostics. Compact metadata and
failure markers remain. The general trace sink and attachable tools are
unchanged; no global compile-time elimination of VM tracing was introduced.

## Validation contracts

The Windows x86 workflow now includes **17** asset-free contracts in both
quiet and diagnostic Release builds: all prior 15 plus these two new targets.

`tests/squirrel_native_calls_contract.cpp` executes source-compiled scripts,
real native closures and real instances. It covers property lookup/error/stack
behavior, all new method shapes, float arguments, full-word Boolean results,
strict versus coercing conversions, userdata sizes/tags, extreme indices,
optional targets and by-value reference consumption. Native ownership tests
use release hooks to require exactly-once destruction after the native frame
has released its stack values.

`tests/squirrel_game_objects_contract.cpp` exercises constructor non-execution,
bound/unbound publication, invalid-class rejection, nonpublishing newslot
success, root self-binding, callback replacement
and missing lookup, reference/weakref lifetime, actual source serialization and
bytecode execution, truncated streams, supplied environments, call failure
cleanup and bounded/NUL-containing strings. Buffer canaries check that copied
records and stream writes do not affect neighboring bytes.

Each new target executes eight complete root-VM lifetimes and a real child VM
in every pass. Only narrow host services are implemented by the test; there is
no fake VM. Checks use throwing assertions that remain active in Release.
The early fixture fix avoids a variable name colliding with Squirrel's `type`
macro and uses the supplied parser's statement boundaries; interpreter source
was not modified to accept the tests. A subsequent fixture correction asserts
2.2.2's nonpublishing `sq_newslot` success rather than expecting an error.
Precise final checks, revision IDs and
artifact hashes are recorded in the PR description.

## Limits and unchanged areas

Vendored Squirrel sources, original decompilation reference, DAT names/search
order, archive selection, stage setup, physics, rendering, working-directory
policy and asset staging tools are unchanged. No game-specific fallback or
loading rule was added to satisfy a subjective symptom.

Eight main-file assembly blocks and the low-level runtime shims remain, along
with additional C host/engine functions. Removing them requires recovery of
remaining receiver/register contracts, not guessed zero-argument substitutes.
This is not a claim that the entire repository is pure C++.

There is no original EXE/DAT or interactive Windows gameplay session in this
environment. Asset-free CI is not visual/gameplay parity evidence. Before
merging, keep the three DATs beside the built EXE and perform the bounded
`AGENTS.md` check: enter the first stage, jump, see an enemy, then exit. Keep
associated binaries/logs; all CI artifacts, including failed batches, remain
retained according to the workflow's retention period.
