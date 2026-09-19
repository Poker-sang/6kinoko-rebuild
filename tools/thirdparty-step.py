# commit: test: include size and locale-pointer returns in the CRT audit
from pathlib import Path
import hashlib
p=Path('tools/audit_unused_crt.py');s=p.read_text()
assert s.count('uint\\d+_t|char)') == 1
s=s.replace('uint\\d+_t|char)', r'uint\d+_t|char|size_t|struct\s+lconv)');p.write_text(s)
p=Path('tests/test_legacy_islands.py');s=p.read_text()
assert hashlib.sha256(s.encode()).hexdigest() == '87793b025e1fbc977dacb7154c762cdf92b94b397ac255201bfdde47e104d70d'
i=s.index('    def test_duplicate_definition')
s=s[:i]+'''    def test_crt_size_and_locale_pointer_definitions(self):
        from audit_unused_crt import definitions
        source = 'size_t size_result(const char *s) { return 0; }\\n'
        source += 'struct lconv *locale_result(void) { return localeconv(); }\\n'
        source += 'size_t declaration(void);\\nstruct lconv *global = 0;\\n'
        self.assertEqual([entry['name'] for entry in definitions(source)],
                         ['size_result', 'locale_result'])

'''+s[i:]
p.write_text(s)
