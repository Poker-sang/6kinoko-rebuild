# Continued C++ boundary migration (2026-09-18)

Starting PR revision: `db658e1656a42f5bdc403f24e0a2129b6a1e3328`
(the code baseline is `30848eeb26b4736f5b9ce621c23930c2111eb73b`; the next
commit only recorded its source snapshot).

## What changed

The previous batch already linked the vendored Squirrel 2.2.2 compiler,
interpreter, object/GC implementations, serialization and standard libraries.
This batch preserves that architecture rather than introducing another VM.

* 17 game-side method entry adapters now use explicit Win32 C++ signatures.
  The first fastcall argument occupies ECX; a deliberately unused argument
  reserves EDX; the remaining arguments retain the original stack layout and
  callee cleanup. Their recovered C game bodies are unchanged. The old vtable
  fields are address-only storage: dispatch still goes through `legacy_abi.h`,
  not a call through the decompiler's cdecl field type.
* The remaining SQRefCounted deleting-destructor entry now invokes the
  source-qualified base destructor and source allocator. VM shared-state
  queries use the source `SQVM` type instead of embedding offset +140.
  The pre-existing no-op constructor is retained as a named embedding callback;
  it does not allocate, initialize extra state or push a return value.
* Removed nine unreferenced assembly entry shims, plus the unreferenced
  missing-receiver SQObject constructor and old set-error-handler wrapper.
  The immutable `src/decompiled/6kinoko.exe.c` remains the original reference.
* Ordinary recovered CRT/Windows wrappers are in a separately compiled C++
  module, with C exports, private helpers and typed dynamic-import resolution.
  The range validator is shared with the remaining register-only C boundary.
* Recovered MOVS/STOS helpers and the FPR scratch bank use typed C++ interfaces.
  STOSD and truncated GUID reads use `memcpy` rather than unaligned typed
  dereferences. Counts, return values, unsigned modulo FPR indexing, and the
  process-shared scratch bank are unchanged.
* `errors-only` selection now takes precedence even when verbose tracing is
  off. Other filter prefixes and output-sink behavior are preserved. No live
  VM trace call sites, loading branches or screenshot tools were substituted
  for game behavior; diagnostics remain opt-in.

Inline assembly blocks in the rebuilt game translation unit: **35 -> 8**.
The register-only runtime boundary still contains **2** blocks. These counts
exclude the immutable decompiler reference and third-party source.

## Regression coverage

Added asset-free Release-enabled contracts (not `assert`-only tests):

* `legacy_method_entries_contract`: all 17 migrated entries, 10,000 iterations,
  receiver and result preservation, integer/char/float argument bits, callee
  stack cleanup, and an unaligned ClassType getter.
* `retdec_memory_contract`: all recovered memory helpers, exact DWORD counts,
  sentinel bytes, offsets 0..3, nonpositive counts without memory access, and
  positive/negative FPR index wrapping.
* `diagnostics_filter_contract`: explicit errors-only policy in both verbosity
  modes, retained nonverbose scene/error selection and graphics/audio filters.
* `squirrel_refcount_contract`: deleting/nondeleting base destruction and weak
  reference invalidation, null compatibility boundaries, child shared state,
  and the unchanged no-op constructor stack contract.

Both Windows x86 CI configurations run these in addition to the seven existing
asset-free contracts. Source revisions, logs and executable artifacts are
recorded per run. Portable memory/filter tests are also run under GCC and Clang
with AddressSanitizer/UBSan. CI status and the exact tested revision are recorded
in the PR after execution; this report alone is not a claim that CI passed.

## Deliberate boundaries / not claimed

This is **not** a conversion of all game code to C++. The remaining large C
translation unit still contains game/codec/CRT remnants and unresolved
register-receiver call sites. In particular, the remaining inline assembly
adapts 405800, 43CF20/43D110, 450F30, 45E300, 45E460, 4606D0 and the ESI reader.
`_memcpy2` still uses the old caller-frame recovery, and `__ftol` consumes x87
ST(0). Removing those without first reconstructing their callers would only
hide missing arguments. They were not replaced with guessed game logic.

No original EXE/DAT assets or interactive Windows debugger were available to
this batch. Asset-free CI cannot establish visual/gameplay parity. DAT staging,
EXE-relative lookup, existing resource order, stage logic and manual-smoke
scope (enter first stage, jump, see an enemy, exit) remain unchanged. Prior
build/runtime artifacts and the vendored Squirrel/zlib sources are retained.
