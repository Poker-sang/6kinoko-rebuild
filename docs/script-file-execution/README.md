# Script File Execution

This batch recovers the standalone script-file chain, separate from ACT embedded
scripts and generic native bindings. Original executable SHA256:
`2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155`.
IDA MCP exports in this directory record the original implementation.

| Original | Recovered interface / behavior |
| --- | --- |
| 402AA0 / 402D30 | `kinoko_script_initialize_root` / `kinoko_script_root`; borrowed, typed root pointer |
| 402AC0 | `kinoko_script_close_vm`; original CALL followed by tail JMP means two wrapper releases |
| 402AF0 / 402B90 | `kinoko_script_show_call_stack`; script-maintained file/name/line arrays, original text and MessageBox title |
| 402D40 | `kinoko_script_load_file`; archive `.cv4` lookup, zero-terminated input, bytecode/text dispatch |
| 4A8EA0 / 4A8F90 | `upstream::sqplus_compile_and_run`; recovered SqPlus CompileBuffer/RunScript semantics |
| 471B30 | `kinoko_script_compile_file_argument`; x86 by-value Sqrat ABI, scoped SqPlus and incoming Sqrat ownership |
| 402A50 | `kinoko_script_read_memory`; typed 12-byte reader shared with ACT |

Bytecode uses original global 51620C (g664). It leaves the readclosure stack slot,
duplicates its borrowed closure, uses root for a missing or null environment,
and calls with one argument, no returned value, errors enabled. An opened bytecode
file returns true even after a VM load/call failure. Do not add blanket stack
restoration here. Existing ACT execution counts and stack policy are unchanged.

Text uses SqPlus current VM 5149DC (g644), strlen input and the original source
filename. It owns the compiled closure and return object, preserves an explicitly
supplied null environment, requests a result, and pops result plus closure on
success. Compilation/execution failures throw the historical SquirrelError;
execution failure first pops its closure, and the error constructor leaves the
VM's last-error object on the stack. There is no invented false-success fallback.
The host trace-bearing call bridge remains in use.

SqPlus temporary ownership uses the current VM; ownership of the incoming Sqrat
argument uses its recorded VM. C++ scope destruction covers exceptional exits.
VM error diagnostics now use sq_getlasterror/sq_getstring and restore their own
stack contribution instead of reading offsets 64/68/28.

ShowCallStack is **not** a VM error callback or VM stack inspector. It reads
`debug_call_stack_file`, `debug_call_stack_name`, `debug_call_stack_line` from root.
The final separator is emitted even for an empty name array.

References: supplied `../squirrel-2.2.2/SQUIRREL2/squirrel/sqapi.cpp`
(sq_compilebuffer, sq_call, reference operations), vendored matching runtime,
and `third_party/sqplus-20080713/sqplus/SquirrelVM.cpp` / SquirrelObject.cpp.

Reconstruction safeguards, not claimed as original behavior: null/malformed path
checks, allocation-overflow guard, reader bounds validation, null VM guard,
bounded tag read for one-byte files, and dynamically sized call-stack
rows with empty-string fallback for absent strings. File storage is released on
C++ exceptional exits as well as success.
The standalone script loader now uses the original virtual reader call and
continues after a short read, with the unread bytes initialized to zero.

Validation policy: compile/link only; no game, CTest or contract executable is
run by the agent. Added text-script contract cases cover root/custom/null
environments, call flags, stack balance, and compile/run exceptions. Existing
partial/clamped/EOF/negative memory-reader cases use the named pointer interface.
User runtime confirmation remains pending for this batch.
