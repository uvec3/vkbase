cmake_minimum_required(VERSION 3.16)
project(vkbase)
set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

add_library(imgui imgui.hpp imgui.cpp
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
		imgui/misc/cpp/imgui_stdlib.cpp)

target_link_libraries(imgui PUBLIC ${Vulkan_LIBRARIES})

target_include_directories(imgui
		PUBLIC imgui
		PUBLIC imgui/misc/cpp
		PRIVATE imgui/backends
		PRIVATE ${Vulkan_INCLUDE_DIRS})

add_library(imgui_textedit ImGuiColorTextEdit/TextEditor.cpp ImGuiColorTextEdit/TextEditor.h)
target_include_directories( imgui_textedit PRIVATE imgui)

add_library(imgui_Log Log.cpp Log.hpp )
target_include_directories( imgui_Log PRIVATE imgui PRIVATE ${Vulkan_INCLUDE_DIRS})