"""Decode the retained x86 crash VM without running or modifying the game."""
import pathlib
import runpy
import struct
import sys

base = pathlib.Path(__file__).resolve().parents[1]
d = runpy.run_path(str(base / 'crash-check-20260912/inspect_stack.py'))
memory = d['memory']
def words(address, count):
    return struct.unpack('<' + 'I' * count, memory(address, count * 4))
def string(address):
    return memory(address + 28, words(address + 20, 1)[0]).decode('cp932', 'replace')

frame = d['unpack']('I', d['context_rva'] + 180)[0]
frames = []
for i in range(5):
    values = words(frame, 8)
    print('FRAME', i, hex(frame), [hex(v) for v in values])
    frames.append(values)
    frame = values[0]
vm = frames[2][2]
v = words(vm, 42)
print('VM', hex(vm), [(i * 4, hex(x)) for i, x in enumerate(v)])
for i in range(v[25]):
    ci = words(v[24] + i * 48, 12)
    print('CI', i, [hex(x) for x in ci])
    if ci[2] != 0x08000100:
        continue
    closure = words(ci[3], 12)
    print('CLOSURE', [hex(x) for x in closure])
    proto = closure[9]
    p = words(proto, 26)
    print('PROTO', [hex(x) for x in p])
    print('SCRIPT', string(p[4]), string(p[6]))
    for j in range(p[13]):
        pair = words(p[14] + 8*j, 2)
        print('LITERAL', j, string(pair[1]) if pair[0] == 0x08000010 else pair)
    print('INDEX', (ci[0] - 8 - (proto + 96)) // 8)
    print('OPS', memory(ci[0] - 32, 40).hex(' '))
print('STACK', [(i, [hex(x) for x in words(v[6] + i * 8, 2)]) for i in range(v[12] + 8)])
array = frames[0][3]
print('OLD ARRAY', hex(array), [hex(x) for x in words(array, 12)])
print('FAULT CODE', memory(d['eip'] - 32, 64).hex(' '))
