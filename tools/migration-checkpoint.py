from pathlib import Path
p = Path('include/kinoko/audio_host.h')
s = p.read_text()
assert 'extern int32_t g765;' in s and 'extern char g874;' not in s
p.write_text(s.replace('extern int32_t g765;', 'extern int32_t g765;\nextern char g874;'))
p = Path('tests/audio_runtime_contract.cpp')
s = p.read_text()
assert 'char* g877 = nullptr;' in s
p.write_text(s.replace('char* g877 = nullptr;', 'char* g877 = nullptr;\nchar g874 = 0;'))
Path(__file__).unlink()
