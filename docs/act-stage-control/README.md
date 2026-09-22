# ACT stage control — R1

Continues the completed Mesh / 3D batch (`23b9d5d`, handoff `64ec933`).
Original executable SHA256:
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
Evidence in this directory was obtained from IDA database `84a4091c`.

## Restored behavior

- `4515C0`: set hidden, suspend the optional active document, then the source
  document; return the source callback result. Do not deduplicate documents.
- `4515F0`: clear wake deadline and hidden, resume only the active document.
- `428AF0` / `428BD0`: preserve the document suspension gate, resource order,
  query order (2D, target, Mesh, Chip), and non-short-circuit result accumulation.
  Only 2D textures unload/reload; recognized other resource types are skipped.
  Resume uses the stored resource path. Re-read the resource slot for each query
  and the list end after callbacks, as in the original.
- `428BD0` assembly confirms the reload prefix is a stack argument; decompiler
  ESI pseudo-arguments are not an additional parameter in the restored ABI.
- Replace four legacy C implementations with typed C++ receivers, named fields,
  explicit document virtual methods and resource methods. Existing ownership is
  retained; these transitions borrow their documents and resources.

## Audited existing behavior

`451590` (blocking Sleep), `4515A0` (DWORD wake deadline), `450D80`
(EndStage lock, command/draw cleanup and stage state reset), and `4513F0`
(VM unregister) were compared with the existing native implementation. These
already-correct paths retain their behavior and ownership; this batch does not
claim to migrate all unrelated ACT loading or execution code.

The existing clock contract source now checks suspension callback order,
return propagation, hidden state and wake reset. Per user instruction, only
compile/link validation is authorized: no game, CTest or automated test execution.
