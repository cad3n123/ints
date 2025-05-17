cmake_minimum_required(VERSION 3.14)
project(MyGLFWApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include paths
include_directories(include)
include_directories(external)
include_directories(external/imgui)
include_directories(external/imgui/backends)

# Source files
file(GLOB_RECURSE APP_SOURCES src/*.cpp)

set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

# Use GLFW via vcpkg or system
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Create executable
add_executable(main ${APP_SOURCES} ${IMGUI_SOURCES})

# Link libraries
target_link_libraries(main PRIVATE glfw OpenGL::GL)

# Enable warnings
target_compile_options(main PRIVATE -Wall -Wextra)
