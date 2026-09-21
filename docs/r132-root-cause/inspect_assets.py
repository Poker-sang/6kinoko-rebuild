"""Offline ACT inspection; no game, VM, or regression test execution."""
import hashlib, json, struct
from pathlib import Path

class Reader:
    def __init__(self, raw):
        self.data, self.pos, self.schemas = raw, 0, {}
    def read(self, size):
        result = self.data[self.pos:self.pos+size]
        if len(result) != size: raise ValueError(('short', self.pos, size))
        self.pos += size
        return result
    def value(self, fmt): return struct.unpack('<'+fmt, self.read(struct.calcsize('<'+fmt)))[0]
    def string(self): return self.read(self.value('I')).decode('cp932', errors='replace')
    def properties(self, kind):
        if self.value('B'):
            self.schemas[kind] = [(self.string(), self.value('I')) for _ in range(self.value('I'))]
        return {n: self.string() if t == 3 else self.value({0:'i', 1:'f', 2:'B'}[t])
                for n, t in self.schemas[kind]}
    def script(self):
        fields = self.properties('script')
        offset = self.pos+4
        raw = self.read(self.value('I'))
        return dict(fields, size=len(raw), offset=offset, prefix_hex=raw[:16].hex(),
                    source=raw.decode('cp932', errors='replace') if not fields.get('compiled') else None)

def inspect(path, offset):
    encrypted = path.read_bytes()
    key = ((offset >> 1) | 0x23) & 255
    raw = bytes(v ^ key for v in encrypted)
    r = Reader(raw)
    if r.read(4) != b'ACT1' or r.value('I') != 1: raise ValueError('header')
    r.read(r.value('I'))
    result = dict(asset=path.name, archive_offset=offset, xor_key=key,
                  raw_sha256=hashlib.sha256(encrypted).hexdigest(), properties=r.properties('act'), script=r.script(), layers=[])
    for _ in range(r.value('I')):
        kind = r.value('I')
        layer = r.properties(kind)
        layouts = []
        for _ in range(r.value('I')):
            key_kind = r.value('I')
            r.properties(key_kind)
            if r.value('B'):
                layout_kind = r.value('I')
                fields = r.properties(layout_kind)
                layouts.append(dict(kind=hex(layout_kind), fields=fields))
                if layout_kind == 0xc9ca5c20:
                    count, size = r.value('I'), r.value('I')
                    r.read(count*size)
        for _ in range(r.value('I')):
            timeline_kind = r.value('I')
            r.properties(timeline_kind)
            count, size = r.value('I'), r.value('I')
            r.read(count*size)
        result['layers'].append(dict(fields=layer, layouts=layouts, script=r.script()))
    return result

root = Path(__file__).parent
results = [inspect(root/'assets'/name, offset) for name, offset in
           [('worldmap.act',58460), ('titlemenu.act',26687904), ('w1-c01a.act',4347064)]]
(root/'asset-scripts.json').write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')
for result in results:
    print(result['asset'], 'document', result['script'])
    for layer in result['layers']:
        print(layer['fields']['stName'], layer['script'])
