"""Source provenance, not a substitute for original DAT/gameplay comparison."""
import hashlib
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = ROOT / 'src/decompiled/6kinoko.exe.c'
CAPTURE = ROOT / 'docs/decompiler-cleanup-r126/original-stage-evidence.json'


def original_function(name):
    text = ORIGINAL.read_text(encoding='utf-8')
    # Match a definition, not a declaration or a RetDec class/type label.
    match = re.search(r'^int32_t ' + re.escape(name) + r'\([^\n]*\) \{', text, re.M)
    if match is None:
        raise AssertionError('Missing original definition: ' + name)
    end = text.index('\n}\n', match.end())
    return text[match.start():end + 2]


class ActEvidence(unittest.TestCase):
    def test_unmodified_reference_hashes(self):
        for path, expected in (
            (ORIGINAL, '504d899c97d12700aad88d88edbc072f95c600184b684c0bf15bad7471821014'),
            (CAPTURE, '0ec538d5c8d730e818a92894f39137c7ed925e3f3862723ba2205abc16965b7a'),
        ):
            self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(), expected)

    def test_original_document_defaults(self):
        original = original_function('function_427530')
        migrated = (ROOT / 'src/reconstructed/act_document_io.cpp').read_text()
        schema = (ROOT / 'include/kinoko/act_document_records.hpp').read_text()
        defaults = (
            ('resolution_ms', 4, 'int32_t', '16'),
            ('screen_width', 8, 'int32_t', '1280'),
            ('screen_height', 12, 'int32_t', '720'),
            ('margin_left', 72, 'int32_t', '128'),
            ('margin_top', 76, 'int32_t', '128'),
            ('margin_right', 80, 'int32_t', '128'),
            ('margin_bottom', 84, 'int32_t', '128'),
            ('offset_x', 88, 'float32_t', '0.0f'),
            ('offset_y', 92, 'float32_t', '0.0f'),
            ('visible', 96, 'char', '1'),
            ('resources_suspended', 204, 'char', '0'),
        )
        for name, offset, kind, value in defaults:
            with self.subTest(field=name):
                self.assertIn(f'*({kind} *)(result + {offset}) = {value};', original)
                self.assertIn(f'KINOKO_DOCUMENT_FIELD({name}, {offset});', schema)
                cpp_value = value if kind == 'float32_t' else ('uint8_t' if kind == 'char' else kind) + '{' + value + '}'
                self.assertIn(f'view.set(&DocumentRecord::{name}, {cpp_value});', migrated)
        self.assertIn('(int32_t *)"act"', original)
        self.assertIn('.assign("act", 3);', migrated)

    def test_zeroing_is_not_misclassified_as_original(self):
        original = original_function('function_427530')
        self.assertNotIn('memset', original)
        for offset in (40, 68, 220, 236):
            self.assertNotRegex(original, r'\*\([^\n]+\)\(result \+ ' + str(offset) + r'\)\s*=')
        migrated = (ROOT / 'src/reconstructed/act_document_io.cpp').read_text()
        self.assertNotIn('view.clear();', migrated)
        self.assertIn('padding/unknown words are untouched', migrated)

    def test_original_post_load_sequence(self):
        functions = json.loads(CAPTURE.read_text())['functions']
        instructions = {int(row['address'], 16): row['instruction']
                        for function in functions for row in function['assembly']}
        expected = [
            (0x466179, 'call    sub_428000'),
            (0x46617e, 'mov     ecx, [esi]'),
            (0x466180, 'mov     edx, [ecx]'),
            (0x466182, 'mov     eax, [edx+18h]'),
            (0x466185, 'push    offset Source'),
            (0x46618a, 'call    eax'),
        ]
        actual = [(address, instructions[address]) for address in sorted(instructions)
                  if 0x466179 <= address <= 0x46618a]
        self.assertEqual(actual, expected)
        self.assertIn('.e6 = function_4289c0', ORIGINAL.read_text(encoding='utf-8'))
        # This does not assert that the missing call has been restored.

    def test_normal_cleanup_order_not_failure_policy(self):
        functions = json.loads(CAPTURE.read_text())['functions']
        instructions = {int(row['address'], 16): row['instruction']
                        for function in functions for row in function['assembly']}
        self.assertIn('operator delete', instructions[0x465fb9])
        self.assertEqual(instructions[0x465fcc], 'mov     edx, [eax+10h]')
        self.assertEqual(instructions[0x465fd1], 'call    edx')
        self.assertEqual(instructions[0x465fd5], 'mov     edi, [esi+8]')
        self.assertEqual(instructions[0x465fe5], 'call    sub_450020')
        self.assertIn('operator delete', instructions[0x465feb])
        self.assertIn('operator delete', instructions[0x465ff4])

    def test_typed_runtime_holder_corresponds_to_original_store(self):
        original = original_function('function_44fde0')
        self.assertIn('*(int32_t *)result = a2;', original)
        source = (ROOT / 'src/reconstructed/act_source.cpp').read_text()
        self.assertIn('kinoko_act_runtime_initialize(storage.get(), holder)', source)
        self.assertNotIn('function_44fde0(', source)
        schema = (ROOT / 'include/kinoko/act_resource_records.hpp').read_text()
        self.assertIn('KinokoActSourceHolder *source_holder;', schema)
        self.assertIn('KINOKO_ACT_FIELD(RuntimeRecord, source_holder, 0);', schema)
        self.assertIn('sizeof(RuntimeRecord) == 192', schema)


    def test_runtime_pointer_and_clock_migration(self):
        schema = (ROOT / 'include/kinoko/act_resource_records.hpp').read_text()
        for declaration in ('KinokoActDocument *active_document;',
                            'KinokoActSourceHolder *active_holder;',
                            'SQVM *vm;', 'FindState *find_state;'):
            self.assertIn(declaration, schema)
        for name, offset in (('active_document', 12), ('active_holder', 16),
                             ('find_state', 84), ('vm', 152), ('name', 164)):
            self.assertIn(f'KINOKO_ACT_FIELD(RuntimeRecord, {name}, {offset});', schema)
        header = (ROOT / 'include/kinoko/act_resource.h').read_text()
        lifecycle = (ROOT / 'src/reconstructed/act_runtime_lifecycle.cpp').read_text()
        for old in ('function_44fde0(', 'function_450020(', 'retdec_destroy_act_runtime('):
            self.assertNotIn(old, header + lifecycle)
        self.assertIn('kinoko_act_runtime_dispose(KinokoActRuntime *', header)
        clock = (ROOT / 'src/reconstructed/act_resource.cpp').read_text()
        self.assertNotIn('enum Offset', clock)
        self.assertNotIn('address_ +', clock)
        self.assertIn('RecordView<RuntimeRecord>', clock)
        original = original_function('function_450d80')
        for offset in range(108, 152, 4):
            self.assertIn(f'*(int32_t *)(v1 + {offset}) = 0;', original)
        self.assertIn('*(int32_t *)(v1 + 100) = result;', original_function('function_4515a0'))


    def test_typed_layer_chain_uses_shared_document_schema(self):
        files = ('act_layer_access.cpp', 'act_frame_update.cpp', 'act_frame_render.cpp', 'act_source.cpp')
        source = ''.join((ROOT / 'src/reconstructed' / name).read_text(encoding='utf-8') for name in files)
        self.assertNotIn('DocumentLayers', source)
        for name in ('function_452020(', 'function_452040(', 'function_41efb0('):
            self.assertNotIn(name, source)
        self.assertIn('DocumentRecord::layers', source)
        original = original_function('function_455f50')
        self.assertIn('+ 184', original)
        self.assertIn('+ 180', original)
        self.assertIn('_3f__3f_2_40_YAPAXI_40_Z(4)', original)
        self.assertIn('*(int32_t *)(result + 4)', original_function('function_452020'))


if __name__ == '__main__':
    unittest.main()
