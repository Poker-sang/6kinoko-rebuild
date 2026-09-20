"""Fail-closed tests for source provenance; no network or game files required."""
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from verify_upstream import verify


def digest(data):
    return hashlib.sha256(data).hexdigest()


class Provenance(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.library = self.root / 'third_party' / 'fixture'
        self.library.mkdir(parents=True)
        self.original = b'int source = 1;\n'
        (self.library / 'source.c').write_bytes(self.original)
        self.manifest = {'release': 'historical fixture', 'url': 'https://example.invalid/fixture',
                         'archive_sha256': 'a' * 64, 'files': {'source.c': digest(self.original)}}
        self.save()

    def save(self):
        (self.library / 'UPSTREAM.json').write_text(json.dumps(self.manifest))

    def valid(self):
        return not verify(self.root, ('fixture',))[0]

    def test_original_and_checkout_newlines(self):
        self.assertTrue(self.valid())
        (self.library / 'source.c').write_bytes(self.original.replace(b'\n', b'\r\n'))
        self.assertTrue(self.valid())

    def test_missing_source_or_manifest(self):
        (self.library / 'source.c').unlink()
        self.assertFalse(self.valid())
        (self.library / 'UPSTREAM.json').unlink()
        self.assertFalse(self.valid())

    def test_changed_source_requires_documented_patch(self):
        changed = self.original.replace(b'1', b'2')
        (self.library / 'source.c').write_bytes(changed)
        self.assertFalse(self.valid())
        patch = {'source.c': {'original_sha256': digest(self.original), 'adapted_sha256': digest(changed)}}
        (self.library / 'PATCHES.json').write_text(json.dumps(patch))
        self.assertFalse(self.valid())
        (self.library / 'PATCHES.md').write_text('Document the narrow change.')
        self.assertTrue(self.valid())
        patch['source.c']['original_sha256'] = 'b' * 64
        (self.library / 'PATCHES.json').write_text(json.dumps({'files': patch}))
        self.assertFalse(self.valid())

    def test_unmanifested_source(self):
        (self.library / 'extra.hpp').write_text('// new implementation')
        self.assertFalse(self.valid())

    def test_bad_digest_and_escape(self):
        self.manifest['files']['source.c'] = 'invalid'
        self.save()
        self.assertFalse(self.valid())
        self.manifest['files'] = {'../outside.c': digest(self.original)}
        self.save()
        self.assertFalse(self.valid())


if __name__ == '__main__':
    unittest.main()
