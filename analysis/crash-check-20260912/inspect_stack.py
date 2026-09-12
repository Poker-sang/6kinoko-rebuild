"""Read x86 WER dump frame chains and collapse identical recursive frames."""
import bisect
import collections
import pathlib
import re
import struct
import sys

data = pathlib.Path(sys.argv[1]).read_bytes()
assert data[:4] == b'MDMP'

def unpack(fmt, offset):
    return struct.unpack_from('<' + fmt, data, offset)

count, directory = unpack('II', 8)
streams = {}
for index in range(count):
    kind, size, rva = unpack('III', directory + 12 * index)
    streams[kind] = (rva, size)

ranges = []
if 9 in streams:
    rva, _ = streams[9]
    count, cursor = unpack('QQ', rva)
    for index in range(count):
        start, size = unpack('QQ', rva + 16 + 16 * index)
        ranges.append((start, start + size, cursor))
        cursor += size
else:
    rva, _ = streams[5]
    count, = unpack('I', rva)
    for index in range(count):
        start, size, cursor = unpack('QII', rva + 4 + 16 * index)
        ranges.append((start, start + size, cursor))
ranges.sort()
starts = [item[0] for item in ranges]

def memory(address, size):
    index = bisect.bisect_right(starts, address) - 1
    if index < 0:
        raise ValueError('Memory unavailable')
    start, end, cursor = ranges[index]
    if address + size > end:
        raise ValueError('Memory unavailable')
    return data[cursor + address - start:cursor + address - start + size]

rva, _ = streams[4]
module_count, = unpack('I', rva)
for index in range(module_count):
    base, size = unpack('QI', rva + 4 + 108 * index)
    name_rva, = unpack('I', rva + 4 + 108 * index + 20)
    name_size, = unpack('I', name_rva)
    name = data[name_rva + 4:name_rva + 4 + name_size].decode('utf-16-le')
    if name.lower().endswith('kinoko_retdec_rebuild.exe'):
        image_base, image_size = base, size
        print('module:', name, 'base:', hex(base))
        break

symbols = []
for line in pathlib.Path(sys.argv[2]).read_text(errors='replace').splitlines():
    match = re.match(r'\s+[0-9a-f]+:[0-9a-f]+\s+(\S+)\s+([0-9a-fA-F]{8})\s', line)
    if match:
        symbols.append((int(match[2], 16) - 0x400000 + image_base, match[1]))
symbols.sort()
addresses = [item[0] for item in symbols]

def symbol(address):
    if not image_base <= address < image_base + image_size:
        return hex(address)
    index = bisect.bisect_right(addresses, address) - 1
    return '%s+0x%x' % (symbols[index][1], address - addresses[index])

rva, _ = streams[6]
thread, = unpack('I', rva)
code, = unpack('I', rva + 8)
context_size, context_rva = unpack('II', rva + 160)
ebp, eip = unpack('II', context_rva + 180)
print('thread:', thread, 'exception:', hex(code), 'eip:', symbol(eip))
chain = []
seen = set()
while ebp not in seen and len(chain) < 100000:
    seen.add(ebp)
    try:
        previous, ret, arg = struct.unpack('<III', memory(ebp, 12))
    except ValueError:
        break
    chain.append((ret, arg))
    if previous <= ebp:
        break
    ebp = previous
print('frames:', len(chain))
groups = []
for ret, arg in chain:
    if groups and groups[-1][0] == ret:
        groups[-1][1] += 1
    else:
        groups.append([ret, 1, arg])
for ret, count, arg in groups:
    print('count=%d return=%s first_arg=%s' % (count, symbol(ret), symbol(arg)))
