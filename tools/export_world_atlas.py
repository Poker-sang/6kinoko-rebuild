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
    # Preserve CP932 names when reading assets; Windows CLI argv conversion can
    # otherwise corrupt the Japanese names of the three building sheets.
    index = {}
    inspect = args.probe.resolve().with_name('kinoko_archive_inspect.exe')
    for suffix in 'abc':
        archive = args.reference / f'6kinoko_{suffix}.dat'
        listing = subprocess.check_output([str(inspect), '--all', str(archive)])
        for match in re.finditer(rb'^  (.+) offset=(\d+) size=(\d+)\r?$', listing, re.M):
            name, offset, length = match.groups()
            index[name.decode('cp932').lower()] = (archive, int(offset), int(length))
    cache = {}

    def asset(name):
        name = name.replace('\\', '/').lower()
        if name in cache:
            return cache[name]
        archive, offset, length = index[name]
        with archive.open('rb') as stream:
            stream.seek(offset)
            raw = stream.read(length)
        assert len(raw) == length
        key = ((offset >> 1) | 0x23) & 255
        cache[name] = bytes(b ^ key for b in raw)
        return cache[name]

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
                'animation': 'Original building animation: 90 game frames, 16 ms/frame; no cloud/road/stage overlays.'}
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
    # The red cavern is a separate terrain overlay, faded in by UpdateAreaEffect
    # while visiting world 5b. Its top/bottom transition images cover adjacent
    # worlds only during the camera transition. Keep each world's own terrain
    # in this simultaneous atlas and reveal only the central world-5b tile.
    cavern, = [l for l in act['layers'] if l['stName'] == 'w5_bg']
    for record in cavern['keys'][0]['records']:
        chip_id, x, y = struct.unpack('<Iii', bytes.fromhex(record))
        if chip_id != 1142:
            continue
        _, texture_id, left, top, width, height = chips[chip_id]
        tile = read_cv2(asset(textures[texture_id] + '.cv2')).crop((left, top, left+width, top+height))
        atlas.alpha_composite(tile, (x, y))
    atlas.save(args.output / 'world-atlas-terrain.png')
    symbols, = [l for l in act['layers'] if l['stName'] == 'symbol']
    buildings = []
    for record in symbols['keys'][0]['records']:
        chip_id, x, y = struct.unpack('<Iii', bytes.fromhex(record))
        chip = chips[chip_id]
        # These MCD texture IDs refer to 家 / 家02 / 家03, verified visually.
        if chip[1] in (19, 33, 34):
            buildings.append((chip, x, y))
    manifest['buildings'] = [{'chip_id': c[0], 'x': x, 'y': y} for c,x,y in buildings]
    manifest['cavern_layer'] = 'w5_bg chip 1142, alpha=1; adjacent-world transition overlays excluded'
    manifest['building_columns'] = [0, 1, 2, 1]
    manifest['excluded_building_column'] = '3: dark defeated/cleared variant'
    frames = []
    for column in (0, 1, 2, 1):
        frame = atlas.copy()
        for chip, x, y in buildings:
            chip_id, texture_id, left, top, width, height = chip
            sheet = read_cv2(asset(textures[texture_id] + '.cv2'))
            if chip_id != 1135:  # Static shadow of the floating building.
                left = column * 96
            tile = sheet.crop((left, top, left+width, top+height))
            if chip_id == 1135:
                tile.putalpha(tile.getchannel('A').point(lambda a: round(a * 0.8)))
            frame.alpha_composite(tile, (x, y))
        frames.append(frame)
    atlas = frames[0]
    atlas.save(args.output / 'world-atlas.png')
    atlas.resize((size[0]*2, size[1]*2), Image.Resampling.NEAREST).save(args.output / 'world-atlas-2x.png')
    preview = Image.new('RGBA', size, '#20242b')
    preview.alpha_composite(atlas)
    preview.convert('RGB').save(args.output / 'world-atlas-preview.png')
    # APNG keeps the original colors and alpha; GIF is a broadly compatible copy.
    frames[0].save(args.output / 'world-atlas-animated.png', save_all=True,
                   append_images=frames[1:], duration=[960, 160, 160, 160], loop=0, disposal=0, blend=0)
    rgb_frames = []
    for frame in frames:
        background = Image.new('RGBA', size, '#20242b')
        background.alpha_composite(frame)
        rgb_frames.append(background.convert('RGB'))
    palette = rgb_frames[0].quantize(colors=256)
    gif_frames = [f.quantize(palette=palette, dither=Image.Dither.NONE) for f in rgb_frames]
    gif_frames[0].save(args.output / 'world-atlas.gif', save_all=True,
                       append_images=gif_frames[1:], duration=[960,160,160,160], loop=0, disposal=1)
    (args.output / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    main()
