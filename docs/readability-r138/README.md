# Readability estimate at R137 / before R138

Snapshot: `3b459eb7b8901d7f1a231e670ad61be8737ff719` (R137 accepted by user).
`inventory.json` records each detected function; `tools/audit_readability.py`
can regenerate the inventory for the current checkout.

| Approximate category | Functions | Share | Function-body line share |
|---|---:|---:|---:|
| Address-named legacy implementations | 443 | 26.6% | 30.9% |
| Named but mixed with decompiler conventions | 240 | 14.4% | 29.0% |
| Thin bridges to named implementations | 67 | 4.0% | 1.0% |
| Named structured implementation candidates | 914 | 54.9% | 39.1% |
| Total | 1,664 | 100% | 100% |

Scope: 110 unique first-party C/C++ source paths literally listed in top-level
CMake, counted once even if used in multiple targets. Excludes original archival
decompilation, external Squirrel/Xiph/vendor implementations, headers, tests and
tools. Conditional source may be counted even if a particular build excludes it.
This is source inventory, not a linked-symbol or original-function coverage count.

Lexical heuristic: short branch-free kinoko-call adapters are bridges; remaining
function_ADDRESS definitions are legacy; named bodies with decompiler temporary/
global names, address-function calls, goto or explicit integer-offset access (and
named bodies still in decompiled files) are mixed; the remainder are candidates.
Macros, constructors, complicated signatures, inline/header definitions and
semantic problems are not exhaustively recognized. A function and its wrapper
are distinct source definitions. Neither a descriptive name nor C++ proves that
fields, lifetime and original behavior are understood. Platform utilities also
contribute to the candidate count. Categories must not be used as certification.

Conservative engineering estimate of fully readable progress: **40–50%**.
Candidate bodies account for only 39.1% of counted function-body lines; some mixed
bodies are substantially understood, while some candidates still hide layout or
ownership details. This range is a judgment, not a measured completion score or
an estimate of remaining hours. Gameplay restoration is a different dimension.
The old 600-entry replacement mapping measures address disposition, including
retired Squirrel/runtime code, and must not be described as 100% readable game code.

Next priority remains evidence-backed schemas/ownership and actual logic, rather
than mass renaming. R138 continues map collision queries from the R137 baseline.
