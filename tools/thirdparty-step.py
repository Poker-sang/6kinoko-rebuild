# commit: refactor: call the system CRT directly instead of 21 forwarding adapters
from pathlib import Path
import re, sys, hashlib
root = Path.cwd()
sys.path.insert(0, str(root / 'tools'))
from audit_unused_crt import definitions, LEX
before = {
 'src/decompiled/6kinoko_rebuilt.c': '1d66b9efbe8d35dbceb1a23a50bd4b6960ebd2bb8bfef44965e8e5642a1d6998',
 'src/decompiled/retdec_math_compat.h': '20e43972d78e80d86acf9a35d675a0495f7b4002e3389c446a0fb99834ff197e',
 'src/platform/retdec_runtime_compat.cpp': '5aaef7595a54fb5e89a4a91147d7499307f9724f896eb9c1d58fba427f03f742',
 'tests/stage_contract.c': '7abbad8f26dfa8055b9dfd61a1c3c036da8b3475bc30bed47399ed7370e1f9ed'
}
for name, digest in before.items():
 assert hashlib.sha256((root/name).read_bytes()).hexdigest() == digest, name
mapping = {'_malloc':'malloc','_realloc':'realloc','_free':'free','_memcpy':'memmove','_memset':'memset','_memchr':'memchr','_memcpy_s':'memcpy_s','_memmove_s':'memmove_s','_strcpy_s':'strcpy_s','_strncpy_s':'strncpy_s','_qsort':'qsort','_strtod':'strtod','_frexp':'frexp','_fabs':'fabs','_acos':'acos','_floor':'floor','_ceil':'ceil','_sprintf_s':'sprintf_s','llvm_log2_f80':'log2l','llvm_round_f80':'roundl','llvm_exp2_f80':'exp2l'}
target = root/'src/platform/retdec_runtime_compat.cpp'
text = target.read_text(); removed = 0
for entry in reversed(list(definitions(text))):
 if entry['name'] in mapping:
  text = text[:entry['start']] + text[entry['end']:]; removed += 1
assert removed == 21
target.write_text(re.sub(r'\n{4,}', '\n\n\n', text))
pattern = re.compile(r'\b('+'|'.join(map(re.escape,mapping))+r')\b')
for folder in ('src','include','tests'):
 for path in (root/folder).rglob('*'):
  if path.suffix not in ('.h','.hpp','.c','.cpp') or path.name in ('6kinoko.exe.c','retdec_math_compat.h'): continue
  text = path.read_text(); chunks = []; end = 0
  for match in LEX.finditer(text):
   chunks.append(pattern.sub(lambda m:mapping[m[0]], text[end:match.start()])); chunks.append(match[0]); end = match.end()
  chunks.append(pattern.sub(lambda m:mapping[m[0]], text[end:]))
  result = ''.join(chunks).replace('#include "retdec_math_compat.h"\n','')
  if path.name == '6kinoko_rebuilt.c': result = result.replace('double strtod(const char *text, char **end);\n','')
  if result != text: path.write_text(result)
(root/'src/decompiled/retdec_math_compat.h').unlink()
after = {
 'src/decompiled/6kinoko_rebuilt.c':'8e40b82c5298beed5ad6d072e81e5248a339c2206533f21a11b77f53cd3135cd',
 'src/platform/retdec_runtime_compat.cpp':'74de4f4144e222dacb60524a87c385ec167b961333dedde8b8ee09019273948c',
 'tests/stage_contract.c':'4f531c75b78831d9ccef104f6f761fb98017b28c152ba33145d9230758e400ba'
}
for name, digest in after.items():
 assert hashlib.sha256((root/name).read_bytes()).hexdigest() == digest, name
