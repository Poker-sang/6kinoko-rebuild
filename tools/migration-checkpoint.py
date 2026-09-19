from pathlib import Path
p = Path('tests/stage_contract.c')
s = p.read_text()
old = '/* Exercise the actual thiscall ABI, not a cdecl call to the adapter signature.\n   The separate /RTC1 ABI contract detects an incorrect callee stack cleanup;\n   these guards also catch accidental writes to caller storage. */'
assert old in s
s = s.replace(old, '/* MSVC C does not expose __thiscall function-pointer syntax. Exercise the\n   typed ECX/EDX adapter here; actor_lifecycle_contract.cpp independently calls\n   the real thiscall ABI under /RTC1. These guards catch caller-storage writes. */')
s = s.replace('    typedef int32_t (__thiscall *SetStep)(int32_t, KinokoOwnedObjectWords);\n', '')
s = s.replace('    ((SetStep)function_4606d0)(actor, argument);', '    function_4606d0(actor, NULL, argument);')
p.write_text(s)
Path(__file__).unlink()
