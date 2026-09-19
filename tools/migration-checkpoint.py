from pathlib import Path
p = Path('tests/legacy_frame_copy_contract.cpp')
s = p.read_text()
s = s.replace('    check_selection();\n', '    check_selection();\n    check_register();\n')
s = s.replace('    require(actual == expected && canary', '    if (actual != expected) std::fprintf(stderr, "EBP expected=%08x actual=%08x\\n", expected, static_cast<unsigned>(actual));\n    require(actual == expected && canary')
s = s.replace('    require(triple[2] == 64 &&', '    if (static_cast<uintptr_t>(result) != address(output.data()) || input != output)\n        std::fprintf(stderr, "copy result=%08x to=%08x from=%08x triple=%08x first=%u expected=%u\\n",\n            static_cast<unsigned>(result), static_cast<unsigned>(address(output.data())),\n            static_cast<unsigned>(address(input.data())), static_cast<unsigned>(reinterpret_cast<uintptr_t>(&triple)),\n            output[0], input[0]);\n    require(triple[2] == 64 &&')
p.write_text(s)
Path(__file__).unlink()
