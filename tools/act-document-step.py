# commit: refactor: distinguish ACT array storage ownership from borrowed spans
from pathlib import Path
p = Path('include/kinoko/act_document_records.hpp')
s = p.read_text()
old = 'template<class T> struct DocumentPointerSpan { T **begin, **end, **capacity; };'
new = '''// act_array.cpp publishes begin/end views and owns a native vector in word 3.
// The last word is NOT an end-of-capacity pointer from the original VC8 vector.
template<class T> struct DocumentPointerSpan {
    T **begin, **end;
    kinoko::ActArray *storage;
};'''
assert s.count(old) == 1
s = s.replace('#include "kinoko/act_types.h"', '#include "kinoko/act_types.h"\n#include "kinoko/act_array.hpp"')
s = s.replace(old, new)
s = s.replace('static_assert(sizeof(DocumentPointerSpan<LayerRecord>) == 12);', 'static_assert(sizeof(DocumentPointerSpan<LayerRecord>) == 12);\nstatic_assert(offsetof(DocumentPointerSpan<LayerRecord>, storage) == 8);')
p.write_text(s)
p = Path('tests/act_document_contract.cpp')
s = p.read_text()
assert s.count('layers.capacity') == 1 and s.count('resources.capacity') == 1
p.write_text(s.replace('layers.capacity', 'layers.storage').replace('resources.capacity', 'resources.storage'))
p = Path('tests/actor_records_contract.cpp')
s = p.read_text()
old = '    CHECK(kinoko_integer_map_find(lookup.get(&TreeIndex::head), 38) == address(&animation));'
new = '''    const auto flipped_slot = kinoko_integer_map_find(lookup.get(&TreeIndex::head), 38);
    CHECK(flipped_slot != static_cast<int32_t>(lookup.get(&TreeIndex::head)));
    CHECK(*pointer<int32_t>(flipped_slot) == address(&animation));'''
assert s.count(old) == 1
p.write_text(s.replace(old, new))
p = Path('docs/act-document-loading-20260921/ARRAY-OWNERSHIP.md')
assert not p.exists()
p.write_text('''# ACT array representation correction

The active act_array.cpp implementation stores an owned kinoko::ActArray pointer
at the third word of each published span. Its begin/end are borrowed views into
that vector; the parser advances end only for successfully owned records. The
new document schema must describe this current representation, not label the
third word as a historical end-of-capacity pointer. Change its type to
kinoko::ActArray* storage and retain the 12-byte/offset-8 assertions. Initialization
and runtime behavior are unchanged. The constructor tests still verify all three
words are null, in both checked and optimized builds.

The actor fixture's new lookup assertion also follows integer_map.cpp's actual
contract: find returns the address of the mapped integer slot, or the map sentinel
on failure, not the mapped integer itself. Dereference the validated slot; keep
the existing flipped bounds and missing-take assertions unchanged.
''')
