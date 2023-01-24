cmake_minimum_required(VERSION 3.16)
project(vkbase)
set(CMAKE_CXX_STANDARD 20)

#include ability of shader compilation inside engine
set(VKBASE_SHADER_RUNTIME_COMPILE OFF CACHE BOOL "Compile shaders at runtime extension for EngineBase")
message(STATUS "VKBASE_SHADER_RUNTIME_COMPILE: ${VKBASE_SHADER_RUNTIME_COMPILE}")

#if shader compilation is enabled, setup shaderc dependency
if(VKBASE_SHADER_RUNTIME_COMPILE)
	find_package(Python COMPONENTS Interpreter REQUIRED)
	execute_process(
			COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/third_party/shaderc/utils/git-sync-deps
	)
	set(SHADERC_SKIP_TESTS true)
	add_subdirectory(third_party/shaderc)
endif()

#add engine base library
add_subdirectory(src)

#System backend for android game activity
add_library(glfwSysRes third_party/imgui/backends/imgui_impl_glfw.cpp)
target_include_directories(glfwSysRes
		PUBLIC third_party/imgui
		PUBLIC third_party/imgui/backends
		INTERFACE include/VKBASE/platform-dependent)


#System backend for GLFW desktop
add_library(androidSysRes third_party/imgui/backends/imgui_impl_android.cpp)
target_include_directories(androidSysRes
		PUBLIC third_party/imgui
		PUBLIC third_party/imgui/backends
		INTERFACE include/VKBASE/platform-dependent)
