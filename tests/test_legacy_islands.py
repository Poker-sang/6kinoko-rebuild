"""Regression tests for conservative retired-library component discovery."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from audit_legacy_islands import component, graph, interior_references
from unittest.mock import patch

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

    def test_cmake_modules_participate_in_pinned_source_audit(self):
        from audit_legacy_islands import audit, MAIN
        commit = 'a' * 40
        files = {MAIN: SAMPLE.encode(), 'cmake/Exports.cmake': b'function_401000'}
        def fake_git(*args):
            if args[0] == 'rev-parse': return commit.encode()
            if args[0] == 'ls-tree': return ('\n'.join(files) + '\n').encode()
            if args[0] == 'show': return files[args[1].split(':', 1)[1]]
            raise AssertionError(args)
        with patch('audit_legacy_islands.git', fake_git):
            with self.assertRaisesRegex(ValueError, 'reachable'):
                audit(commit, [], ['function_401000'])

    def test_comments_are_not_roots(self):
        self.assertEqual(len(self.selected(external={'notes.h': '// g10\n/* function_401000() */'})), 4)

    def test_unknown_address_records_are_retained_with_dependencies(self):
        source = SAMPLE.replace('return (int32_t)&g11;', 'g99=1; return (int32_t)&g11;')
        source += 'int32_t g99;\n'
        entities, edges, roots, live = graph(source, {})
        self.assertIn('g99', roots)
        self.assertEqual(component(entities, edges, roots, live, ['function_401000']),
                         {'g10', 'g11', 'function_401000', 'function_402000'})
        with self.assertRaisesRegex(ValueError, 'reachable'):
            component(entities, edges, roots, live, ['g99'])
        with self.assertRaisesRegex(ValueError, 'reachable'):
            self.selected(SAMPLE + 'int32_t g99 = (int32_t)&g10;\n')

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

    def test_named_crt_helpers_require_explicit_opt_in(self):
        text = SAMPLE + '\nint32_t __old_helper(void);\n// Address range: 0x403000 - 0x403100\nint32_t __old_helper(void) { return (int32_t)&g10; }\n'
        with self.assertRaisesRegex(ValueError, 'reachable'):
            self.selected(text)
        entities, edges, roots, live = graph(text, {}, include_named_functions=True)
        selected = component(entities, edges, roots, live, ['__old_helper'])
        self.assertEqual(len(selected), 5)

    def test_named_helper_address_and_external_prototype_are_roots(self):
        text = SAMPLE + '\nint32_t __old_helper(void);\n// Address range: 0x403000 - 0x403100\nint32_t __old_helper(void) { return (int32_t)&g10; }\n'
        for reference in ('0x403000', '&__old_helper', '"__old_helper"', 'int32_t __old_helper(void);'):
            with self.subTest(reference=reference), self.assertRaisesRegex(ValueError, 'reachable'):
                entities, edges, roots, live = graph(text, {'outside.h': reference}, include_named_functions=True)
                component(entities, edges, roots, live, ['__old_helper'])

    def test_merged_interior_entry_and_collapsed_data_are_not_discarded(self):
        source = '// Address range: 0x401000 - 0x401050\nint32_t function_401000(void) { return 0; }\n'
        source += 'int32_t g1 = 0; // 0x501000\n'
        entries, _, _, _ = graph(source, {})
        original = source + 'int32_t g2 = 0; // 0x501020\n'
        for literal in ('0x401040', '0x501010', '4198464', '\"0x401040\"'):
            with self.subTest(literal=literal):
                ranges, hits = interior_references(source, entries, {'external.c': literal}, original)
                self.assertEqual(len(hits), 1)
                self.assertEqual(ranges['g1'], [0x501000, 0x501020])
        _, hits = interior_references(source, entries, {'external.c': '0x401050; // 0x401040'}, original)
        self.assertEqual(hits, [])
        with self.assertRaisesRegex(ValueError, 'Unbounded'):
            interior_references(source, entries, {}, source)

    def test_original_image_callbacks_are_rejected(self):
        from check_migration_boundaries import CALLBACK_LITERAL, masked
        for code in ('sq_newclosure(vm, (SQFUNCTION)kinoko_pointer(0x01020304), 1);',
                     'sq_newclosure(kinoko_vm(vm),\n (SQFUNCTION)0x01020304, 0);',
                     'sq_newclosure(vm, (SQFUNCTION)kinoko_pointer(16909060), 1);'):
            with self.subTest(code=code):
                self.assertIsNotNone(CALLBACK_LITERAL.search(masked(code)))
        for code in ('sq_newclosure(vm, kinoko_sqrat_get_float, 1);',
                     'sq_newclosure(vm, (SQFUNCTION)function_callback, 1);',
                     'sq_newclosure(vm, (SQFUNCTION)kinoko_pointer(callback), 1);',
                     '// sq_newclosure(vm, (SQFUNCTION)0x01020304, 1);'):
            with self.subTest(code=code):
                self.assertIsNone(CALLBACK_LITERAL.search(masked(code)))

    def test_crt_size_and_locale_pointer_definitions(self):
        from audit_unused_crt import definitions
        source = 'size_t size_result(const char *s) { return 0; }\n'
        source += 'struct lconv *locale_result(void) { return localeconv(); }\n'
        source += 'size_t declaration(void);\nstruct lconv *global = 0;\n'
        self.assertEqual([entry['name'] for entry in definitions(source)],
                         ['size_result', 'locale_result'])

    def test_duplicate_definition_and_bad_braces_fail_closed(self):
        for suffix in ('int32_t function_401000(void) { return 0; }', '{', '}'):
            with self.subTest(suffix=suffix), self.assertRaises(ValueError):
                self.selected(SAMPLE + suffix)


if __name__ == '__main__':
    unittest.main()
