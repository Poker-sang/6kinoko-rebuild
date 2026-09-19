"""Temporary transport of an exact, locally reviewed deletion set."""
import hashlib
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
p = root / 'src/decompiled/6kinoko_rebuilt.c'
text = p.read_text()
digest = lambda s: hashlib.sha256(s.encode()).hexdigest()
expected = '831315880b2385de2d84819564a76068497c540cafc1d7e7a5e9d3eadf1ee9f7'
if digest(text) == expected:
    raise SystemExit(0)
assert digest(text) == '2e5e72d351167b5852a2cd2ccee46ea58939c1f8f870c3f5ba94efdb8f970b84'
# Explicit set rather than expanding the selection after excluding the tools
# which call original-EXE addresses. All bodies remain in the baseline commit.
removed = set('''function_403ad0 function_4044d0 function_405800 function_4096d0 function_409840 function_4099c0 function_409da0 function_40a6c0 function_40a890 function_40b760 function_4101e0 function_411720 function_411930 function_411aa0 function_411b90 function_4120d0 function_412240 function_413f18 function_415810 function_416de0 function_416ec0 function_440180 function_450350 function_450f30 function_45e120 function_45e300 function_4689d0 function_469750 function_46ab10_legacy function_473dd8 function_473dde function_473ed0 function_473f30 function_474000 function_474020 function_474110 function_475710 function_475760 function_4757e0 function_475fc0 function_4760a0 function_476130 function_4762b0 function_4766b0 function_476a20 function_476eb0 function_476f40 function_477160 function_477190 function_477460 function_477480 function_477490 function_477950 function_479160 function_4792f0 function_479760 function_47aa44 function_47aa49 function_47aa54 function_47aa59 function_47aac4 function_47aac9 function_47aad4 function_47aad9 function_47acb4 function_47acb9 function_47acc4 function_47acd4 function_47afd0 function_47b020 function_47b140 function_47b290 function_47b350 function_47b3c0 function_47b410 function_47b510 function_47b630 function_47b670 function_47b890 function_47bb70 function_47bba0 function_47bc10 function_47bca0 function_47bfc0 function_47c5b0 function_47c8b0 function_47c930 function_47cc00 function_47cd00 function_47cdf0 function_47ce30 function_47d190 function_47d1a0 function_47d230 function_47d250 function_47d3f0 function_47d4f0 function_47d520 function_47d600 function_47d6e0 function_47d960 function_47d9b0 function_47daf0 function_47e0c0 function_47e130 function_47e160 function_47e7b0 function_47e810 function_47e840 function_47e990 function_47ea70 function_47eb50 function_47f0a0 function_47f820 function_47f850 function_47f880 function_47f960 function_47f9c0 function_47fab0 function_480080 function_480120 function_480200 function_4802d0 function_480410 function_480570 function_480600 function_480750 function_480820 function_4808b0 function_480a10 function_480a60 function_480b30 function_480d80 function_480da0 function_481190 function_481260 function_4812d0 function_4812e0 function_481330 function_4814b0 function_481520 function_481860 function_4818c0 function_481c90 function_481cb0 function_481ef0 function_481f30 function_482060 function_4820c0 function_4822c0 function_482650 function_4827d0 function_482a70 function_482af0 function_482b20 function_482cb6 function_482cbe function_482cd0 function_482e90 function_483300 function_483eb0 function_483ef0 function_483f10 function_4840e0 function_484300 function_484810 function_484d10 function_484da0 function_484ec0 function_485340 function_485350 function_485480 function_4854a0 function_4855f0 function_485610 function_4857e0 function_486160 function_486190 function_486500 function_486520 function_4865a0 function_486690 function_4866b0 function_4867f0 function_4869d0 function_486a30 function_486c50 function_486d70 function_486dd0 function_486f90 function_487200 function_487260 function_487490 function_4874f0 function_487530 function_4876d0 function_487830 function_4879d0 function_4879f0 function_487a10 function_487bd0 function_487e00 function_488020 function_488040 function_488500 function_488550 function_488740 function_488770 function_488980 function_488bb0 function_488be0 function_488c90 function_489280 function_489310 function_489600 function_489710 function_4897d0 function_4897f0 function_489850 function_489910 function_489940 function_489a80 function_489b00 function_489c60 function_489de0 function_489e30 function_489e80 function_489e90 function_4985b0 function_4a8d60_this function_4a9500 function_4a9a30 function_4d4040 function_4d4b30 retdec_read_esi retdec_unbound_function_42c300 retdec_unbound_function_450950 retdec_unbound_function_450e30 retdec_unbound_function_450f30'''.split())
lex = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
def mask(s):
    return lex.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), s)
masked = mask(text)
defs = []
for m in re.finditer(r'^[^\n;{}#]*?\b([A-Za-z_]\w*)\s*\([^;{}]*?\)\s*\{', masked, re.M):
    if m[1] in {'if', 'while', 'for', 'switch', '__except'}:
        continue
    start, br = m.start(), m.end()-1
    header = masked[start:br].rstrip()
    i, depth = len(header)-2, 1
    while depth:
        depth += (header[i] == ')') - (header[i] == '(')
        i -= 1
    am = re.search(r'([A-Za-z_]\w*)\s*$', header[:i+1])
    if not am or am[1] in {'if', 'while', 'for', 'switch', '__except'}:
        continue
    depth, end = 1, br+1
    while depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    if defs and start < defs[-1]['end']:
        continue
    defs.append(dict(name=am[1], start=start, end=end,
                     line=text.count('\n', 0, start)+1,
                     lines=text.count('\n', start, end)+1))
selected = [d for d in defs if d['name'] in removed]
assert len(removed) == 239 and len(selected) == 242
assert sum(d['lines'] for d in selected) == 41874
manifest = {
    'baseline_commit': '3cffc802f4c54ebb5ac1b9c5dfb5482e0be84547',
    'source': 'src/decompiled/6kinoko_rebuilt.c',
    'classification': 'disconnected definitions; deletion, NOT C++ migration',
    'root_policy': 'all external compilable-source references, all main-TU data initializers and literal numeric function addresses; main-TU prototypes and original-EXE-only oracle tools excluded',
    'definitions': [dict(name=d['name'], line=d['line'], body_lines=d['lines'],
                         sha256=digest(text[d['start']:d['end']])) for d in selected]
}
edits = [(d['start'], d['end'], '') for d in selected]
for m in re.finditer(r'^[^\n;{}#=]*?\b([A-Za-z_]\w*)\s*\([^;{}=]*?\)\s*;[^\S\n]*\n?', masked, re.M):
    if m[1] in removed and not any(d['start'] <= m.start() < d['end'] for d in defs):
        edits.append((m.start(), m.end(), ''))
for a, b, replacement in sorted(edits, reverse=True):
    text = text[:a]+replacement+text[b:]
text = re.sub(r'#if defined\(_MSC_VER\) && defined\(_M_IX86\)\s*#else\s*#endif', '', text)
assert digest(text) == expected, 'Output differs from reviewed local tree.'
p.write_text(text)
output = root / 'analysis/no-inline-asm-20260919'
output.mkdir(exist_ok=True)
(output / 'removed-definitions.json').write_text(json.dumps(manifest, indent=2)+'\n')
p = root / '.github/workflows/windows-x86.yml'
s = p.read_text()
s = s.replace('            -DKINOKO_RETDEC_TRACE_FILTER=ON `', '            -DKINOKO_RETDEC_TRACE_FILTER=ON `\n            -DKINOKO_RETDEC_MAP_FILE="${{ github.workspace }}/build-runs/ci-${{ matrix.trace }}/kinoko.map" `')
s = s.replace('            build-runs/ci-${{ matrix.trace }}/*.log', '            build-runs/ci-${{ matrix.trace }}/*.log\n            build-runs/ci-${{ matrix.trace }}/*.map')
p.write_text(s)
print('Published exact reviewed deletion: 242 definitions / 41874 body lines.')
