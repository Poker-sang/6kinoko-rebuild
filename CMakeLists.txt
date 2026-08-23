cmake_minimum_required(VERSION 3.24)

project(kinoko_rebuild LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "The first runtime target is Windows x86.")
endif()

if(MSVC)
    add_compile_options(/W4 /permissive- /EHsc /utf-8)
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

add_library(kinoko_core STATIC
    src/reconstructed/archive.cpp
    src/reconstructed/asset_store.cpp
)
target_include_directories(kinoko_core PUBLIC include)

add_executable(kinoko_rebuild WIN32
    src/reconstructed/win_main.cpp
    src/reconstructed/gdi_renderer.cpp
)
target_include_directories(kinoko_rebuild PRIVATE include)
target_link_libraries(kinoko_rebuild PRIVATE kinoko_core user32 gdi32)

add_executable(kinoko_retdec_rebuild WIN32
    src/decompiled/6kinoko_rebuilt.c
    src/decompiled/retdec_runtime_compat.c
    src/decompiled/retdec_asm_stubs.c
    src/decompiled/retdec_entry.c
)
target_include_directories(kinoko_retdec_rebuild PRIVATE src/decompiled)
target_compile_definitions(kinoko_retdec_rebuild PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(kinoko_retdec_rebuild PRIVATE /TC /Gy /wd4100 /wd4244 /wd4267 /wd4706)
target_link_options(kinoko_retdec_rebuild PRIVATE /OPT:REF /OPT:ICF)
set(KINOKO_D3DX9_33_LIB "${CMAKE_SOURCE_DIR}/third_party/d3dx9_33.lib")
target_link_libraries(kinoko_retdec_rebuild PRIVATE
    user32
    gdi32
    winmm
    d3d9
    "${KINOKO_D3DX9_33_LIB}"
    dinput8
    dxguid
    imm32
    ole32
    dbghelp
)

add_executable(kinoko_archive_inspect
    tools/inspect_archive.cpp
)
target_include_directories(kinoko_archive_inspect PRIVATE include)
target_link_libraries(kinoko_archive_inspect PRIVATE kinoko_core)

add_executable(kinoko_asset_probe
    tools/probe_asset.cpp
)
target_include_directories(kinoko_asset_probe PRIVATE include)
target_link_libraries(kinoko_asset_probe PRIVATE kinoko_core)

add_executable(kinoko_dump_inspect
    tools/inspect_minidump.cpp
)
target_link_libraries(kinoko_dump_inspect PRIVATE dbghelp)

add_executable(kinoko_process_dump
    tools/capture_process_dump.cpp
)
target_link_libraries(kinoko_process_dump PRIVATE dbghelp)

add_executable(kinoko_archive_smoke
    tests/archive_smoke.cpp
)
target_include_directories(kinoko_archive_smoke PRIVATE include)
target_link_libraries(kinoko_archive_smoke PRIVATE kinoko_core)

set(KINOKO_REFERENCE_DIR "D:/6kinoko" CACHE PATH "Directory containing the original DAT files")
enable_testing()
add_test(
    NAME archive_smoke
    COMMAND kinoko_archive_smoke "${KINOKO_REFERENCE_DIR}"
)

foreach(config Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${config}" config_upper)
    set_target_properties(
        kinoko_rebuild kinoko_archive_inspect kinoko_archive_smoke
        kinoko_asset_probe kinoko_dump_inspect kinoko_process_dump kinoko_retdec_rebuild
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${CMAKE_SOURCE_DIR}/runtime"
    )
endforeach()
