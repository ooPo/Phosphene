cmake_minimum_required(VERSION 3.16)
project(Phosphene VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ==============================================================
#  Dependencies
# ==============================================================

find_package(SDL3 REQUIRED)
find_package(Soxr REQUIRED)

# ==============================================================
#  Library target
# ==============================================================

add_library(phosphene
    src/window.cpp
    src/display.cpp
    src/renderer.cpp
    src/audio.cpp
    src/resampler.cpp
    src/input.cpp
    src/math3d.cpp
)

add_library(Phosphene::phosphene ALIAS phosphene)

set_target_properties(phosphene PROPERTIES
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(phosphene PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Both SDL3 and Soxr headers appear in our public headers, so both are PUBLIC.
target_link_libraries(phosphene PUBLIC SDL3::SDL3 Soxr::Soxr)

if(MSVC)
    target_compile_options(phosphene PRIVATE /W4)
else()
    target_compile_options(phosphene PRIVATE -Wall -Wextra)
endif()

# ==============================================================
#  Examples (opt-in)
# ==============================================================

option(PHOSPHENE_BUILD_EXAMPLES "Build example programs" OFF)

if(PHOSPHENE_BUILD_EXAMPLES)
    add_executable(phosphene_example examples/basic/main.cpp)
    target_link_libraries(phosphene_example PRIVATE phosphene)

    add_executable(phosphene_cube examples/cube/main.cpp)
    target_link_libraries(phosphene_cube PRIVATE phosphene)

    add_executable(phosphene_textured_cube examples/textured_cube/main.cpp)
    target_link_libraries(phosphene_textured_cube PRIVATE phosphene)

    add_executable(phosphene_hud examples/hud/main.cpp)
    target_link_libraries(phosphene_hud PRIVATE phosphene)
endif()

# ==============================================================
#  Install + package config
# ==============================================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(PHOSPHENE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/Phosphene)

install(TARGETS phosphene
    EXPORT    PhospheneTargets
    ARCHIVE   DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY   DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME   DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/phosphene
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT PhospheneTargets
    FILE      PhospheneTargets.cmake
    NAMESPACE Phosphene::
    DESTINATION ${PHOSPHENE_INSTALL_CMAKEDIR}
)

configure_package_config_file(
    cmake/PhospheneConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PhospheneConfig.cmake
    INSTALL_DESTINATION ${PHOSPHENE_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/PhospheneConfigVersion.cmake
    VERSION     ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/PhospheneConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/PhospheneConfigVersion.cmake
    cmake/FindSoxr.cmake
    DESTINATION ${PHOSPHENE_INSTALL_CMAKEDIR}
)
