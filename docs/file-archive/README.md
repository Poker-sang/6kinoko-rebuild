# File I/O and DAT access

Reference: C:/WorkSpace/6kinoko/6kinoko.exe, SHA256
2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
IDA MCP session 84f4b757; this folder contains decompilations and the original
virtual tables / critical instruction sequences. The untouched reference C is
src/decompiled/6kinoko.exe.c. This batch changes file host ports, not Squirrel VM
semantics or its source-backed implementation.

## Recovered interfaces and ownership

- CFileReader is 12 bytes: methods, HANDLE, transferred count. Package reader is
  28 bytes: that prefix, entry size (+12), offset (+16), read position (+20), XOR
  key (+24). Native compile-time assertions pin the layout.
- All seven reader virtual slots now have correct thiscall receiver contracts;
  C++ fastcall adapters bridge ECX explicitly. Slot 1 (4072B0) opens a legacy
  string via slot 2; it was incorrectly implemented as a count getter. Slot 4
  (466490) really returns the transferred count. Writer shares the first six slots.
- 407270 uses FILE_READ_DATA (1), share read/write, OPEN_EXISTING. 40D520 uses
  GENERIC_WRITE, exclusive sharing, CREATE_ALWAYS. Read/write/seek now have real
  pointers and HANDLEs instead of uninitialized receiver locals.
- 407370 replaces/closes an existing stream, chooses ordinary/package at open
  time and releases failed opens. Readers subsequently identify their own type;
  changing mount count no longer reinterprets an existing 12-byte ordinary stream
  as a 28-byte package. Null/allocation guards are reconstruction safeguards.
- File/archive address-named bodies and old generated virtual tables are removed.
  The index remains standard map/list/vector storage, with named mount/insert/
  entry-open APIs and native returned HANDLE ownership. Original lowercased CRC
  includes NUL; lookup removes exactly ./ before slash conversion; case folding,
  collision-chain comparison and duplicate replacement remain unchanged.
- Main script/image/CSV, audio, ACT, MCD, PAT, mesh and script binding callers use
  typed open/close/size ports. Integer adapters remain only for unmigrated parser
  record/virtual boundaries in file_io_legacy.h; they contain no I/O implementation.
- Mesh ReaderOwner previously passed its stack pointer slot to the heap reader
  destructor. It now closes the owned stream itself. This is a reconstruction
  ownership bug, not behavior attributed to the original.

## Original quirks versus existing guarded loader behavior

IMPORTANT ORIGINAL QUIRK: 410C00 stores entry-relative position after Seek,
although 407370 initializes and 410B90 advances/checks an absolute position.
FILE_END also subtracts distance. The virtual implementation preserves these
facts, with a prominent code comment; the reconstruction's swapped size/offset
fields and extra seek have been removed. This does NOT prove the original game
exercises every seek/read combination.

410B90 ignores ReadFile's BOOL and XORs the clamped requested byte range when
some bytes were read, even on a short physical file. The original virtual method
retains this behavior. Ordinary virtual Read returns ReadFile's BOOL, including
success with zero bytes at EOF.

Existing validated asset loaders keep their separate exact-read/relative-skip
contract: reject short reads or out-of-entry spans, retain absolute position,
and decode a complete read. This is established reconstruction compatibility,
not a claim that the original virtual methods performed these validations.
An overflowed entry end is now rejected explicitly. Archive index size/length
checks and its prior partial-publication behavior on malformed entries remain.
No loose-file fallback is introduced when packages are active; resource paths
and the EXE-directory DAT convention remain unchanged.

## Validation plan

Compile/link all targets only. New file_archive_contract uses production I/O and
archive code to cover virtual read/write/count/string-open dispatch, ordinary EOF,
DAT mount/lookup/case/slashes, XOR, exact-read bounds, object type independence,
reopening/failed-open release, and all three original seek origins. Existing ACT,
PAT, mesh and audio fixtures are updated for typed ports; legacy stage fixtures
explicitly identify their synthetic package records. No game, CTest or contract
executable is run, per user instructions. Each build revision is committed first
and gets separate English build/runtime directories; all artifacts are retained.
