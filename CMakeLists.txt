cmake_minimum_required(VERSION 3.24)

project(kinoko_rebuild LANGUAGES C CXX RC)

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
    add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/W4;/permissive-;/EHsc;/utf-8>")
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

add_library(kinoko_core STATIC
    src/reconstructed/archive.cpp
    src/reconstructed/asset_store.cpp
)
target_include_directories(kinoko_core PUBLIC include)

add_library(kinoko_actor_collision STATIC src/reconstructed/actor_collision.cpp src/platform/game_math.cpp src/platform/audio_math.cpp)
target_include_directories(kinoko_actor_collision PUBLIC include)
option(KINOKO_MATH_AUDIT "Record the first update-thread rounding change" OFF)
if(KINOKO_MATH_AUDIT)
    target_compile_definitions(kinoko_actor_collision PUBLIC KINOKO_MATH_AUDIT)
endif()


add_library(kinoko_native_methods STATIC
    src/reconstructed/act_resource.cpp
    src/reconstructed/act_clone.cpp
    src/reconstructed/actor_methods.cpp
    src/reconstructed/physical_input.cpp
    src/reconstructed/direct_input.cpp
    src/reconstructed/input_aggregation.cpp
    src/reconstructed/input_runtime.cpp
    src/reconstructed/actor_animation.cpp
    src/reconstructed/actor_cleanup.cpp
    src/reconstructed/stage_cleanup.cpp
    src/reconstructed/script_callbacks.cpp
    src/reconstructed/sprite.cpp
    src/reconstructed/sprite_rect.cpp
    src/reconstructed/sprite_color.cpp
    src/reconstructed/texture_store.cpp
    src/reconstructed/map_layers.cpp
    src/reconstructed/map_render.cpp)
target_include_directories(kinoko_native_methods PUBLIC include)

add_library(kinoko_audio_runtime STATIC src/reconstructed/audio_runtime.cpp)
set(KINOKO_STB_VORBIS_DIR "${CMAKE_SOURCE_DIR}/third_party/stb_vorbis-1.22")
target_include_directories(kinoko_audio_runtime PUBLIC include PRIVATE "${KINOKO_STB_VORBIS_DIR}")
target_compile_definitions(kinoko_audio_runtime PRIVATE _CRT_SECURE_NO_WARNINGS
    $<TARGET_PROPERTY:kinoko_retdec_rebuild,COMPILE_DEFINITIONS>)
target_compile_options(kinoko_audio_runtime PRIVATE /Gy)
target_link_libraries(kinoko_audio_runtime PUBLIC kinoko_actor_collision ole32 dxguid winmm user32)

add_library(kinoko_legacy_abi STATIC
    src/reconstructed/legacy_abi.cpp
    src/reconstructed/legacy_method_entries.cpp
    src/reconstructed/legacy_copy_entries.cpp)
target_include_directories(kinoko_legacy_abi PUBLIC include)

add_library(kinoko_retdec_support STATIC
    src/platform/retdec_memory.cpp
    src/platform/retdec_address_space.cpp
    src/platform/legacy_frame_copy.cpp
    src/platform/legacy_frame_entry.cpp
    src/platform/retdec_runtime_compat.cpp)
target_include_directories(kinoko_retdec_support PUBLIC include PRIVATE src/decompiled)
target_compile_definitions(kinoko_retdec_support PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(kinoko_retdec_support PRIVATE /Gy)
# Only this transitional frame adapter needs fixed frames, never the callers.
set_source_files_properties(src/platform/legacy_frame_entry.cpp
    PROPERTIES COMPILE_OPTIONS "/Oy-;/GL-")

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
    src/decompiled/retdec_asm_stubs.c
    src/platform/windows_entry.cpp
    src/platform/resources/windows_resources.rc
)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT kinoko_retdec_rebuild)
target_include_directories(kinoko_retdec_rebuild PRIVATE
    src/decompiled
    include
    src/platform/resources
)
target_compile_definitions(kinoko_retdec_rebuild PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
set(KINOKO_SQUIRREL2_ROOT
    "${CMAKE_SOURCE_DIR}/third_party/squirrel-2.2.2"
    CACHE PATH
    "Squirrel 2.2.2 source root for compilation and verified C++ runtime helpers")
# Generated C callers use the public API; only representation conversions
# remain in squirrel_api_types.h. Do not expose private VM layouts to C.
target_include_directories(kinoko_retdec_rebuild PRIVATE "${KINOKO_SQUIRREL2_ROOT}/include")
target_include_directories(kinoko_native_methods PRIVATE "${KINOKO_SQUIRREL2_ROOT}/include")
option(KINOKO_RETDEC_DISABLE_TRACE
    "Default to quiet output; KINOKO_TRACE=1 enables tracing without a rebuild"
    ON)
option(KINOKO_RETDEC_TRACE_FILTER
    "Keep only graphics/audio/input markers in RetDec diagnostic traces"
    OFF)
option(KINOKO_CAPTURE_FIRST_CHANCE
    "Default to first-fault context and full-memory dumps for diagnostic runs"
    OFF)
option(KINOKO_CAPTURE_SCRIPT_FAILURE "Capture the first VM script error before unwinding" OFF)
option(KINOKO_RETDEC_TRACE_ERRORS_ONLY "Only write VM failures and exception markers" OFF)
if(KINOKO_CAPTURE_SCRIPT_FAILURE)
    target_compile_definitions(kinoko_retdec_rebuild PRIVATE RETDEC_CAPTURE_SCRIPT_FAILURE)
endif()
if(KINOKO_RETDEC_TRACE_ERRORS_ONLY)
    target_compile_definitions(kinoko_retdec_rebuild PRIVATE RETDEC_TRACE_ERRORS_ONLY)
endif()
if(KINOKO_CAPTURE_FIRST_CHANCE)
    target_compile_definitions(kinoko_retdec_rebuild PRIVATE RETDEC_CAPTURE_FIRST_CHANCE)
endif()
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

# ACT compilation uses a separate VM; runtime compile APIs use the current VM.
# Runtime uses the supplied core; CallNative has one receiver-scope hook.
# All ABI and ownership adapters live in src/squirrel/.
add_library(kinoko_squirrel_cpp_vm STATIC
    src/reconstructed/act_frame_update.cpp
    src/reconstructed/act_frame_render.cpp
    src/reconstructed/act_draw_storage.cpp
    src/reconstructed/act_layer_access.cpp
    src/reconstructed/act_runtime_lifecycle.cpp
    src/reconstructed/act_lifetime.cpp
    src/reconstructed/act_document.cpp
    src/reconstructed/act_properties.cpp
    src/reconstructed/act_map.cpp
    src/reconstructed/act_containers.cpp
    src/reconstructed/act_layout.cpp
    src/squirrel/act_binding.cpp
    src/squirrel/squirrel_legacy_api.cpp
    src/squirrel/squirrel_host_compat.cpp
    src/squirrel/squirrel_vm_bootstrap.cpp
    src/squirrel/actor_lifecycle.cpp
    src/squirrel/squirrel_native_calls.cpp
    src/squirrel/squirrel_game_objects.cpp
    src/squirrel/squirrel_table_values.cpp
    src/squirrel/sqrat_object_bridge.cpp
    src/squirrel/native_property_bridge.cpp
    src/squirrel/squirrel_native_arguments.cpp
    src/squirrel/squirrel_class_binding.cpp
    src/squirrel/actor_registration.cpp
    src/squirrel/input_registration.cpp
    src/squirrel/global_registration.cpp
    src/squirrel/table_serialization.cpp
    src/squirrel/camera_map_binding.cpp
    src/squirrel/squirrel_native_variables.cpp
    src/squirrel/squirrel_method_dispatch.cpp
    src/reconstructed/squirrel_object.cpp
    src/reconstructed/native_instance.cpp
    src/squirrel/squirrel_source_runtime.cpp
    src/squirrel/squirrel_source_gc.cpp
    src/squirrel/squirrel_compile_bridge.cpp
    src/squirrel/squirrel_value_bridge.cpp
    src/squirrel/squirrel_operations.cpp
    src/squirrel/squirrel_thread.cpp
    src/squirrel/squirrel_csv.cpp
    src/squirrel/squirrel_generator_bridge.cpp
    src/squirrel/squirrel_array_bridge.cpp
    "${KINOKO_SQUIRREL2_ROOT}/sqstdlib/sqstdaux.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/sqstdlib/sqstdblob.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/sqstdlib/sqstdio.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/sqstdlib/sqstdmath.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/sqstdlib/sqstdrex.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/sqstdlib/sqstdstream.cpp"
    "${KINOKO_SQUIRREL2_ROOT}/sqstdlib/sqstdstring.cpp"
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
target_link_libraries(kinoko_retdec_rebuild PRIVATE kinoko_squirrel_cpp_vm)
target_compile_options(kinoko_retdec_rebuild PRIVATE
    $<$<COMPILE_LANGUAGE:C>:/TC>
    "$<$<COMPILE_LANGUAGE:C,CXX>:/Gy;/wd4100;/wd4244;/wd4267;/wd4706>")
# Match the original PE subsystem version and its non-client frame metrics.
target_link_options(kinoko_retdec_rebuild PRIVATE /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS,5.01)
if(KINOKO_RETDEC_MAP_FILE)
    target_link_options(kinoko_retdec_rebuild PRIVATE "/MAP:${KINOKO_RETDEC_MAP_FILE}")
endif()
set(KINOKO_D3DX9_33_LIB "${CMAKE_SOURCE_DIR}/third_party/d3dx9_33.lib")
target_link_libraries(kinoko_retdec_rebuild PRIVATE
    kinoko_audio_runtime
    kinoko_actor_collision
    kinoko_native_methods
    kinoko_legacy_abi
    kinoko_retdec_support
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
    tests/stage_audio_contract.cpp
    src/decompiled/retdec_asm_stubs.c
)
target_include_directories(kinoko_stage_contract PRIVATE src/decompiled
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
add_test(NAME squirrel_owned_state_exit_contract COMMAND kinoko_stage_contract --owned-state-exit)
foreach(entity IN ITEMS Init0989 Init0cfb Init0d0c)
    add_test(NAME entity_${entity}_contract
        COMMAND kinoko_stage_contract --entity-stutter "${KINOKO_REFERENCE_DIR}" ${entity})
endforeach()
add_test(NAME damage_pause_contract COMMAND kinoko_stage_contract --damage-pause)
add_test(NAME crystal_countdown_contract
    COMMAND kinoko_stage_contract --crystal-countdown "${KINOKO_REFERENCE_DIR}")
add_test(NAME moving_map_contract
    COMMAND kinoko_stage_contract --moving-map "${KINOKO_REFERENCE_DIR}")
add_test(NAME orange_platform_contract
    COMMAND kinoko_stage_contract --orange-platform "${KINOKO_REFERENCE_DIR}")
add_test(NAME green_stage5_contract
    COMMAND kinoko_stage_contract --green-stage5 "${KINOKO_REFERENCE_DIR}")
add_test(NAME water_alpha_contract
    COMMAND kinoko_stage_contract --water-alpha "${KINOKO_REFERENCE_DIR}")
add_test(NAME act_reentry_contract
    COMMAND kinoko_stage_contract --act-reentry "${KINOKO_REFERENCE_DIR}")

add_test(NAME moving_map_water_contract
    COMMAND kinoko_stage_contract --moving-map-water "${KINOKO_REFERENCE_DIR}")

foreach(position IN ITEMS "2200,559" "2400,559" "2600,559" "2770,463")
    string(REPLACE "," ";" coordinates "${position}")
    add_test(NAME moving_map_water_${position}
        COMMAND kinoko_stage_contract --moving-map-water "${KINOKO_REFERENCE_DIR}" ${coordinates})
endforeach()

add_executable(kinoko_game_math_contract tests/game_math_contract.cpp)
target_link_libraries(kinoko_game_math_contract PRIVATE kinoko_actor_collision)
target_compile_options(kinoko_game_math_contract PRIVATE /fp:strict)
add_test(NAME game_math_contract COMMAND kinoko_game_math_contract)

add_executable(kinoko_legacy_abi_contract tests/legacy_abi_contract.cpp)
target_link_libraries(kinoko_legacy_abi_contract PRIVATE kinoko_legacy_abi)
add_test(NAME legacy_abi_contract COMMAND kinoko_legacy_abi_contract)

add_executable(kinoko_texture_store_contract tests/texture_store_contract.cpp)
target_link_libraries(kinoko_texture_store_contract PRIVATE kinoko_native_methods user32)
add_test(NAME texture_store_contract COMMAND kinoko_texture_store_contract)

add_executable(kinoko_act_frame_contract
    tests/act_frame_contract.cpp
    src/reconstructed/act_layer_access.cpp
    src/reconstructed/act_draw_storage.cpp
    src/reconstructed/sprite_rect.cpp
    src/reconstructed/act_frame_render.cpp
    src/reconstructed/legacy_abi.cpp)
target_include_directories(kinoko_act_frame_contract PRIVATE include)
target_link_libraries(kinoko_act_frame_contract PRIVATE user32)
add_test(NAME act_frame_contract COMMAND kinoko_act_frame_contract)

add_executable(kinoko_diagnostics_capture tests/diagnostics_capture.cpp)
target_link_libraries(kinoko_diagnostics_capture PRIVATE kinoko_diagnostics dbghelp)
add_test(NAME diagnostics_capture COMMAND kinoko_diagnostics_capture)

add_executable(kinoko_squirrel_source_vm_contract tests/squirrel_source_vm_contract.cpp)
target_include_directories(kinoko_squirrel_source_vm_contract PRIVATE
    include "${KINOKO_SQUIRREL2_ROOT}/include")
target_link_libraries(kinoko_squirrel_source_vm_contract PRIVATE kinoko_squirrel_cpp_vm)
target_link_options(kinoko_squirrel_source_vm_contract PRIVATE /OPT:REF /OPT:ICF)
add_test(NAME squirrel_source_vm_contract COMMAND kinoko_squirrel_source_vm_contract)
set_tests_properties(squirrel_source_vm_contract PROPERTIES TIMEOUT 60)

add_executable(kinoko_native_record_view_contract tests/native_record_view_contract.cpp)
target_include_directories(kinoko_native_record_view_contract PRIVATE include)
add_test(NAME native_record_view_contract COMMAND kinoko_native_record_view_contract)

add_executable(kinoko_actor_records_contract tests/actor_records_contract.cpp)
target_include_directories(kinoko_actor_records_contract PRIVATE include)
target_link_libraries(kinoko_actor_records_contract PRIVATE kinoko_native_methods)
add_test(NAME actor_records_contract COMMAND kinoko_actor_records_contract)

add_executable(kinoko_com_owner_contract tests/com_owner_contract.cpp)
target_include_directories(kinoko_com_owner_contract PRIVATE include)
add_test(NAME com_owner_contract COMMAND kinoko_com_owner_contract)

add_executable(kinoko_audio_runtime_contract tests/audio_runtime_contract.cpp)
target_include_directories(kinoko_audio_runtime_contract PRIVATE include src/decompiled "${KINOKO_STB_VORBIS_DIR}")
target_compile_definitions(kinoko_audio_runtime_contract PRIVATE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(kinoko_audio_runtime_contract PRIVATE kinoko_actor_collision ole32 dxguid winmm user32)
add_test(NAME audio_runtime_contract COMMAND kinoko_audio_runtime_contract)
set_tests_properties(audio_runtime_contract PROPERTIES TIMEOUT 60)

add_executable(kinoko_squirrel_c_api_contract
    tests/squirrel_c_api_contract.c tests/squirrel_c_api_driver.cpp)
target_include_directories(kinoko_squirrel_c_api_contract PRIVATE include
    "${KINOKO_SQUIRREL2_ROOT}/include")
target_link_libraries(kinoko_squirrel_c_api_contract PRIVATE kinoko_squirrel_cpp_vm)
target_link_options(kinoko_squirrel_c_api_contract PRIVATE /OPT:REF /OPT:ICF)
add_test(NAME squirrel_c_api_contract COMMAND kinoko_squirrel_c_api_contract)
set_tests_properties(squirrel_c_api_contract PROPERTIES TIMEOUT 60)

add_executable(kinoko_legacy_method_entries_contract tests/legacy_method_entries_contract.cpp)
target_link_libraries(kinoko_legacy_method_entries_contract PRIVATE kinoko_legacy_abi)
add_test(NAME legacy_method_entries_contract COMMAND kinoko_legacy_method_entries_contract)

add_executable(kinoko_retdec_memory_contract tests/retdec_memory_contract.cpp)
target_link_libraries(kinoko_retdec_memory_contract PRIVATE kinoko_retdec_support)
add_test(NAME retdec_memory_contract COMMAND kinoko_retdec_memory_contract)

add_executable(kinoko_diagnostics_filter_contract tests/diagnostics_filter_contract.cpp)
target_include_directories(kinoko_diagnostics_filter_contract PRIVATE include)
add_test(NAME diagnostics_filter_contract COMMAND kinoko_diagnostics_filter_contract)

add_executable(kinoko_squirrel_refcount_contract tests/squirrel_refcount_contract.cpp)
target_include_directories(kinoko_squirrel_refcount_contract PRIVATE include
    "${KINOKO_SQUIRREL2_ROOT}/include" "${KINOKO_SQUIRREL2_ROOT}/squirrel")
target_link_libraries(kinoko_squirrel_refcount_contract PRIVATE kinoko_squirrel_cpp_vm kinoko_legacy_abi)
add_test(NAME squirrel_refcount_contract COMMAND kinoko_squirrel_refcount_contract)

add_executable(kinoko_squirrel_host_object_contract tests/squirrel_host_object_contract.cpp)
target_include_directories(kinoko_squirrel_host_object_contract PRIVATE include
    "${KINOKO_SQUIRREL2_ROOT}/include" "${KINOKO_SQUIRREL2_ROOT}/squirrel")
target_link_libraries(kinoko_squirrel_host_object_contract PRIVATE kinoko_squirrel_cpp_vm)
add_test(NAME squirrel_host_object_contract COMMAND kinoko_squirrel_host_object_contract)
set_tests_properties(squirrel_host_object_contract PROPERTIES TIMEOUT 60)

add_executable(kinoko_squirrel_pair_contract tests/squirrel_pair_contract.cpp)
target_include_directories(kinoko_squirrel_pair_contract PRIVATE include
    "${KINOKO_SQUIRREL2_ROOT}/include" "${KINOKO_SQUIRREL2_ROOT}/squirrel")
target_link_libraries(kinoko_squirrel_pair_contract PRIVATE kinoko_squirrel_cpp_vm)
target_link_options(kinoko_squirrel_pair_contract PRIVATE /OPT:REF /OPT:ICF)
add_test(NAME squirrel_pair_contract COMMAND kinoko_squirrel_pair_contract)
set_tests_properties(squirrel_pair_contract PROPERTIES TIMEOUT 60)

add_executable(kinoko_act_properties_contract tests/act_properties_contract.cpp)
target_include_directories(kinoko_act_properties_contract PRIVATE include
    "${KINOKO_SQUIRREL2_ROOT}/include" "${KINOKO_SQUIRREL2_ROOT}/squirrel")
target_sources(kinoko_act_properties_contract PRIVATE src/reconstructed/act_properties.cpp)
target_link_options(kinoko_act_properties_contract PRIVATE /OPT:REF /OPT:ICF)
add_test(NAME act_properties_contract COMMAND kinoko_act_properties_contract)
set_tests_properties(act_properties_contract PROPERTIES TIMEOUT 60)

foreach(contract IN ITEMS sqrat_object native_property)
    add_executable(kinoko_${contract}_contract tests/${contract}_contract.cpp)
    target_include_directories(kinoko_${contract}_contract PRIVATE include
        "${KINOKO_SQUIRREL2_ROOT}/include" "${KINOKO_SQUIRREL2_ROOT}/squirrel")
    target_link_libraries(kinoko_${contract}_contract PRIVATE kinoko_squirrel_cpp_vm)
    add_test(NAME ${contract}_contract COMMAND kinoko_${contract}_contract)
    set_tests_properties(${contract}_contract PROPERTIES TIMEOUT 60)
endforeach()
add_executable(kinoko_squirrel_binding_contract tests/squirrel_binding_contract.cpp)
target_include_directories(kinoko_squirrel_binding_contract PRIVATE include
    "${KINOKO_SQUIRREL2_ROOT}/include" "${KINOKO_SQUIRREL2_ROOT}/squirrel")
target_link_libraries(kinoko_squirrel_binding_contract PRIVATE kinoko_squirrel_cpp_vm kinoko_legacy_abi)
target_link_options(kinoko_squirrel_binding_contract PRIVATE /OPT:REF /OPT:ICF)
add_test(NAME squirrel_binding_contract COMMAND kinoko_squirrel_binding_contract)
set_tests_properties(squirrel_binding_contract PROPERTIES TIMEOUT 60)

foreach(contract IN ITEMS squirrel_native_calls squirrel_game_objects)
    add_executable(kinoko_${contract}_contract tests/${contract}_contract.cpp)
    target_include_directories(kinoko_${contract}_contract PRIVATE include
        "${KINOKO_SQUIRREL2_ROOT}/include" "${KINOKO_SQUIRREL2_ROOT}/squirrel")
    target_link_libraries(kinoko_${contract}_contract PRIVATE kinoko_squirrel_cpp_vm kinoko_legacy_abi)
    target_link_options(kinoko_${contract}_contract PRIVATE /OPT:REF /OPT:ICF)
    add_test(NAME ${contract}_contract COMMAND kinoko_${contract}_contract)
    set_tests_properties(${contract}_contract PROPERTIES TIMEOUT 60)
endforeach()

# /RTC1 checks caller/callee stack agreement without hand-written ESP probes.
add_executable(kinoko_actor_lifecycle_contract tests/actor_lifecycle_contract.cpp)
target_include_directories(kinoko_actor_lifecycle_contract PRIVATE include
    "${KINOKO_SQUIRREL2_ROOT}/include" "${KINOKO_SQUIRREL2_ROOT}/squirrel")
target_link_libraries(kinoko_actor_lifecycle_contract PRIVATE kinoko_squirrel_cpp_vm kinoko_legacy_abi)
target_compile_options(kinoko_actor_lifecycle_contract PRIVATE /Od /RTC1)
target_link_options(kinoko_actor_lifecycle_contract PRIVATE /OPT:REF /OPT:ICF)
add_test(NAME actor_lifecycle_contract COMMAND kinoko_actor_lifecycle_contract)
set_tests_properties(actor_lifecycle_contract PROPERTIES TIMEOUT 120)

add_executable(kinoko_legacy_copy_entries_contract tests/legacy_copy_entries_contract.cpp)
target_link_libraries(kinoko_legacy_copy_entries_contract PRIVATE kinoko_legacy_abi)
target_compile_options(kinoko_legacy_copy_entries_contract PRIVATE /Od /RTC1)
add_test(NAME legacy_copy_entries_contract COMMAND kinoko_legacy_copy_entries_contract)

# Test actual register forwarding separately from deterministic candidate selection.
# The entry contract supplies a scanner spy; the copy contract links the real one.
foreach(policy IN ITEMS checked optimized)
    foreach(boundary IN ITEMS copy entry)
        set(contract kinoko_legacy_frame_${boundary}_${policy}_contract)
        add_executable(${contract} tests/legacy_frame_${boundary}_contract.cpp)
        target_include_directories(${contract} PRIVATE include)
        if(boundary STREQUAL "entry")
            # The scanner spy must not pull in a library containing the real
            # scanner. Compile the actual production entry and validator here.
            target_sources(${contract} PRIVATE
                src/platform/legacy_frame_entry.cpp
                src/platform/retdec_address_space.cpp)
        else()
            target_link_libraries(${contract} PRIVATE kinoko_retdec_support)
        endif()
        if(policy STREQUAL "checked")
            target_compile_options(${contract} PRIVATE /Od /RTC1 /Oy-)
        else()
            target_compile_options(${contract} PRIVATE /O2 /Oy)
        endif()
        target_link_options(${contract} PRIVATE /OPT:REF /OPT:ICF)
        add_test(NAME legacy_frame_${boundary}_${policy}_contract COMMAND ${contract})
        set_tests_properties(legacy_frame_${boundary}_${policy}_contract PROPERTIES TIMEOUT 120)
    endforeach()
endforeach()

set(KINOKO_TOOL_TARGETS
    kinoko_act_frame_contract
    kinoko_native_record_view_contract
    kinoko_actor_records_contract
    kinoko_com_owner_contract
    kinoko_audio_runtime_contract
    kinoko_squirrel_c_api_contract
    kinoko_actor_lifecycle_contract kinoko_legacy_copy_entries_contract
    kinoko_legacy_frame_copy_checked_contract kinoko_legacy_frame_copy_optimized_contract
    kinoko_legacy_frame_entry_checked_contract kinoko_legacy_frame_entry_optimized_contract
    kinoko_archive_inspect kinoko_archive_smoke
    kinoko_asset_probe kinoko_dump_inspect kinoko_process_dump kinoko_stage_contract
    kinoko_legacy_abi_contract kinoko_game_math_contract
    kinoko_texture_store_contract
    kinoko_diagnostics_capture
    kinoko_squirrel_source_vm_contract
    kinoko_legacy_method_entries_contract kinoko_retdec_memory_contract
    kinoko_diagnostics_filter_contract kinoko_squirrel_refcount_contract
    kinoko_squirrel_host_object_contract
    kinoko_squirrel_pair_contract kinoko_act_properties_contract
    kinoko_sqrat_object_contract kinoko_native_property_contract
    kinoko_squirrel_binding_contract
    kinoko_squirrel_native_calls_contract kinoko_squirrel_game_objects_contract
)

option(KINOKO_BUILD_ORIGINAL_ORACLE "Build offline original-PE VM comparison tool" OFF)
if(KINOKO_BUILD_ORIGINAL_ORACLE)
    add_library(kinoko_original_vm_probe SHARED tools/original_vm_oracle.cpp)
    target_include_directories(kinoko_original_vm_probe PRIVATE include)
    target_link_libraries(kinoko_original_vm_probe PRIVATE kinoko_squirrel_cpp_vm kinoko_actor_collision)
    add_executable(kinoko_original_vm_oracle tools/original_vm_oracle_host.cpp)
    list(APPEND KINOKO_TOOL_TARGETS kinoko_original_vm_oracle kinoko_original_vm_probe)
endif()

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
