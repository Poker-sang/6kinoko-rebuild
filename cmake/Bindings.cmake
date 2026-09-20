# Historical source implementations; the host owns the only Squirrel VM.
# KINOKO_SQUIRREL2_ROOT is supplied by the main or isolated contract build.
set(KINOKO_BINDING_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(KINOKO_SQPLUS_DIR "${KINOKO_BINDING_ROOT}/third_party/sqplus-20080713/sqplus")
add_library(kinoko_upstream_bindings STATIC
    "${KINOKO_BINDING_ROOT}/src/squirrel/upstream_sqplus.cpp"
    "${KINOKO_BINDING_ROOT}/src/squirrel/upstream_sqplus_scalars.cpp"
    "${KINOKO_BINDING_ROOT}/src/squirrel/upstream_sqrat.cpp"
    "${KINOKO_SQPLUS_DIR}/SquirrelObject.cpp"
    "${KINOKO_SQPLUS_DIR}/SquirrelVM.cpp"
    "${KINOKO_SQPLUS_DIR}/SqPlus.cpp")
target_include_directories(kinoko_upstream_bindings PUBLIC "${KINOKO_BINDING_ROOT}/include"
    "${KINOKO_SQUIRREL2_ROOT}/include" PRIVATE
    "${KINOKO_SQUIRREL2_ROOT}/squirrel" "${KINOKO_SQPLUS_DIR}"
    "${KINOKO_BINDING_ROOT}/third_party/sqrat-0.8.1/include")
# The host keeps its existing descriptors and VM bootstrap. Exclude only
# standalone snapshot registration helpers, not their unresolved-symbol stubs.
# MSVC diagnoses references even in otherwise-discardable COMDAT sections.
target_compile_definitions(kinoko_upstream_bindings PRIVATE SQPLUS_HOST_OBJECT_ONLY)
if(MSVC)
    target_compile_options(kinoko_upstream_bindings PRIVATE /Gy)
    target_compile_definitions(kinoko_upstream_bindings PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_options(kinoko_upstream_bindings INTERFACE /OPT:REF)
else()
    target_compile_options(kinoko_upstream_bindings PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(kinoko_upstream_bindings INTERFACE -Wl,--gc-sections)
endif()
