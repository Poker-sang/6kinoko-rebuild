"""Keep call expressions out of the source-pruning definition/prototype scan."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location(
    'audit', Path(__file__).resolve().parents[1] / 'tools/audit_legacy_reachability.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class DefinitionScan(unittest.TestCase):
    def test_nested_call_is_not_a_definition(self):
        source = '''int32_t function_401000(void) {
    if (function_402000() == 0) { return 1; }
    return function_403000();
}
int32_t function_402000(void) { return 0; }
'''
        _, entries = audit.definitions(source)
        self.assertEqual([e['name'] for e in entries],
                         ['function_401000', 'function_402000'])
        self.assertLessEqual(entries[0]['end'], entries[1]['start'])

    def test_calls_and_initializers_are_not_prototypes(self):
        source = '''int32_t function_401000(void);
static int32_t *function_402000_this(int32_t receiver);
return function_403000();
    function_404000();
int32_t value = function_405000();
'''
        declarations = list(audit.PROTOTYPE.finditer(source))
        self.assertEqual(len(declarations), 2)

    def test_literals_comments_and_conditional_variants(self):
        source = '''// int32_t function_400001() { }
#if FIRST
int32_t function_401000() { puts("} function_400002()"); return 0; }
#else
int32_t function_401000() { /* } */ return 1; }
#endif
static int32_t retdec_unbound_function_402000() { return 0; }
'''
        _, entries = audit.definitions(source)
        self.assertEqual([e['name'] for e in entries],
                         ['function_401000', 'function_401000',
                          'retdec_unbound_function_402000'])


if __name__ == '__main__':
    unittest.main()
