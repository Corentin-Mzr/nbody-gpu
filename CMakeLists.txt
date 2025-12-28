cmake_minimum_required(VERSION 3.30)
project(NBody-GPU)

# Set output folder to build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
foreach(OUTPUTCONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR})
endforeach()

# C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# OpenGL
find_package(OpenGL REQUIRED)

# GLFW
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
    SYSTEM)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.2
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
    SYSTEM)

set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
set(GLM_INSTALL_ENABLE OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glm)

# GLAD
FetchContent_Declare(
    glad
    URL https://github.com/Dav1dde/glad/archive/refs/tags/v2.0.8.zip
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
)

FetchContent_MakeAvailable(glad)
add_subdirectory("${glad_SOURCE_DIR}/cmake" glad_cmake)

glad_add_library(
    glad
    REPRODUCIBLE
    EXCLUDE_FROM_ALL
    LOADER
    API gl:core=4.6
)

# Glob for source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Glob for header files
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS src/*.hpp)

# Create exe
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
    src
)

# Link libs
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    OpenGL::GL
    glfw
    glm::glm
    glad
)

# # Copy fonts and shaders
# add_custom_target(copy_folders ALL
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     ${CMAKE_SOURCE_DIR}/fonts ${CMAKE_BINARY_DIR}/fonts
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/shaders
# )