cmake_minimum_required(VERSION 4.0) # CMake version
project(Cicada_Engine)  # Project Name
set(CMAKE_CXX_STANDARD 20)  # C++ Version
add_executable(${PROJECT_NAME} main.cpp)    # Executable build upon main.cpp

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")    # Add to the search path CMake directory (root of the project)
find_package(Vulkan REQUIRED)  # Required package

find_package(glfw3 CONFIG REQUIRED)
#find_package(glm CONFIG REQUIRED) # Included with Vulkan SDK but maybe need to be download with VCPKG

find_package(tinyobjloader CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE tinyobjloader::tinyobjloader)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)