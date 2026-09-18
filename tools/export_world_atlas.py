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


def read_mcd(data):
    r = Reader(data)
    assert r.read(4) == b'2DMC' and r.u32() == 1
    r.read(r.u32())
    count, size = r.unpack('II')
    chips = {}
    for _ in range(count):
        record = r.read(size)
        chip = struct.unpack_from('<IIhhhh', record)
        chips[chip[0]] = chip
        r.u32()
    textures = {}
    for _ in range(r.u32()):
        texture_id = r.u32()
        textures[texture_id] = r.string()
    assert r.pos == len(data)
    return chips, textures


def read_cv2(data):
    depth, width, height, stride, packed = struct.unpack_from('<BIIII', data)
    assert depth in (24, 32) and packed == 0
    assert len(data) == 17 + stride * height * 4
    return Image.frombytes('RGBA', (width, height), data[17:], 'raw', 'BGRA', stride * 4)


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
    chips, textures = read_mcd(asset('data/worldmap/worldmap.mcd'))
    layer, = [layer for layer in act['layers'] if layer['stName'] == 'bg_a']
    assert len(layer['keys']) == 1 and layer['script']['raw'].strip('\0') == ''
    records = layer['keys'][0]['records']
    assert len(records) == 11
    size = (act['properties']['screenWidth'], act['properties']['screenHeight'])
    atlas = Image.new('RGBA', size)
    manifest = {'size': size, 'layer': 'bg_a', 'maps': [],
                'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
                'animation': 'Static original base layer; animated overlays intentionally excluded.'}
    for record in records:
        chip_id, x, y = struct.unpack('<Iii', bytes.fromhex(record))
        _, texture_id, left, top, width, height = chips[chip_id]
        name = textures[texture_id].replace('\\', '/').lower() + '.cv2'
        data = asset(name)
        tile = read_cv2(data).crop((left, top, left + width, top + height))
        assert tile.size == (544, 384)
        assert 0 <= x <= size[0] - width and 0 <= y <= size[1] - height
        tile.save(args.output / (Path(name).stem + '.png'))
        atlas.alpha_composite(tile, (x, y))
        manifest['maps'].append({'asset': name, 'chip_id': chip_id, 'x': x, 'y': y,
                                 'width': width, 'height': height,
                                 'decoded_sha256': hashlib.sha256(data).hexdigest()})
    atlas.save(args.output / 'world-atlas.png')
    atlas.resize((size[0]*2, size[1]*2), Image.Resampling.NEAREST).save(args.output / 'world-atlas-2x.png')
    preview = Image.new('RGBA', size, '#20242b')
    preview.alpha_composite(atlas)
    preview.convert('RGB').save(args.output / 'world-atlas-preview.png')
    (args.output / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    main()
