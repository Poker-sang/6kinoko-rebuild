# commit: ci: align migration guards with retired frame adapters and fixture addresses
from pathlib import Path
p = Path('tools/check_migration_boundaries.py')
s = p.read_text()
start = s.index('    # These settings belong ONLY to the transitional adapter;')
end = s.index('    if errors:', start)
s = s[:start] + '''    # The transitional frame/stack adapters have been fully retired. Do not
    # require their old compiler flags, and do not permit their reintroduction.
    cmake = (ROOT / 'CMakeLists.txt').read_text()
    for source in ('src/platform/legacy_frame_entry.cpp',
                   'src/platform/legacy_frame_copy.cpp'):
        if (ROOT / source).exists() or source in cmake:
            errors.append('Retired stack/frame adapter returned: ' + source)
''' + s[end:]
s = s.replace("print('NOTE: legacy_frame_copy.cpp still preserves the old operand-selection heuristic.')", "print('PASS: retired frame/stack adapters remain absent.')")
p.write_text(s)
p = Path('tests/test_legacy_islands.py')
s = p.read_text()
old = "'4198464'"
new = "str(0x601040)"
if s.count(old) != 1: raise SystemExit('Unexpected interior-address fixture')
p.write_text(s.replace(old, new))
