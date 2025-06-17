cmake_minimum_required(VERSION 3.10)
project(MyVulkanApp)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

include_directories(${SDL2_INCLUDE_DIRS})
link_directories(${SDL2_LIBRARY_DIRS})

set(IMGUI_SRC
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_demo.cpp
    imgui/backends/imgui_impl_sdl2.cpp
    imgui/backends/imgui_impl_vulkan.cpp
)

add_subdirectory(vk-bootstrap)

add_executable(MyVulkanApp
  ./src/main.cpp
  ./src/engine.cpp
  ./src/initializers.cpp
  ./src/vertexData.cpp
  ./src/camera.cpp
  ${IMGUI_SRC}
)

target_include_directories(MyVulkanApp PRIVATE imgui imgui/backends)
target_link_libraries(MyVulkanApp
  PRIVATE
    Vulkan::Vulkan
    vk-bootstrap::vk-bootstrap
    ${SDL2_LIBRARIES}
)

