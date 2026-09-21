# Native container continuation after PR #7

## R107 — Input cluster registration, copy, update and destruction

Baseline: master f9e9d7f, which merges PR #7. Original IDA queries are retained in input-cluster-evidence-r107.json.

The Input cluster now owns a real std::deque<int32_t> behind an opaque pointer at cluster+172. Initialization registers the existing default keyboard record. Removed the old block-map allocation/growth, wrapped-index copy loops and generated deque destructor. Copying preserves shallow device pointers (including pointers into the source Input), rather than inventing destination rebinding. The native virtual deleting entry clears container ownership, restores CInputManager identity and honors the storage flag; the old receiverless base destructor is removed.

The live Input::Update previously bypassed the recovered 4077C0 implementation with a second C routine that copied keyboard state and merged only twelve count fields using different tie/edge rules. It now uses 4077C0 and the actual deque. Original 4077C0 clears 96 state bytes; compares signed magnitudes for two direction fields; merges twelve buttons; preserves earlier equal values; accumulates release edges only at zero; and selects six float axes by strict magnitude. The existing source-backed merge implements those rules. Original 46E6F0 registers optional controller records before the keyboard. This batch does not implement the still-missing optional-controller vector population; the current initializer has only the keyboard record. Device-vector and key-byte-vector emulation in input_copy.cpp remain explicit future work.

Compiled-only contracts cover deque growth through forty pointers, shrinking/empty/self assignment, independent storage with borrowed values, signed direction ties, all twelve buttons, release edges and scalar retained-storage destruction. No game, ctest or contract executable is run. Quiet mode still silences trace output only; no VM trace calls are removed.

## R108 — Input key registration

Replaced the fixed 32-byte global registration buffer and emulated byte-vector assignment with std::vector<uint8_t>, owned through tracker+1024. Original 408430 suppresses duplicate scan codes and grows dynamically; 4082D0 clears 256 counters, registration and three modifiers. Copy retains independent registration storage and copies counters/modifiers. Per-frame 408320 now uses native accessors. The old proxy slot is not interpreted as a modern STL object. Contracts register all 256 scan codes twice and check uniqueness/order, independent copy, smaller/empty/self assignment and destruction; they are compiled only. The process-global Input remains process-lifetime storage, as before.

## R109 — Input device records

Replaced manual contiguous buffer assignment/allocation/destruction with std::vector<Device> behind manager+180. Device copy construction installs the base vtable, assignment preserves existing identity and destruction dispatches the original scalar destructor. Configuration read/write, assignment, per-frame update and copy use native storage accessors. Initialization now sizes the vector to the enumerated controller count and follows original 46E6F0: twelve default button bindings per controller, controller pointers first and keyboard last in the cluster deque. Copy still intentionally keeps shallow cluster pointers. Input container contracts are updated for native construction/destruction and compiled only.

## R110 — String glyph deque

CStringLayout owns std::deque<Glyph> through layout+176; manual block maps, rotation, capacity growth and spare block ownership are removed. Glyph is the 256-byte engine record, with copy construction installing CSpriteEx and assignment preserving the destination vtable. Sprite destruction resets base identity but owns no atlas/texture. Replication and pop retain the original explicit atlas reference accounting; clone copy/drop intentionally makes no reference adjustment. Render, layer assignment, cache collection and rebuild traverse native accessors. Updated lifetime/replication/cache contract sources are compiled only. Atlas vector and renderer pixel list remain separate pending migration.

## R111 — Atlas vector and renderer pixel list

CStringLayout now owns std::vector<Atlas>; native element constructors, assignment and destructors preserve the recovered renderer/string operations and borrowed texture behavior. Collection uses vector erase; clone uses vector assignment followed by clear, without adding texture releases. Rasterizer scratch-pointer list now uses std::list<void*>. Original 445230 list assignment is shallow and discards old list nodes without freeing pointed-to buffers; renderer clear/destruction explicitly frees buffers. Manual vector capacity growth, list nodes and sentinel allocations are removed. Runtime atlas lookup and compiled-only cache contracts use native accessors. This does not implement previously absent generic rich-text engine branches.

## R112 — Audio request container ownership

Removed manually linked audio handle queues and per-insertion reallocation of parallel handle arrays. std::list<uint32_t> owns queue nodes; BufferStore owns std::vector<unique_ptr<BufferRecord>> and std::vector<uint32_t> generations. Buffer paths now own real std::string storage through a pointer-sized boundary field, retaining the surrounding verified buffer offsets. Queue nodes carry handles only; decoder and DirectSound ownership remains in BgmTrack. Construction failures do not publish partial handles. Existing generation/lookup and FIFO contract sources now address native containers and strings. This replaces container ownership in the reconstructed manager; it does not claim the full original audio manager scheduling/recycling policy is reconstructed.

## R113 — Global stage owner list

Replaced the global stage-owner list with std::list<StageEntry>. Opaque entry tokens hold native iterators, so update/draw walks retain insertion order and stable traversal without exposing an STL layout. Stage insertion and all three C update/draw consumers now use accessors. Payload destruction remains separate from list-storage destruction as in 465F70/4D47F0. Removed the receiverless generated list clear and the final manual 4214A0 node allocator. Cleanup/update fixtures now construct native storage. No game or local automated test is executed.

## R114 — Actor priority index

Replaced the hand-written unbalanced priority tree with std::multimap<int32_t, unique_ptr<Entry>>. Entry tokens hold real map iterators, and actors retain opaque tokens for priority reset/erase. Original 463610 compares signed priorities; normal insertion puts equal keys after existing entries, while the insert-left variant puts equal keys first. lower_bound/upper_bound hints preserve both policies. Refresh and shutdown use native ordered traversal and preserve actor reference/handle release order. Removed manual parent/left/right rewiring, extrema maintenance and recursive priority-node frees. Compiled-only contracts cover signed ordering, equal-key insertion policy, successor erase and repeated clear.

R114 build note: the game and stage contract linked, but the complete build failed because actor_records_contract could not resolve kinoko_priority_clear. R114 is not a successful full-build artifact. Its build/run directories, logs and staged resources are retained. R115 moves the standalone container implementations into kinoko_native_methods so every consumer links them.

## R115 — Animation and sound integer lookup maps

Replaced the hand-written animation lookup BST and empty sound lookup tree with std::map<int32_t,int32_t>. Put retains stable mapped-value addresses and replaces duplicate values; lookup returns the opaque container identity on a miss, preserving call-site branching. PAT aliases, SetTake, manager cleanup and sound shutdown all use native storage. Removed generated 4706C0 pointer traversal, 429C70 recursive frees and sentinel allocation. actor_records_contract now uses the real lookup map instead of a fake node-layout stub. Contracts also cover negative keys, overwrite, growth without mapped-address invalidation and missing lookups.

## R116 — Owning animation list and frame vectors

Original 464F80 inserts animation records into manager+52's owning list (465347..465377) separately from the engine's next/previous animation links. The reconstructed parser had allocated detached records and never linked that owning list. It now adopts completed records into std::list<unique_ptr<Animation>>, with std::vector<FrameRecord> owning each fixed-size frame array. AnimationRecord publishes borrowed begin/end pointers for engine readers; it is not a modern STL overlay. Failed pending records use the same destructor to free frame payloads and storage. Manager clear releases actors before animation ownership, as before. Cleanup contract fixtures use actual native owners.
