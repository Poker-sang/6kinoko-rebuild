# Exact historical sources; no configure-time network access or system-codec fallback.
get_filename_component(KINOKO_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(KINOKO_OGG_DIR "${KINOKO_SOURCE_ROOT}/third_party/libogg-1.1.3")
set(KINOKO_VORBIS_DIR "${KINOKO_SOURCE_ROOT}/third_party/libvorbis-1.2.0")
add_library(kinoko_ogg STATIC
    "${KINOKO_OGG_DIR}/src/bitwise.c" "${KINOKO_OGG_DIR}/src/framing.c")
target_include_directories(kinoko_ogg PUBLIC "${KINOKO_OGG_DIR}/include")
if(NOT WIN32)
    # Equivalent to upstream configure's fixed-width typedef selection. The
    # Windows build uses the original MSVC branch in ogg/os_types.h instead.
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ogg-config/ogg")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ogg-config/ogg/config_types.h"
        "#pragma once\n#include <stdint.h>\ntypedef int16_t ogg_int16_t;\ntypedef uint16_t ogg_uint16_t;\ntypedef int32_t ogg_int32_t;\ntypedef uint32_t ogg_uint32_t;\ntypedef int64_t ogg_int64_t;\n")
    target_include_directories(kinoko_ogg PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/ogg-config")
endif()
set(KINOKO_VORBIS_CORE mdct smallft block envelope window lsp lpc analysis
    synthesis psy info floor1 floor0 res0 mapping0 registry codebook sharedbook
    lookup bitrate)
set(KINOKO_VORBIS_SOURCES)
foreach(source IN LISTS KINOKO_VORBIS_CORE)
    list(APPEND KINOKO_VORBIS_SOURCES "${KINOKO_VORBIS_DIR}/lib/${source}.c")
endforeach()
add_library(kinoko_vorbis STATIC ${KINOKO_VORBIS_SOURCES})
target_include_directories(kinoko_vorbis PUBLIC "${KINOKO_VORBIS_DIR}/include"
    PRIVATE "${KINOKO_VORBIS_DIR}/lib")
target_link_libraries(kinoko_vorbis PUBLIC kinoko_ogg)
add_library(kinoko_vorbisfile STATIC "${KINOKO_VORBIS_DIR}/lib/vorbisfile.c")
target_include_directories(kinoko_vorbisfile PRIVATE "${KINOKO_VORBIS_DIR}/lib")
target_link_libraries(kinoko_vorbisfile PUBLIC kinoko_vorbis)
# Only contract tests use the encoder to generate non-proprietary fixtures.
add_library(kinoko_vorbisenc STATIC EXCLUDE_FROM_ALL "${KINOKO_VORBIS_DIR}/lib/vorbisenc.c")
target_include_directories(kinoko_vorbisenc PRIVATE "${KINOKO_VORBIS_DIR}/lib")
target_link_libraries(kinoko_vorbisenc PUBLIC kinoko_vorbis)
foreach(target kinoko_ogg kinoko_vorbis kinoko_vorbisfile kinoko_vorbisenc)
    if(MSVC)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_compile_options(${target} PRIVATE /Gy)
    else()
        target_link_libraries(${target} PUBLIC m)
    endif()
endforeach()
add_library(kinoko_vorbis_decoder STATIC "${KINOKO_SOURCE_ROOT}/src/reconstructed/vorbis_decoder.cpp")
target_include_directories(kinoko_vorbis_decoder PUBLIC "${KINOKO_SOURCE_ROOT}/include")
target_link_libraries(kinoko_vorbis_decoder PUBLIC kinoko_vorbisfile)
