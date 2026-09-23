# ACT load-to-frame continuation — 2026-09-23

At source commit `17fdb34`, the unchanged lexical readability inventory reports
137 `named_mixed_legacy` functions / 2,172 body lines across the scoped C/C++
files, down from 156 / 2,692 at the start of this continuation. The targeted
`act_document.cpp`, `act_lifetime.cpp`, `stage_runtime.cpp`, and
`act_frame_render.cpp` each now report zero mixed bodies. These counts are
lexical estimates, not proof of complete C++ recovery or original behavior.

- Batch 70, source `3625695`: the layer's embedded name and document-owned
  layer/resource spans use offset-checked byte record views on load and
  teardown. IDA 41F800 confirmed the key, timeline and script read order.
- Batch 71, sources `04a107b` and correction `7b6b809`: stage traversal uses
  the list sentinel API and frame cleanup/drawing uses the host's method
  table identities. The first build could not link a new string-layout
  helper into the frame contract; an inline lookup fixed that in a separate
  build directory.
- Batch 72, source `1ff86be`: texture name normalization and mesh controller
  child dispatch moved from the decompiled C file into their C++ resource
  files, retaining their C ABI entries. IDA 457A10 confirmed child bounds
  +156/+160 and the manager's virtual slot +12.
- Batch 73, sources `470d356` and correction `17fdb34`: stage list and sound
  lookup ownership is grouped behind named accessors. Contract fixtures
  provide and inspect the original ABI globals, so the first rename failed
  to compile/link those contracts; the correction retained those symbols.
  Both failed build trees remain available for review.

Each successful source commit preceded a distinct quiet Win32 Release build
with the three original DATs copied and SHA256/size-verified beside its EXE.
The latest artifact is
`runtime-builds/stage-list-ownership-quiet-r73-fix/kinoko_retdec_rebuild.exe`,
SHA256 `1E4FA22C26CA03402ED84AB56FB3E70A88086ED56430AE4544541CA44D924A72`.
Contract targets compiled; the agent ran no game, CTest, contract executable,
or local automated test. Earlier successful and failed artifacts were retained.
The pre-existing edit to
`docs/decompiler-cleanup-r126/original-stage-evidence.json` remains untouched.
