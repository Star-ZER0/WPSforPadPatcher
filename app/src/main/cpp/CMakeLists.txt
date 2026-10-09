cmake_minimum_required(VERSION 3.22.1)
project(wpsforpadpatcher LANGUAGES CXX)

add_library(wpsforpadpatcher SHARED native_hook.cpp jni_bridge.cpp)
target_compile_features(wpsforpadpatcher PRIVATE cxx_std_17)
target_compile_options(wpsforpadpatcher PRIVATE
    -Wall -Wextra -Werror -fvisibility=hidden -fno-exceptions -fno-rtti)
target_link_options(wpsforpadpatcher PRIVATE
    "-Wl,-z,max-page-size=16384"
    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map")
set_target_properties(wpsforpadpatcher PROPERTIES
    LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/exports.map")
target_link_libraries(wpsforpadpatcher PRIVATE log dl)
