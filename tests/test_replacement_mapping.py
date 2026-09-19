"""Regression checks for source-definition evidence extraction."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location(
    'mapping_audit', Path(__file__).resolve().parents[1] / 'tools/audit_replacement_mapping.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class DefinitionTests(unittest.TestCase):
    def test_ternary_call_is_not_a_definition(self):
        self.assertIsNone(audit.definition('''
    if ((numeric
        ? function_48a830(vm, 2, &value) : function_48a7d0(vm, 2, &value)) >= 0) {
        field = value;
    }
''', 'function_48a830'))

    def test_real_definition_preserves_comment_and_literal_braces(self):
        source = '''extern "C" int32_t __fastcall function_48a830(
    int32_t receiver, void *unused) {
    // A } in a comment must not end the body.
    trace("{");
    return receiver;
}'''
        self.assertEqual(audit.definition(source, 'function_48a830'), source)

    def test_declaration_and_commented_definition_do_not_count(self):
        source = '''int32_t function_48a830(int32_t);
/* int32_t function_48a830(int32_t x) { return x; } */'''
        self.assertIsNone(audit.definition(source, 'function_48a830'))

    def test_link_match_requires_the_symbol_and_its_object_file(self):
        text = '''0001:1 ?construct@other@@ 00401000 f library:other.obj
0001:2 _construct_extra 00402000 f library:camera_map_binding.obj
0001:3 @kinoko_camera_update@8 00403000 f library:script_callbacks.obj'''
        self.assertFalse(audit.linked_symbol(text, 'construct', 'src/squirrel/camera_map_binding.cpp'))
        self.assertTrue(audit.linked_symbol(text, 'kinoko_camera_update', 'src/reconstructed/script_callbacks.cpp'))


if __name__ == '__main__':
    unittest.main()
