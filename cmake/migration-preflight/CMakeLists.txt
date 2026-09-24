cmake_minimum_required(VERSION 3.24)
project(kinoko_migration_preflight LANGUAGES CXX)

# Standalone, intentionally independent of the Win32/x86 game target.
# This validates compilation of extracted rules, NOT a portable game build.
get_filename_component(KINOKO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_library(kinoko_migration_compat INTERFACE)
target_include_directories(kinoko_migration_compat INTERFACE "${KINOKO_ROOT}/include")
target_compile_features(kinoko_migration_compat INTERFACE cxx_std_17)
add_executable(migration_compat_contract "${KINOKO_ROOT}/tests/migration_compat_contract.cpp")
target_link_libraries(migration_compat_contract PRIVATE kinoko_migration_compat)
set_target_properties(migration_compat_contract PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(migration_compat_contract PRIVATE /W4 /permissive- /EHsc /utf-8)
else()
    target_compile_options(migration_compat_contract PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Execution remains an explicit user choice, following AGENTS.md.
option(KINOKO_MIGRATION_REGISTER_TESTS "Register the opt-in resource-rules contract" OFF)
if(KINOKO_MIGRATION_REGISTER_TESTS)
    enable_testing()
    add_test(NAME migration_compat_contract COMMAND migration_compat_contract)
endif()
