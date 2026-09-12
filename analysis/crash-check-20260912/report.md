# Crash diagnosis (no runtime changes)

Scope: inspect the user's reported instability using existing Windows events,
WER dumps, linker maps and original IDA evidence. No game was launched, source
behavior changed, or README updated for this diagnosis. Carry forward skill
instructions and E-triage/E-imports from ../actor-state-cpp-20260912/report.md.

## Confirmed Shutdown Recursion

Windows Application event 1000 records four recent process failures:

| Time (local 2026-09-12) | Build | PID | Final fault |
| --- | --- | --- | --- |
| 18:34:28 | actor-animation quiet | 0x9180 | c00000fd at RVA 30bf3 |
| 18:35:42 | actor-animation quiet | 0x6f04 | c00000fd at RVA 30c03 |
| 18:36:57 | actor-animation diagnostic | 0x281c | c00000fd at RVA 30c03 |
| 18:37:34 | actor-state quiet | 0x8994 | c00000fd at RVA 30c08 |

Multiple c0000409/c00001a5/c00000fd event records with the same PID/start time
belong to the same process failure and must not be counted as independent runs.
Linker maps resolve these offsets to function_429c70. Existing local WER dumps
for PIDs 28420, 10268 and 35220 contain respectively 64604, 64686 and 64689
consecutive frames returning to function_429c70+0x1d, all passing &g1224.
The first nonrecursive callers in all three dumps are:

WinMain -> function_40d940 -> virtual scene shutdown -> function_469680 ->
function_464e20 -> function_429c70.

function_40d940 is runtime shutdown, called after function_40db30 returns.
function_469680 shuts down map and Actor managers; function_464e20 destroys the
Actor animation tree. The generated function_429c70 writes the real right-child
pointer into a reconstructed stack temporary, then recursively calls itself
with the unrelated global &g1224 instead. The sentinel test therefore does not
follow the tree to a leaf, and recursion exhausts the stack. The generated
delete operation also receives &g1224 instead of the real node.

Skill start.ps1/open.ps1 opened original session edac2e85. IDA decompile of
429C70 confirms the intended tree deletion: while node+21 is not a sentinel,
recurse into node[2], save node[0] for the next iteration, and delete the actual
node. This confirms a reconstruction defect, not an original game rule.

The same faulty helper exists in the supplied unmodified decompiler output and
in actor-state builds predating the latest actor-animation module. This proves
this shutdown failure was not introduced by that latest module; it does not
prove the absence of all possible gameplay regressions.

## Validation Gap

The earlier actor-state quiet smoke used PID 38672 (0x9710). Windows events at
01:54:11..14 record the same shutdown fault for that exact process, despite
smoke-close.log reporting closed=true. exercise_game_window.py checks exit
status/responsiveness before posting WM_CLOSE, then only waits for termination;
it does not inspect the final exit code after the wait. The prior statement
that closure was normal was therefore insufficiently supported. Future close
validation must distinguish termination from successful exit.

## Separate Gameplay Access Violation

At 02:07:26 and 02:08:38 Windows records c0000005 for actor-state diagnostic at
RVA a2c69, inside retdec_squirrel_release (map range a2a80..a2d20). These are a
different failure from the shutdown recursion. Available recent CrashDumps
contain only the shutdown failures. Access to the older WER archive directories
was denied, so no register/object lifetime conclusion is established for those
access violations. They remain unresolved; the log tail alone is insufficient.

## Artifacts and Next Work

inspect_stack.py reads existing x86 minidump streams and collapses recursive
EBP frames using each build's linker map. The three *-stack.txt files record
its output. Source dumps remain untouched under
C:/Users/poker/AppData/Local/CrashDumps/; no original binary or dump was added to
the repository. No code fix, rebuild or repeated gameplay was attempted here.

Prioritize restoring the real node-based tree destruction and auditing its
callers' reconstructed stack writes, plus checking process exit codes after
closure. Then diagnose the independent Squirrel release fault from a matching
access-violation dump. Do not increase stack size or bypass cleanup to suppress
the symptom. Static original code plus three independent dump chains satisfy
ADF P0 for the shutdown finding; the gameplay release finding remains partial.
