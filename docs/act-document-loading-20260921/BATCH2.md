# Batch 2: partial ACT ownership, separate from post-load behavior

## Evidence and scope

The captured original instructions in
`../decompiler-cleanup-r126/original-stage-evidence.json` show normal stage
cleanup at 465F70: free holder, call document deleting virtual at 465FCC,
destroy/free runtime, free owner. This does not prove an original rejection
branch: 466179 calls 428000 and proceeds directly to the 46618A virtual call.

The rebuilt stage loader *already* rejects failed ACT loads. Before this batch
its allocation guard freed only the 12-byte owner and leaked its partially
loaded document. Its payload parser records completed layers/resources in
owned arrays and their existing destructor handles partial contents. Reuse
that existing destructor rather than inventing an alternate cleanup policy.

The current runtime destructor (`retdec_destroy_act_runtime`) reads its
`source_holder` before deciding whether its ACT is a source borrow or clone.
Stage cleanup freed that holder first. Detach only the references equal to the
owner's holder/document before freeing them. Keep independently cloned ACTs,
keep the documented release order, and still re-read the runtime after the
virtual document-destructor callback (including detaching a replacement).

## Changes

- `kinoko_stage_owner_destroy` is the single consuming cleanup API for a full
  or partial owner. Global stage cleanup uses the same implementation.
- An unpublished stage now owns its document/holder/runtime until successful
  list publication or explicit transfer to the caller when no list exists.
  Rejection and C++ unwind therefore close the reader first, destroy the
  partial document, and finally free the owner; published owners are not freed
  at function return.
- `kinoko_act_source_create_runtime` holds raw constructor storage with an
  allocation guard until 44FDE0 returns, so a throwing FindMap allocation does
  not strand storage. Its port also needs `/EHsc-`, like the earlier reader
  fix. This is synchronous C++ unwinding, not SEH recovery.

## Contracts (results recorded only after commit and execution)

- Existing ACT reader fault-injection contract is now also compiled with /O2.
- `stage_owner_contract` compiles production stage/source/cleanup units and
  injects load/constructor/list-allocation failures. It checks null allocation,
  rejection, unpublished/published ownership, callback runtime replacement,
  source-borrow detachment and single destruction. Host mocks are explicitly
  confined to this contract; it is not a real payload or game test.
- `act_document_lifetime_contract` runs a separate stage-contract mode with
  the real CFileReader, real ACT property/script/layer parser, real source
  holder/runtime and deleting destructor. A generated ACT contains a seven-
  byte relative header skip and an owned layer/script. Every truncated file
  prefix must be rejected and cleaned up once; an exclusive file open inside
  the document destructor verifies reader-before-document closure. Successful
  list publication, repeated clear and caller ownership are also checked.
  Generated .bin fixtures are preserved with CI artifacts. No original DAT
  or graphics startup is used, so these checks do not establish gameplay.

## 46618A remains a separate, unresolved behavior change

The call site proves ECX=document, a stack `Source` argument, and slot 0x18
before holder creation. The retained table maps that slot to 4289C0, but its
C body still has a lost receiver and damaged virtual-call recovery. The
current resource parser already performs resource loads and layout binding.
Calling that body, or simply adding another load pass, would risk invalid
memory and duplicate resource ownership. This batch does not do either.
Full target assembly/dispatch and resource-lifecycle evidence is still needed
before restoring this original behavior; the reference files are unmodified.
