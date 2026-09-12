cmake_minimum_required(VERSION 3.24)

project(kinoko_rebuild LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "The first runtime target is Windows x86.")
endif()

if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "The reconstructed runtime requires MSVC with -A Win32.")
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

add_library(kinoko_actor_collision STATIC src/reconstructed/actor_collision.cpp)
target_include_directories(kinoko_actor_collision PUBLIC include)

add_library(kinoko_native_methods STATIC
    src/reconstructed/act_resource.cpp
    src/reconstructed/actor_methods.cpp
    src/reconstructed/actor_animation.cpp
    src/reconstructed/actor_cleanup.cpp
    src/reconstructed/script_callbacks.cpp
    src/reconstructed/sprite.cpp)
target_include_directories(kinoko_native_methods PUBLIC include)

add_library(kinoko_legacy_abi STATIC src/reconstructed/legacy_abi.cpp)
target_include_directories(kinoko_legacy_abi PUBLIC include)

add_library(kinoko_diagnostics STATIC src/platform/diagnostics.cpp)
target_include_directories(kinoko_diagnostics PUBLIC include)
target_compile_definitions(kinoko_diagnostics PRIVATE _CRT_SECURE_NO_WARNINGS
    $<TARGET_PROPERTY:kinoko_retdec_rebuild,COMPILE_DEFINITIONS>)

# The original embeds zlib 1.2.3. Use that source for its damaged RetDec codec.
set(KINOKO_ZLIB_DIR "${CMAKE_SOURCE_DIR}/third_party/zlib-1.2.3")
add_library(kinoko_zlib STATIC
    ${KINOKO_ZLIB_DIR}/adler32.c
    ${KINOKO_ZLIB_DIR}/crc32.c
    ${KINOKO_ZLIB_DIR}/compress.c
    ${KINOKO_ZLIB_DIR}/deflate.c
    ${KINOKO_ZLIB_DIR}/trees.c
    ${KINOKO_ZLIB_DIR}/inflate.c
    ${KINOKO_ZLIB_DIR}/inftrees.c
    ${KINOKO_ZLIB_DIR}/inffast.c
    ${KINOKO_ZLIB_DIR}/zutil.c
)
target_include_directories(kinoko_zlib PUBLIC ${KINOKO_ZLIB_DIR})

add_executable(kinoko_retdec_rebuild WIN32
    src/decompiled/6kinoko_rebuilt.c
    src/decompiled/retdec_runtime_compat.c
    src/decompiled/retdec_asm_stubs.c
    src/platform/windows_entry.cpp
)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT kinoko_retdec_rebuild)
target_include_directories(kinoko_retdec_rebuild PRIVATE
    src/decompiled
    include
)
target_compile_definitions(kinoko_retdec_rebuild PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
option(KINOKO_ENABLE_SQUIRREL_CPP_VM
    "Use the C++ Squirrel 2.2.2 Execute backend from the RetDec runtime"
    OFF)
set(KINOKO_SQUIRREL2_ROOT
    "${CMAKE_SOURCE_DIR}/third_party/squirrel-2.2.2"
    CACHE PATH
    "Squirrel 2.2.2 source root for compilation and verified C++ runtime helpers")
option(KINOKO_RETDEC_DISABLE_TRACE
    "Default to quiet output; KINOKO_TRACE=1 enables tracing without a rebuild"
    ON)
option(KINOKO_RETDEC_TRACE_FILTER
    "Keep only graphics/audio/input markers in RetDec diagnostic traces"
    OFF)
option(KINOKO_RETDEC_DIAGNOSTIC_NO_BGM_SERVICE
    "Disable background BGM ring-buffer service for diagnosis"
    OFF)
option(KINOKO_RETDEC_DIAGNOSTIC_NO_BGM_WRITE
    "Skip BGM DirectSound writes for diagnosis"
    OFF)
set(KINOKO_RETDEC_MAP_FILE "" CACHE FILEPATH
    "Optional linker map file for the RetDec runtime target")
if(KINOKO_RETDEC_DISABLE_TRACE)
    target_compile_definitions(kinoko_retdec_rebuild PRIVATE
        RETDEC_DISABLE_TRACE RETDEC_DISABLE_RENDER_CAPTURE)
endif()
if(KINOKO_RETDEC_TRACE_FILTER)
    target_compile_definitions(kinoko_retdec_rebuild PRIVATE RETDEC_TRACE_FILTER)
endif()
if(KINOKO_RETDEC_DIAGNOSTIC_NO_BGM_SERVICE)
    target_compile_definitions(kinoko_retdec_rebuild PRIVATE
        RETDEC_DIAGNOSTIC_NO_BGM_SERVICE)
endif()
if(KINOKO_RETDEC_DIAGNOSTIC_NO_BGM_WRITE)
    target_compile_definitions(kinoko_retdec_rebuild PRIVATE
        RETDEC_DIAGNOSTIC_NO_BGM_WRITE)
endif()

if(NOT EXISTS "${KINOKO_SQUIRREL2_ROOT}/squirrel/sqvm.cpp")
    message(FATAL_ERROR
        "ACT source compilation requires Squirrel 2.2.2 at "
        "${KINOKO_SQUIRREL2_ROOT}")
endif()

# Inline ACT source is compiled in a separate VM; bytecode runs in the rebuild.
add_library(kinoko_squirrel_cpp_vm STATIC
    src/squirrel/squirrel_vm_bridge.cpp
    src/squirrel/squirrel_compile_bridge.cpp
    src/squirrel/squirrel_value_bridge.cpp
    "${KINOKO_SQUIRREL2_ROOT}/squirrel/sqapi.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/squirrel/sqbaselib.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/squirrel/sqclass.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/squirrel/sqcompiler.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/squirrel/sqdebug.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/squirrel/sqfuncstate.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/squirrel/sqlexer.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/squirrel/sqmem.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/squirrel/sqobject.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/squirrel/sqstate.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/squirrel/sqtable.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/squirrel/sqvm.cpp"
)
target_include_directories(kinoko_squirrel_cpp_vm PRIVATE
    "${KINOKO_SQUIRREL2_ROOT}/include"
    "${KINOKO_SQUIRREL2_ROOT}/squirrel"
    include
)
target_compile_definitions(kinoko_squirrel_cpp_vm PRIVATE
    WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS
)
if(MSVC)
    target_compile_options(kinoko_squirrel_cpp_vm PRIVATE
        /permissive /Gy /wd4100 /wd4244 /wd4267 /wd4706
    )
endif()
if(KINOKO_ENABLE_SQUIRREL_CPP_VM)
    target_compile_definitions(kinoko_retdec_rebuild PRIVATE
        KINOKO_ENABLE_SQUIRREL_CPP_VM=1
    )
endif()
target_link_libraries(kinoko_retdec_rebuild PRIVATE kinoko_squirrel_cpp_vm)
target_compile_options(kinoko_retdec_rebuild PRIVATE
    $<$<COMPILE_LANGUAGE:C>:/TC> /Gy /wd4100 /wd4244 /wd4267 /wd4706)
# Match the original PE subsystem version and its non-client frame metrics.
target_link_options(kinoko_retdec_rebuild PRIVATE /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS,5.01)
if(KINOKO_RETDEC_MAP_FILE)
    target_link_options(kinoko_retdec_rebuild PRIVATE "/MAP:${KINOKO_RETDEC_MAP_FILE}")
endif()
set(KINOKO_D3DX9_33_LIB "${CMAKE_SOURCE_DIR}/third_party/d3dx9_33.lib")
target_link_libraries(kinoko_retdec_rebuild PRIVATE
    kinoko_actor_collision
    kinoko_native_methods
    kinoko_legacy_abi
    kinoko_diagnostics
    kinoko_zlib
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

set(KINOKO_REFERENCE_DIR "${CMAKE_SOURCE_DIR}/../6kinoko" CACHE PATH "Directory containing the original DAT files")
set(KINOKO_RUNTIME_DIR "${CMAKE_SOURCE_DIR}/runtime-builds/default" CACHE PATH
    "Directory for built runtime executables")
enable_testing()
add_test(
    NAME archive_smoke
    COMMAND kinoko_archive_smoke "${KINOKO_REFERENCE_DIR}"
)

add_executable(kinoko_stage_contract
    tests/stage_contract.c
    src/decompiled/retdec_runtime_compat.c
    src/decompiled/retdec_asm_stubs.c
)
target_include_directories(kinoko_stage_contract PRIVATE
    $<TARGET_PROPERTY:kinoko_retdec_rebuild,INCLUDE_DIRECTORIES>)
target_compile_definitions(kinoko_stage_contract PRIVATE
    RETDEC_DISABLE_TRACE RETDEC_DISABLE_RENDER_CAPTURE)
target_compile_options(kinoko_stage_contract PRIVATE
    $<TARGET_PROPERTY:kinoko_retdec_rebuild,COMPILE_OPTIONS>)
target_link_options(kinoko_stage_contract PRIVATE /OPT:REF /OPT:ICF
    "/MAP:${CMAKE_CURRENT_BINARY_DIR}/stage-contract.map")
target_link_libraries(kinoko_stage_contract PRIVATE
    $<TARGET_PROPERTY:kinoko_retdec_rebuild,LINK_LIBRARIES>)
add_test(NAME stage_native_contract COMMAND kinoko_stage_contract)

add_executable(kinoko_legacy_abi_contract tests/legacy_abi_contract.cpp)
target_link_libraries(kinoko_legacy_abi_contract PRIVATE kinoko_legacy_abi)
add_test(NAME legacy_abi_contract COMMAND kinoko_legacy_abi_contract)

set(KINOKO_TOOL_TARGETS
    kinoko_archive_inspect kinoko_archive_smoke
    kinoko_asset_probe kinoko_dump_inspect kinoko_process_dump kinoko_stage_contract
    kinoko_legacy_abi_contract
)

foreach(config Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${config}" config_upper)
    set_target_properties(
        kinoko_retdec_rebuild
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${KINOKO_RUNTIME_DIR}"
    )
    set_target_properties(
        ${KINOKO_TOOL_TARGETS}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${KINOKO_RUNTIME_DIR}/tools"
        PDB_OUTPUT_DIRECTORY_${config_upper} "${KINOKO_RUNTIME_DIR}/tools"
    )
endforeach()
