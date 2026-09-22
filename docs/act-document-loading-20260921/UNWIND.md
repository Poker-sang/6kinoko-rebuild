# ACT reader exception-unwind correction

The first quiet MSVC x86 run (35577644633, source ceb5965cb650a7055301c83a573a34ef2e7f92d7)
compiled all targets but exposed a reader leak in the injected C++ exception case.
The ordinary result/header/seek failure cases passed before that assertion.

The project defaults to `/EHsc`, which assumes C-linkage functions cannot throw.
The payload parser is reconstructed C++ exposed via an `extern "C"` port; the
scoped reader must not inherit that assumption. Use `/EHsc-` on the ACT I/O and
stage owner translation units, and on their exception-injection test caller.
This disables only `/EHc`, keeps synchronous C++ unwinding, does not enable
`/EHa`, and does not catch access violations or invent a successful load result.

Reference: Microsoft /EH documentation, arguments c and -:
https://learn.microsoft.com/en-us/cpp/build/reference/eh-exception-handling-model

The unwind assertion is retained unchanged. Verification must distinguish a
successful compile from an executed contract; the initial failing run remains
preserved. This is separate from the stage partial-document cleanup change.
