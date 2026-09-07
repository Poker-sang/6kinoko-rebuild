# Moving actors disappearing

Scope: restore the original Windows runtime behavior for moving mushrooms,
enemies and block-emitted stars, including confirmed defects encountered on
their shared execution paths. Original EXE/DATs are reference inputs. Gameplay
and diagnostic/no-log smoke runs are authorized by the current request.

## E-triage / E-imports

Read AGENTS.md, reverse-engineering and ida-reverse SKILL.md, precedent-reverse,
tool-index and re-agent-workflow. Skill start.ps1/open.ps1 opened the original
temporary copy as kinoko-original. The registered connector cannot see the
managed session; tools/ida_query.ps1 reaches the actual IDA MCP HTTP endpoint.
survey_binary confirms PE32, base 0x400000, 3963 functions and normal sections.
SHA256: 2db975a408e260499d52126f25ecf2fbc529cad52d1bffc2a0b7ca2ff695f155.
Readable imports include CreateFileA/W, ReadFile, WriteFile, GetFileSize,
SetFilePointer, D3D9/D3DX, USER32/GDI32, WinMM, IMM32 and COM.
SendMessageA is window messaging (misclassified as network by survey).
LoadLibraryW/GetProcAddress permit dynamic resolution. No packing obstacle.

## E-existing-runtime

Input: runtime-builds/star-landing-diag/retdec_trace.log, 855264383 bytes.
The log contains multiple runs. Targeted searches preserve the original file.
First invalid actor is frame 889, take 4010, after-motion: Y, free height and
Y bounds become FFC00000 while velocity and X remain finite. Later moving
stars also acquire NaN Y and are released. An earlier finite star is released
through item.nut CallbackStar at frame 902; this alone does not prove a bug.
The final EnemyUpdate_Ball failure is followed by an attempted native call
to literal original address 0x4C5BC0 in a relocated rebuilt process.

## Work Items

- [x] Read skills, project context, original survey and current runtime evidence.
- [x] Identify first invalid state using original assembly and runtime checks.
- [x] Restore confirmed native/VM discrepancies with focused regressions.
- [ ] Build and run diagnostic/no-log variants with DATs beside each EXE.
- [ ] Record outcome and commit the completed checkpoint.

## E-root-cause: missing floating-point return declarations

The user reproduced a red star, two fairies, a white kedama and a mushroom
disappearing in the instrumented game. collision-invalid records show normal
terrain and finite actor velocity, but NaN dy entering terrain resolution.
A read-only process dump showed the game worker's x87 stack fully occupied
with special values (tag AAAA, status 0041). Temporary FXSAVE probes localized
the first leak to MapManager::UpdateRenderLayer, original 46EDC0.

That routine calls _floor twice per frame. retdec_runtime_compat.c defines
double _floor(double), but the caller had no declaration and MSVC C assumed
int. The call therefore consumed EAX instead of ST(0) and left both floating
results on the x87 stack. Subsequent frames filled the stack; fminf's return
in Actor::Update then became NaN and contaminated dy/Y. Original IDA assembly
46EDE4/46EE01 calls _floor and 46EDE9/46EE09 explicitly pops each result.
The first finite star pickup in the old log was normal player contact, not
evidence for changing CallbackStar's release condition.

retdec_math_compat.h now declares the shared floating-return math helpers and
is included by both caller and implementation. _ceil, _fabs, _acos, _frexp
and the LLVM floating helpers receive the same ABI correction. No DAT,
movement, collision, item lifetime, respawn or pickup rule was changed.
All temporary per-native/per-frame FPU and collision probes were removed.

The camera regression failed before the change on frame 0: x87 tag 00 -> C0.
Afterward 600 calls preserve the tag and TOP and correctly floor -1.25 to -2
and 3.5 to 3. The initial experiment using only 24-bit precision passed the
old offline suite and was rejected as a sufficient explanation.

## E-error-path

The existing runtime log also captured an attempted call to literal original
address 4C5BC0 after EnemyUpdate_Ball reported a script error. Original
4C5C80 registers function addresses for the standard Squirrel error handlers.
Restore relocated callback addresses and the standard sqstdaux.cpp behavior,
including the variadic print adapter, call-stack information and locals.
GetLine/GetLocal now receive their explicit function-prototype receiver;
sq_getlocal preserves the frame stack-base walk and captured-variable push.
Tests exposed another confirmed discrepancy in 48A0A0: the two string format
arguments for a wrong-type error were missing. Restore them and the temporary
string's release so non-string thrown values do not crash the error handler.

References: supplied Squirrel 2.2.2 sqstdaux.cpp, sqapi.cpp, sqdebug.cpp,
sqobject.cpp and sqfuncproto.h, plus source-build SQVM::CallErrorHandler
assembly in ../evidence/raw/phase3-20260906/squirrel-sqvm.asm. IDA comparisons
include 4C5C80, 4C5BC0, 4A8C90, 48CFA0 and 48A0A0. Regression covers relocated
handlers, anonymous source/line information, outer/local values, primitive
errors, compiler diagnostics and unchanged VM stack depth.

## Verification Checkpoint

Builds: build-runs/moving-actors-diag and build-runs/moving-actors-notrace.
Runtime directories use the same names. Both Release builds pass CTest 2/2
and the extended original-resource first-stage contract, including enemy
walking/ledge falling, star landing/pickup, stone/block interaction and map
load/release/reload. Final contract output is in each build's final-contract.log.
stage_dat.ps1 copied and SHA256-verified all three DATs beside each game EXE.
run_staged.ps1 launched without a working-directory or data-directory override.
No-log still silences only the output/capture sinks.

The user played the corrected no-log build and confirmed that disappearance
is no longer observed. The live screenshot shows a red star, kedama and fairy
present in the first stage. The user then reported a crash while stomping an
enemy and requested this checkpoint be committed first. That crash is the
next investigation; it is not claimed fixed here. A final corrected diagnostic
startup/gameplay run remains pending this interruption (both instrumented
diagnostic predecessors were run through opening and first-stage gameplay).

The temporary debugger attach paused at system DLL TLS callbacks. Detaching
restored Responding=True. Subsequent game diagnosis used logs and a read-only
dump, without debugger attachment or breakpoint interference.

P0 synthesis: the disappearance root cause has original assembly evidence,
a before/after failing regression, runtime localization and user confirmation.
This checkpoint is not a claim of complete game fidelity. No IOC analysis is
applicable to this local game reconstruction.
