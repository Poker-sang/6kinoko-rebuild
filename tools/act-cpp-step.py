# commit: refactor: type ACT runtime ownership and replace offset-based clock access
# One-use transport of a reviewed unified diff, not runtime code.
import base64, hashlib, pathlib, subprocess, zlib
expected = {'.github/workflows/windows-x86.yml': '97b0463c20814149e62c45be296029cba93354605b41f4ab4a364e0150e3c87c', 'CMakeLists.txt': '0dcbd2da4692d6b071a3e61adfb88a932d436816801e993dc48104abe8f5ad38', 'docs/act-cpp-continuation-20260921/BATCH1.md': None, 'include/kinoko/act_resource.h': '6ec97c5a850d11b30c5f2ba4170524ca024f6fa62200fa583861b1809ff4d599', 'include/kinoko/act_resource_records.hpp': 'c61d760812d7d4f645547306bfd509cea0cdc1a5b7ed92fc7b7b5ca19e85dc57', 'src/decompiled/6kinoko_rebuilt.c': '842380ea6d6c9c049b2288f70645755b2199333c7bf156f076747b563fc01079', 'src/reconstructed/act_frame_render.cpp': '3fd70e1025d701cfe6fb01593b0387331097c88254e1e488078815d058b8014f', 'src/reconstructed/act_frame_update.cpp': '3e1e675d14ba31a0ad960f17303b149faf52148f66ce67d42eea0ebfdcf8c4a8', 'src/reconstructed/act_layer_access.cpp': 'ab365459886a93321d5135e24af43d782c4b54611d08dabd05fe48c3ec587d22', 'src/reconstructed/act_resource.cpp': 'bd313d9d78f0a89708e0a3e9a421c09815ba293724d4cfac47e967d359b957a4', 'src/reconstructed/act_runtime_lifecycle.cpp': 'c2badd1340f1a58eb00a9159a876fe504cf67bf85a1c4ac6fe9264d4a7041e04', 'src/reconstructed/map_copy.cpp': 'fad83c841a6a3e8d141bb03e555ef8491b77f967468bc47a969ef5bfbec84927', 'src/reconstructed/stage_cleanup.cpp': '2c678b7ed3a2cf395191aa474ecaf1c935524d5a6ff629847f48e4377ab4c04a', 'src/reconstructed/stage_runtime.cpp': '3feba46be2551437cf9635ea44b81af975e9831a934eb498b34c3d1ad500cc9c', 'src/squirrel/act_binding.cpp': '5bd302eb8d12594a28e70c715f9b3f4a616db98b6bf847f62ad9cfc848c7b603', 'tests/act_clock_contract.cpp': None, 'tests/stage_contract.c': 'c5412a24b259b4a8260ada9c3947d294c138b317b462a40633e8a9045ba892f7', 'tests/stage_owner_contract.cpp': '242de303b4198717a96bc4327de9a9984a09a6e7517a3d0428ace13263399284', 'tests/test_act_evidence.py': '0be9a172d844da1fb8f0cf1bf8420bfff79a304a501cdc99704171ae5bbfb9f3'}
for name, digest in expected.items():
    path = pathlib.Path(name)
    if digest is None:
        if path.exists(): raise SystemExit('New file already exists: ' + name)
    elif hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise SystemExit('Changed source; refusing patch: ' + name)
parts = [pathlib.Path('tools/act-cpp-patch.part1'), pathlib.Path('tools/act-cpp-patch.part2')]
encoded = ''.join(p.read_text().strip() for p in parts)
encoded = encoded.replace('X95c3p5ff7', 'X95c3p1ff7').replace('byXtTvSnntKZ', 'byXtTvntKZ')
patch = zlib.decompress(base64.b64decode(encoded))
assert hashlib.sha256(patch).hexdigest() == 'e4c355c4cc1785b209af89020f4ea3b068b1c1dc75aeedd862c5c75e19db5dea'
subprocess.run(['git', 'apply', '--check', '-'], input=patch, check=True)
subprocess.run(['git', 'apply', '-'], input=patch, check=True)
for part in parts: part.unlink()
