"""Temporary checked transport step; removed after publishing this checkpoint."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from audit_legacy_islands import audit, MAIN, ROOT

SOURCE = '8a36b424810573a7d6812f88dab22211cb29cd54'

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--map', action='append', type=Path, required=True, dest='maps')
    args = p.parse_args()
    report = audit(SOURCE, args.maps, ['function_475790', 'function_4624a0'], include_named_functions=True)
    assert (report['functions'], report['data']) == (32, 9)
    file = ROOT / MAIN
    text = file.read_text()
    assert hashlib.sha256(text.encode()).hexdigest() == report['source_sha256']
    spans = []
    for e in report['candidates']:
        start, end = e['start'], e['end']
        if e['kind'] == 'function':
            start = text.rfind('// Address range:', 0, start)
            assert start >= 0
        else:
            end = text.index('\n', end) + 1
        spans.append((start, end))
    for start, end in sorted(spans, reverse=True):
        text = text[:start] + text[end:]
    for e in report['candidates']:
        if e['kind'] == 'function':
            pattern = r'^[^\n;{}=]*\b' + re.escape(e['name']) + r'\([^;\n]*\);[^\n]*\n'
            text = re.sub(pattern, '', text, flags=re.M)
    result_hash = hashlib.sha256(text.encode()).hexdigest()
    assert result_hash == 'e19e91608cc7ce58a67375177e99cdc563eec410eb237121d41c727daf904562'
    print('retired library source SHA256:', result_hash)
    file.write_text(text)
    report['result_source_sha256'] = result_hash
    report['interpretation'] = ('Closed retired zlib codec and STL tree components; '
                                'linker retention is not evidence of runtime invocation.')
    destination = ROOT / 'docs/upstream-library-audit/retired-zlib-stl.json'
    destination.parent.mkdir(exist_ok=True)
    destination.write_text(json.dumps(report, indent=2) + '\n')

if __name__ == '__main__':
    main()
