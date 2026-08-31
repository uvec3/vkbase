cmake_minimum_required(VERSION 3.16)
project(vkbase)

find_package(Vulkan REQUIRED)

add_library(vkbase_imgui imgui.hpp imgui.cpp
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
		imgui/misc/cpp/imgui_stdlib.cpp)


target_link_libraries(vkbase_imgui PUBLIC ${Vulkan_LIBRARIES} vkbase::assets vkbase_e)


find_package(TBB QUIET)
if(TBB_FOUND)
	target_link_libraries(vkbase_imgui PRIVATE TBB::tbb)
endif()



target_include_directories(vkbase_imgui
		PUBLIC imgui
		PUBLIC imgui/misc/cpp
		PRIVATE imgui/backends
		PRIVATE ${Vulkan_INCLUDE_DIRS})

add_library(imgui_textedit ImGuiColorTextEdit/TextEditor.cpp ImGuiColorTextEdit/TextEditor.h)
target_include_directories( imgui_textedit PRIVATE imgui)

add_library(vkbase_imgui_log Log.cpp Log.hpp )
target_include_directories(vkbase_imgui_log PRIVATE imgui PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vkbase_imgui_log PUBLIC vkbase_e)


add_library(vkbase::imgui ALIAS vkbase_imgui)
add_library(vkbase::imgui::log ALIAS vkbase_imgui_log)