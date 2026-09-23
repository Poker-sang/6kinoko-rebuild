# Address-named function bodies at zero (2026-09-23)

The unchanged `tools/audit_readability.py` inventory reports **0**
`address_named_legacy` functions and **0** associated body lines at source
commit `ad06c11`, versus 123 functions / 439 lines at `6f43923`.
The lexical scope is the C/C++ source paths literally listed in top-level
CMake. This is the requested address-body milestone, not full C++ recovery or
proof of original game behavior. Afterward the inventory still reports
197 `named_mixed_legacy` functions / 4,301 lines and 286 `thin_bridge`
functions / 829 lines across all names, plus 1,411 named structured
candidates / 14,214 lines. The remaining bridges retain original registration
and C ABI entrypoints; their bodies now delegate to named implementations.

- Batch 60, source `d80bb4a`: Squirrel native callback entries and four save
  table/file entries use named implementations with typed VM handling. IDA
  4555A0 confirmed the native draw argument conversion order; 4722E0 and
  472820 confirmed the recursive table serialization entries.
- Batch 61, source `5316b9d`: the source-library VM entries moved their
  implementation behind `SQVM*` interfaces, leaving word-to-pointer
  conversion at each original ABI boundary.
- Batch 62, sources `7aa935f` and fix `1271f08`: SqPlus object ownership,
  userdata finalization and error object handling gained typed interfaces.
  The formatted-error function has no game caller/registered address, so
  test callers use its named API. The first build failed because
  `SQVM::Raise_Error` needs a mutable `SQObjectPtr`; the correction was
  committed and built independently.
- Batch 63, source `9a2f879`: class registration and script-object VM entries
  delegate to named typed implementations, retaining Sqrat root and pair
  release order.
- Batch 64, source `02c916b`: the final game C address entries became named
  implementations or short ABI bridges. IDA 4252E0 supplied the missing
  ECX receiver and virtual slot +24; IDA 44FD30 supplied the IColor virtual
  destructor receiver and delete flag. The reconstructed color vtable now
  contains a linked function pointer rather than the original numeric code
  address. The five borrowed class type-property getters return typed
  pointers. IDA 43E100 is an empty dprint callback.
- Batch 65, source `c4962f8`: further typed Squirrel object, callback,
  typetag, closure and userdata arguments on the named implementation side.

Each successful source commit preceded its own unique quiet Win32 Release
build. Each build copied only the three original `6kinoko_*.dat` files next to
its EXE and checked their size and SHA256. Builds and logs, including batch
62's failed build, are retained. The latest artifact is
`runtime-builds/readability-typed-squirrel-boundaries-quiet/kinoko_retdec_rebuild.exe`,
SHA256 `E0EE37AF544471251FA79284E494B7C661424D2A638E3503DDB508C0AC7CF37A`.
No game session, CTest or contract executable was run by the agent. Contract
targets compiled as part of the build; their assertions were not executed.
The pre-existing modification to
`docs/decompiler-cleanup-r126/original-stage-evidence.json` was not included.
