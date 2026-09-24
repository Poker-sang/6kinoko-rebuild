"""Regression source for inventory blind spots; not auto-executed by this PR."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('audit_readability', Path(__file__).resolve().parents[1]/'tools/audit_readability.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class InventoryContract(unittest.TestCase):
    def test_pointer_and_noexcept_signatures(self):
        source = '''KinokoActorPool *kinoko_actor_pool_construct(KinokoActorPool *p) { return p; }
KinokoActor* acquire() noexcept(false) { return nullptr; }
extern "C" int kinoko_method_x() { field<int32_t>(layer + 132) = 15; return 1; }
'''
        rows = audit.inspect_source('source.cpp', source)['entries']
        self.assertEqual([r['name'] for r in rows], ['kinoko_actor_pool_construct', 'acquire', 'kinoko_method_x'])
        self.assertEqual(rows[2]['category'], 'legacy_marked_candidate')
        self.assertGreater(rows[2]['markers']['literal_offset_access'], 0)

    def test_unparsed_and_comment_markers(self):
        source = '''// field<int>(v2 + 99); goto v3;
#define PEEK field<int32_t>(v1 + 132)
const char* note = "goto v3;";
'''
        report = audit.inspect_source('source.hpp', source)
        self.assertEqual(report['markers']['literal_offset_access'], 1)
        self.assertNotIn('goto', report['markers'])
        self.assertTrue(report['markers_outside_function_candidates'])

    def test_nested_body_is_not_double_counted(self):
        report = audit.inspect_source('source.cpp', 'void step() {\nif (ready()) { run(); }\n}\n')
        self.assertEqual([r['name'] for r in report['entries']], ['step'])


if __name__ == '__main__':
    unittest.main()
