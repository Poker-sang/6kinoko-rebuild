from pathlib import Path
p = Path('tests/legacy_frame_copy_contract.cpp')
s = p.read_text()
old = '}\n#pragma optimize("", on)\nvoid check_selection() {'
new = '''}
__declspec(noinline) void check_nested_entry(unsigned depth) {
    volatile uint32_t separation[1024]{};
    if (depth) check_nested_entry(depth - 1);
    else check_legacy_entry();
    require(separation[0] == 0 && separation[1023] == 0, "nested caller frame canaries");
}
#pragma optimize("", on)
void check_selection() {'''
assert s.count(old) == 1
s = s.replace(old, new).replace('    for (int i = 0; i < 100; ++i) check_legacy_entry();', '    for (int i = 0; i < 100; ++i) check_nested_entry(3);')
p.write_text(s)
Path(__file__).unlink()
