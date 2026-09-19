# Expanded input and script module migration

Tested implementation: c4270eaf18a00300de5a3c17e100efedffa06220.

This batch moves 19 address-named functions and four table stream helpers from
the main C file into four C++ modules:

- input_aggregation.cpp: original 4077C0 InputCluster virtual Update. Replaces
  1,786 lines of expanded deque iterators with four-pointer block traversal,
  signed directional magnitude selection, twelve button counters, zero-count
  release edges, last-device selection and six analog axes. The existing
  manager-specific retdec_update_input_cluster remains unchanged; it uses a
  different host storage representation and is not silently redirected here.
- input_runtime.cpp: six Input configuration/save/load/assignment/update
  functions. Named strides and publication mappings replace repeated copies.
  Existing exclusions, broadcast behavior, failure handling and diagnostics
  are preserved; no speculative new boundary handling was added.
- camera_map_binding.cpp: both class builders and registrations, 19 fields,
  Camera native receiver and callback wrapper. ClassType descriptors now use
  explicit contiguous records. The receiver uses a real eight-byte HSQOBJECT
  instead of taking an eight-byte object API output into a four-byte local.
  Class output construction restores the original explicit object receiver.
- table_serialization.cpp: recursive load/save and compressed file entry
  points. Wire tags now use Squirrel enum names; identical integer/float read
  paths are combined. Existing null skipping, counted strings, one-byte bools,
  array lengths, limits, compression and ownership conventions are retained.

Evidence is saved from IDA MCP for the changed original entries. The deque
layout and iterator expansion are also supported by 4077C0 entry assembly and
408530/408550. All 19 Camera/Map field names, types, offsets and order were
compared with the original decompilation (field-verification.json).

Metrics:

- Main C: 92,765 -> 89,841 lines (-2,924).
- Main C address definitions: 1,327 -> 1,308 (-19).
- New runtime source/header lines: 871; net reduction: 2,053 lines.
- Evidence and tests are excluded from runtime line counts.

Validation:

- Win32 Release quiet and diagnostic builds: 46/46 CTests each.
- New contracts cover deque block wrap, opposite signed directions and ties,
  all twelve buttons, held/released edges, device priority, empty frames,
  Input config two-record roundtrip/broadcast, assignment offsets and original
  excluded keys, Camera/Map native field writes and Camera callback ownership,
  compressed nested table/array/scalar roundtrip and skipped null values.
- Initial e5436a0 builds passed; the new table fixture required a newline
  between Squirrel declarations. The test-only fix passed both full suites.
- Migration boundary check passed.
- Three DAT files staged beside both main EXEs, verified by size and SHA256.
- Run/build names: modules-c4270ea-{quiet,diagnostic}-20260919. Maps, logs,
  test files, earlier failed artifacts and source-commit.txt are retained.
- No graphical smoke test: the user's actorbind-e2673d6 game was running
  (PID 33620 when checked) and was left untouched.

Other saved source excerpts are investigation evidence, not claims that those
functions were migrated. No unrelated function is declared unnecessary.
