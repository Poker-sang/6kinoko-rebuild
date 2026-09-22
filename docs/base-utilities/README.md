# Base Utilities

This batch finishes the selected compression, process-path and critical-section
entries and removes two superseded deque iterator bodies. It is not a claim that
every remaining utility or renderer address entry has been migrated.

Original executable: `../6kinoko/6kinoko.exe`, SHA256
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
The JSON files here are fresh IDA MCP decompilation / cross-reference exports.

| Original | Disposition |
| --- | --- |
| 404390 / 404430 | `kinoko_compress_buffer` / `kinoko_decompress_buffer`; typed input/output, scoped zlib state |
| 408650 | `kinoko_process_initialize`; instance, HWND and EXE directory in a named context |
| 4086E0 | `kinoko_path_split`; typed strings, drive+directory and optional filename+extension |
| 4087E0 / 408830 | `kinoko_critical_section_construct` / `kinoko_critical_section_destruct` |
| 408800 | `kinoko_critical_section_delete`; recovered ECX receiver, low-bit storage release |
| 401040 | `kinoko_graphics_initialize_runtime`; same lock/listener/device initialization order |
| 408530 / 408550 | Removed unused, receiver-damaged deque iterator remnants |

## Semantics and boundaries

The codec continues to use vendored zlib **1.2.3**, matching the original version
string. Compression performs one deflate(Z_FINISH); decompression performs one
inflate(Z_NO_FLUSH). Both accept Z_STREAM_END or Z_OK with spare output. In
particular, the original can return decoded bytes for a truncated stream with no
checksum yet: do not silently replace this contract with strict uncompress().
Existing argument guards and cleanup on failed operations are retained; original
early error returns could leak zlib state. Save wire tags, capacities and file
layout are unchanged. Save/load consumers no longer cast buffers into integers.

The process initializer restores the previously discarded instance handle from
ECX (original global 51B060). Existing HWND consumers still borrow g767 at one
explicit compatibility boundary. The directory cache is a real MAX_PATH array.
Original slash-before-backslash lookup and trailing separator are preserved.
The original only caches its module directory; SetCurrentDirectoryA is an
existing reconstruction measure retained to resolve relative resources beside
the staged EXE. No reference-directory or data-dir override is introduced.
Path splitting returns the CRT status; its sole production caller ignores it.
Existing malformed-path guards remain, and temporary buffers have consistent
MAX_PATH bounds rather than reproducing the original 256/260 mismatch.

Common::CCriticalSection is explicitly 28 bytes on Win32: a methods pointer and
the 24-byte CRITICAL_SECTION at offset 4. Its one-slot scalar-deleting destructor
uses the recovered receiver, calls DeleteCriticalSection, and frees that same
receiver only when flags bit zero is set. The old damaged body used an
uninitialized receiver and freed g1224. Storage uses the host's existing CRT
malloc/free allocation family. A fastcall adapter supplies ECX and reserves EDX
for the recovered thiscall boundary, as elsewhere in this project.

Graphics lock storage is one named record instead of independent g675/g676
globals with implied adjacency. All graphics, presentation and font users now
access its native field; BeginScene/EndScene continue to share the same lock
acquisition. Audio's borrowed methods identity points at the recovered methods
table. Initialization and shutdown timing are otherwise unchanged.

Current src/include/tests have no references to the two removed iterator helpers
(excluding the untouched original decompile). Original xrefs show input/deque
callers; their live counterparts already use native containers in
input_aggregation.cpp, input_copy.cpp and input_runtime.cpp. See also
analysis/script-modules-20260919/report.md. Removal does not remove input behavior.

## C++ exception boundary follow-up

Review also caught a missing annotation in Script File Execution R1: under MSVC
/EHsc, C linkage calls are otherwise assumed not to throw. Script-file entry
declarations now explicitly use noexcept(false), so the original SqPlus exception
can unwind the owning CompileFile argument. C++ consumers share the authoritative
header; compile-time assertions cover the contract. The exception type and script
success/failure policies are unchanged.

## Verification policy

Compile/link only, per the user's test handoff. The new base utility contract
contains a known zlib fixture, exact-capacity/truncation/checksum cases, roundtrip,
path decomposition and directory-only cases, lock recursion/layout canaries, and
deleting/nondeleting virtual calls. Existing application/device contracts use the
new lock record; script contracts assert the throwing C++ declarations.
No game, CTest or contract executable is run by the agent.
