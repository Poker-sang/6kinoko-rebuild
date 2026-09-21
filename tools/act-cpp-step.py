# commit: refactor: unify ACT document layers and expose typed key lookup
# Read literal data from the pinned previous transport, never execute it.
import ast, base64, hashlib, pathlib, subprocess, zlib
old = subprocess.check_output(['git', 'show', '0b668642d78b54ac9843eb21ddd5e61b2f2bb945:tools/act-cpp-step.py']).decode()
tree = ast.parse(old)
expected = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'expected' for t in n.targets))
encoded = next(n.value for n in ast.walk(tree) if isinstance(n, ast.Constant) and isinstance(n.value, str) and n.value.startswith('eNrlfWt32ziS6Hf/'))
marker = 'FPTH8yBJsFhzp7Xyecn'
assert encoded.count(marker) == 1
encoded = encoded.replace(marker, '''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''')
patch = zlib.decompress(base64.b64decode(encoded))
assert hashlib.sha256(patch).hexdigest() == 'e980c48f470114e86363cae41683906b9eadd710f5552809c9c5016275be39ff'
for name, digest in expected.items():
    path = pathlib.Path(name)
    if digest is None:
        if path.exists(): raise SystemExit('New file already exists: ' + name)
    elif hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise SystemExit('Changed source; refusing patch: ' + name)
subprocess.run(['git', 'apply', '--check', '-'], input=patch, check=True)
subprocess.run(['git', 'apply', '-'], input=patch, check=True)
