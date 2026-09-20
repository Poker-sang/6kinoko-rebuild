set(KINOKO_BOOST_ROOT "${CMAKE_CURRENT_LIST_DIR}/../third_party/boost-1.44.0")
add_library(kinoko_boost_count STATIC "${CMAKE_CURRENT_LIST_DIR}/../src/reconstructed/boost_control.cpp")
if(WIN32)
    target_sources(kinoko_boost_count PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../src/reconstructed/boost_hash.cpp")
    if(MSVC)
        # Boost 1.44 derives hash from std::unary_function. Enable the genuine
        # MSVC compatibility declaration for this source without patching Boost
        # or inventing a replacement in namespace std.
        set_property(SOURCE "${CMAKE_CURRENT_LIST_DIR}/../src/reconstructed/boost_hash.cpp"
            APPEND PROPERTY COMPILE_DEFINITIONS _HAS_AUTO_PTR_ETC=1)
    endif()
endif()
target_include_directories(kinoko_boost_count PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../include")
target_include_directories(kinoko_boost_count PRIVATE "${KINOKO_BOOST_ROOT}")
if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(kinoko_boost_count PUBLIC Threads::Threads)
endif()
