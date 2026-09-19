from pathlib import Path
p=Path('src/platform/retdec_runtime_compat.cpp'); s=p.read_text()
a=s.index('int retdec_valid_range('); b=s.index('\nint32_t *Direct3DCreate9', a)
body=s[a:b]
Path('src/platform/retdec_address_space.cpp').write_text('#include <windows.h>\n#include <cstdint>\n#include <cstddef>\n\n// Shared production address validation, independent of CRT emulation and the\n// unresolved legacy scanner. Keep its single-region/protection rules intact.\nextern "C" '+body)
p.write_text(s[:a]+'int retdec_valid_range(const void* address, size_t size, int writeable);\n'+s[b:])
p=Path('CMakeLists.txt'); s=p.read_text().replace('    src/platform/retdec_memory.cpp\n','    src/platform/retdec_memory.cpp\n    src/platform/retdec_address_space.cpp\n')
a='''        target_link_libraries(${contract} PRIVATE kinoko_retdec_support)
        if(policy STREQUAL "checked")'''
b='''        if(boundary STREQUAL "entry")
            # The scanner spy must not pull in a library containing the real
            # scanner. Compile the actual production entry and validator here.
            target_sources(${contract} PRIVATE
                src/platform/legacy_frame_entry.cpp
                src/platform/retdec_address_space.cpp)
        else()
            target_link_libraries(${contract} PRIVATE kinoko_retdec_support)
        endif()
        if(policy STREQUAL "checked")'''
assert a in s; p.write_text(s.replace(a,b))
Path(__file__).unlink()
