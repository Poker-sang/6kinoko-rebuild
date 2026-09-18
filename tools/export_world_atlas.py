"""Export original world-map backgrounds, without gameplay overlay layers."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from PIL import Image


class Reader:
    def __init__(self, data):
        self.data, self.pos = data, 0

    def read(self, size):
        data = self.data[self.pos:self.pos + size]
        assert len(data) == size
        self.pos += size
        return data

    def unpack(self, fmt):
        return struct.unpack('<' + fmt, self.read(struct.calcsize('<' + fmt)))

    def u32(self):
        return self.unpack('I')[0]

    def string(self):
        return self.read(self.u32()).decode('cp932')

    def properties(self):
        if not self.unpack('B')[0]:
            return {}
        fields = [(self.string(), self.u32()) for _ in range(self.u32())]
        return {name: self.string() if kind == 3 else self.unpack({0:'i', 1:'f', 2:'B'}[kind])[0]
                for name, kind in fields}

    def script(self):
        result = self.properties()
        result['raw'] = self.read(self.u32()).decode('cp932', errors='replace')
        return result


def read_act(data):
    r = Reader(data)
    assert r.read(4) == b'ACT1' and r.u32() == 1
    r.read(r.u32())
    result = {'properties': r.properties(), 'script': r.script(), 'layers': [], 'resources': []}
    for _ in range(r.u32()):
        assert r.u32() == 0x2618cf18
        layer = r.properties()
        layer['keys'] = []
        for _ in range(r.u32()):
            assert r.u32() == 0xd933304d
            key = r.properties()
            if r.unpack('B')[0]:
                kind = r.u32()
                key['layout'] = r.properties()
                if kind == 0xc9ca5c20:
                    count, size = r.unpack('II')
                    key['records'] = [r.read(size).hex() for _ in range(count)]
                else:
                    assert kind == 0x655cd5b0
            layer['keys'].append(key)
        assert r.u32() == 0
        layer['script'] = r.script()
        result['layers'].append(layer)
    for _ in range(r.u32()):
        kind = r.u32()
        result['resources'].append({'type': kind, **r.properties()})
    assert r.pos == len(data), (r.pos, len(data))
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--reference', type=Path, required=True)
    p.add_argument('--probe', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)

    def asset(name):
        dest = args.output / Path(name).name
        proc = subprocess.run([str(args.probe.resolve()), str(args.reference.resolve()), name, str(dest)],
                              check=True, stdout=subprocess.PIPE)
        offset = int(re.search(rb'offset=(\d+)', proc.stdout)[1])
        key = ((offset >> 1) | 0x23) & 255
        return bytes(b ^ key for b in dest.read_bytes())

    act = read_act(asset('data/worldmap/worldmap.act'))
    (args.output / 'worldmap.json').write_text(json.dumps(act, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(act, ensure_ascii=True, indent=2))


if __name__ == '__main__':
    main()
