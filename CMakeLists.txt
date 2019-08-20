cmake_minimum_required(VERSION 3.14)
project(vulkan-tutorial C)
include(FetchContent)

set(C_STANDARD 11)
set(C_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

set(GLFW_USE_WAYLAND    ON  CACHE BOOL "")
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY  https://github.com/glfw/glfw
    GIT_TAG         3.3
    GIT_SHALLOW     ON
)
FetchContent_MakeAvailable(glfw)

add_executable(app src/app.c)
target_link_libraries(
    app
    PRIVATE glfw
    PUBLIC ${Vulkan_LIBRARY}
)
target_include_directories(
    app
    PRIVATE "${GLFW_SOURCE_DIR}/include"
    PUBLIC ${Vulkan_INCLUDE_DIR}
)