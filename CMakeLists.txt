cmake_minimum_required(VERSION 3.27)
project(CicadaEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(PROJECTS Cicada)

# add any extra CMake files
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")

# ------------ GTest configuration ------------
# Fetch and download Gtest if necessary
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# then configure Gtest target
enable_testing()

file(GLOB_RECURSE TESTS_SOURCES "Tests/*.cpp")

add_executable(CicadaTests ${TESTS_SOURCES})
target_link_libraries(CicadaTests GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(CicadaTests)
# ------------ GTest configuration ------------

# For now, we stick to a single target but we may want to
# change the build setup to handle multiple targets to, for instance, generate a DLL of the engine to be reused in a game
# made with Cicada
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Find all libraries. Platform dependant, let's try to keep this up and running on Windows/Linux at least
find_package (glfw3 REQUIRED)
find_package (glm REQUIRED)
find_package (Vulkan REQUIRED)
find_package (tinyobjloader REQUIRED)
#find_package (KTX REQUIRED)

find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)

if(WIN32)
    #add_definitions(-D_CRT_SECURE_NO_WARNINGS) # i've seen this in the learnOpenGL CMakeList, try to decomment it if a warning appears on a regular basis
elseif(UNIX AND NOT APPLE)
    #FIXME by someone that has a linux system in its immediate proximity
elseif(APPLE)
    # Sorry we're still poor people
endif()

file(GLOB_RECURSE SOURCES
        "Source/${project}/**/*.h"
        "Source/${project}/**/*.cpp"
)

file(GLOB_RECURSE SHADER_SOURCES
    "Resources/Shaders/src/*.slang"
)

include(CMakePrintHelpers)
cmake_print_variables(SHADER_SOURCES)

add_executable(CicadaEngine ${SOURCES})
target_link_libraries(CicadaEngine ${LIBRARIES})

target_compile_definitions(CicadaEngine PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

# shaders compilation
function (add_slang_shader_target TARGET)
    cmake_parse_arguments ("SHADER" "" "" "SOURCES" ${ARGN})
    set (SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR}/Resources/Shaders/src)
    set (SHADERS_OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/Resources/Shaders/bin)
    set (ENTRY_POINTS -entry vertMain -entry fragMain)
    add_custom_command (
            OUTPUT ${SHADERS_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
    )
    add_custom_command (
            OUTPUT  ${SHADERS_DIR}/slang.spv
            COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o ${SHADERS_OUTPUT_DIR}/slang.spv
            WORKING_DIRECTORY ${SHADERS_DIR}
            DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
            COMMENT "Compiling Slang Shaders"
            VERBATIM
    )
    add_custom_target (${TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()
add_slang_shader_target(Shaders SOURCES ${SHADER_SOURCES})
add_dependencies(CicadaEngine Shaders)

target_link_libraries(CicadaEngine glfw Vulkan::Vulkan)