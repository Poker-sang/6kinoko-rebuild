# Native container continuation after PR #7

## R107 — Input cluster registration, copy, update and destruction

Baseline: master f9e9d7f, which merges PR #7. Original IDA queries are retained in input-cluster-evidence-r107.json.

The Input cluster now owns a real std::deque<int32_t> behind an opaque pointer at cluster+172. Initialization registers the existing default keyboard record. Removed the old block-map allocation/growth, wrapped-index copy loops and generated deque destructor. Copying preserves shallow device pointers (including pointers into the source Input), rather than inventing destination rebinding. The native virtual deleting entry clears container ownership, restores CInputManager identity and honors the storage flag; the old receiverless base destructor is removed.

The live Input::Update previously bypassed the recovered 4077C0 implementation with a second C routine that copied keyboard state and merged only twelve count fields using different tie/edge rules. It now uses 4077C0 and the actual deque. Original 4077C0 clears 96 state bytes; compares signed magnitudes for two direction fields; merges twelve buttons; preserves earlier equal values; accumulates release edges only at zero; and selects six float axes by strict magnitude. The existing source-backed merge implements those rules. Original 46E6F0 registers optional controller records before the keyboard. This batch does not implement the still-missing optional-controller vector population; the current initializer has only the keyboard record. Device-vector and key-byte-vector emulation in input_copy.cpp remain explicit future work.

Compiled-only contracts cover deque growth through forty pointers, shrinking/empty/self assignment, independent storage with borrowed values, signed direction ties, all twelve buttons, release edges and scalar retained-storage destruction. No game, ctest or contract executable is run. Quiet mode still silences trace output only; no VM trace calls are removed.
