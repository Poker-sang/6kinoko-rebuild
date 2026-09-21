# ACT array representation correction

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
