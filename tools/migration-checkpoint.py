from pathlib import Path
import subprocess

doc = Path("src/reconstructed/act_document.cpp")
text = doc.read_text()
start = text.index("void retdec_act_free_properties(")
end = text.index("int32_t retdec_act_load_script(", start)
block = text[start:end].rstrip() + "\n"

out = Path("src/reconstructed/act_properties.cpp")
assert not out.exists()
out.write_text("""// Property parsing and original byte-offset mappings for ACT records.
// Kept independent from loading/rendering so malformed-input contracts can
// exercise the real parser without linking unrelated game host services.
#include "kinoko/act_runtime.h"
#include "kinoko/act_host.h"
#include "kinoko/diagnostics.h"
#include "kinoko/legacy_memory.hpp"
#include <windows.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

using kinoko::legacy::pointer;
using kinoko::legacy::field;

""" + block)

doc.write_text(text[:start] + "// ACT property parsing/mapping lives in act_properties.cpp.\\n\\n" + text[end:])

cm = Path("CMakeLists.txt")
c = cm.read_text()
needle = "    src/reconstructed/act_document.cpp\\n"
assert c.count(needle) == 1
c = c.replace(needle, needle + "    src/reconstructed/act_properties.cpp\\n", 1)
old = """target_link_libraries(kinoko_act_properties_contract PRIVATE kinoko_squirrel_cpp_vm)
target_link_options(kinoko_act_properties_contract PRIVATE /OPT:REF /OPT:ICF)
"""
new = """target_sources(kinoko_act_properties_contract PRIVATE src/reconstructed/act_properties.cpp)
target_link_options(kinoko_act_properties_contract PRIVATE /OPT:REF /OPT:ICF)
"""
assert c.count(old) == 1
c = c.replace(old, new, 1)
cm.write_text(c)

subprocess.run(["python3", "tools/check_migration_boundaries.py"], check=True)
Path(__file__).unlink()
