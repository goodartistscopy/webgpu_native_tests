cmake_minimum_required(VERSION 3.27)

project(WebGPUTest VERSION 0.1
                   DESCRIPTION "WebGPU test"
                   LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

option(USE_MOLD "Use mold linker instead ld" OFF)

set(MOLD_PATH "/usr/local/libexec/mold" CACHE STRING "Path to the mold executable directory")

# Configure the use of the faster mold linker instead of ld
# https://github.com/rui314/mold
# if(USE_MOLD AND UNIX)
#     # GCC >= 12.1.0 and Clang
#     if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12.1.0") OR
#        (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
#         message("Using mold linker")
#         set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=mold")
#         set(CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=mold")
#     # GCC < 12.1.0
#     elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#         message("Using mold linker at ${MOLD_PATH}")
#         set(CMAKE_EXE_LINKER_FLAGS "-B${MOLD_PATH}")
#         set(CMAKE_SHARED_LINKER_FLAGS "-B${MOLD_PATH}")
#     endif()
# endif()

add_subdirectory(deps/dawn EXCLUDE_FROM_ALL)
add_subdirectory(deps/imgui)

add_executable(wgpu_test)
target_sources(wgpu_test
    PRIVATE
    src/wgpu_test.cpp
    src/imgui_helpers.h
    src/imgui_helpers.cpp
    src/webgpu_helpers.h
    src/webgpu_helpers.cpp
)

#target_link_options(wgpu_test PRIVATE -lGL)
target_link_libraries(wgpu_test PRIVATE webgpu_glfw glfw webgpu_dawn imgui)
