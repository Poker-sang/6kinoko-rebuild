# commit: test: complete ACT owner link and repair inherited test fixtures
from pathlib import Path
p = Path('tests/stage_owner_contract.cpp')
s = p.read_text()
old = 'int32_t kinoko_clear_render_queue() { return 0; }'
assert s.count(old) == 1
s = s.replace(old, 'void kinoko_initialize_render_queue() {}\n' + old)
p.write_text(s)
p = Path('tests/actor_records_contract.cpp')
s = p.read_text()
old = '    actor.set(&ActorRecord::direction, 1.0f);\n    kinoko_actor_set_take(actor_address, 38);'
new = '''    // The flipped take must resolve before its bounds can be recomputed.
    // Keep the genuinely missing take (999) below as a separate contract.
    kinoko_integer_map_put(lookup.get(&TreeIndex::head), 38, address(&animation));
    CHECK(kinoko_integer_map_find(lookup.get(&TreeIndex::head), 38) == address(&animation));
    actor.set(&ActorRecord::direction, 1.0f);
    kinoko_actor_set_take(actor_address, 38);'''
assert s.count(old) == 1
p.write_text(s.replace(old, new))
p = Path('tests/stage_contract.c')
s = p.read_text()
a = s.index('static int test_string_layout_binding(')
b = s.index('\nstatic int test_string_layout_lifetime(', a)
section = s[a:b]
assert 'throw \\"font clamp\\";' in section
# Squirrel 2.2.2 consumes the inner semicolon of an unbraced if statement.
# Preserve all assertions, separating top-level source lines for the outer
# OptionalSemicolon, rather than changing the production compiler.
section = section.replace(';"', ';' + chr(92) + 'n"')
p.write_text(s[:a] + section + s[b:])
p = Path('docs/act-document-loading-20260921/TEST-FIXTURES.md')
assert not p.exists()
p.write_text('''# Test fixture corrections (separate from production behavior)

- The new isolated stage-owner executable links stage_cleanup.cpp, which also
  defines the render-queue startup registration. Supply its unused initialization
  stub; the first batch-two build failed only at that unresolved test symbol.
- The pre-existing actor-record test registered animation 37 but selected 38 for
  a mirrored-bounds assertion. Register 38 explicitly. Keep the separate missing
  take 999 assertions unchanged; do not alter production lookup behavior.
- The pre-existing CStringLayout script concatenated unbraced if/throw statements
  on one source line. Squirrel 2.2.2 consumes the inner semicolon and the outer
  statement expects another separator. Give these source fragments real line
  endings. Preserve every property/method assertion and leave the compiler intact.

The earlier quiet run b16b9ce passed the new ACT document contract but failed the
actor-record and two stage-script tests. These are fixture fixes, not evidence of
regressions in original ACT behavior. Retain the failing build/test artifacts.
''')
