"""Regression tests for conservative retired-library component discovery."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_legacy_islands import component, graph

SAMPLE = '''
int32_t function_401000(void);
int32_t function_402000(void);
struct table g10 = { function_401000 }; // 0x501000
struct table g11 = { function_402000 }; // 0x502000
int32_t function_401000(void) { return (int32_t)&g11; }
int32_t function_402000(void) { return (int32_t)&g10; }
'''


class GraphContract(unittest.TestCase):
    def selected(self, source=SAMPLE, external=None):
        entries, edges, roots, live = graph(source, external or {})
        return component(entries, edges, roots, live, ['function_401000'])

    def test_unrooted_function_table_cycle_is_closed(self):
        self.assertEqual(self.selected(), {'g10', 'g11', 'function_401000', 'function_402000'})

    def test_all_external_reference_forms_keep_cycle(self):
        for reference in ('function_401000()', '&function_402000', '&g10',
                          '0x501000', '5246976', '024020000',
                          '"function_401000"', '"0x501000"',
                          '#if UNKNOWN_OPTION\nvoid *p = &g11;\n#endif'):
            with self.subTest(reference=reference), self.assertRaisesRegex(ValueError, 'reachable'):
                self.selected(external={'outside.cpp': reference})

    def test_non_address_helper_is_a_root(self):
        with self.assertRaisesRegex(ValueError, 'reachable'):
            self.selected(SAMPLE + 'void helper(void) { int32_t g99 = (int32_t)&g10; }\n')

    def test_unparsed_initializer_is_a_root(self):
        with self.assertRaisesRegex(ValueError, 'reachable'):
            self.selected(SAMPLE + 'Table unknown_table = { &g10 };\n')

    def test_comments_are_not_roots(self):
        self.assertEqual(len(self.selected(external={'notes.h': '// g10\n/* function_401000() */'})), 4)

    def test_string_that_looks_like_a_prototype_is_still_rooted(self):
        with self.assertRaisesRegex(ValueError, 'reachable'):
            self.selected(SAMPLE + 'const char *lookup = "int32_t function_401000(void);";\n')

    def test_live_outgoing_dependency_does_not_keep_dead_callers(self):
        source = SAMPLE.replace('return (int32_t)&g11;', 'function_403000(); return (int32_t)&g11;')
        source += 'int32_t function_403000(void) { return 1; }\nvoid entry(void) { function_403000(); }\n'
        self.assertEqual(len(self.selected(source)), 4)

    def test_outside_dead_inbound_is_included_not_dropped(self):
        self.assertEqual(len(self.selected(SAMPLE + 'int32_t function_404000(void) { return (int32_t)&g10; }\n')), 5)

    def test_constant_original_address_edge(self):
        self.assertEqual(len(self.selected(SAMPLE.replace('(int32_t)&g11', '0x502000'))), 4)

    def test_duplicate_definition_and_bad_braces_fail_closed(self):
        for suffix in ('int32_t function_401000(void) { return 0; }', '{', '}'):
            with self.subTest(suffix=suffix), self.assertRaises(ValueError):
                self.selected(SAMPLE + suffix)


if __name__ == '__main__':
    unittest.main()
