cmake_minimum_required(VERSION 3.16)
project(vkbase)
set(CMAKE_CXX_STANDARD 20)


set(VKBASE_SHADER_RUNTIME_COMPILE OFF CACHE BOOL "Compile shaders at runtime extension for EngineBase")

#print message
message(STATUS "!!!!!!!!!!!!!!!!!!!!!!!!!!!VKBASE_SHADER_RUNTIME_COMPILE: ${VKBASE_SHADER_RUNTIME_COMPILE}")

if(VKBASE_SHADER_RUNTIME_COMPILE)
	find_package(Python COMPONENTS Interpreter REQUIRED)
	execute_process(
			COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/third_party/shaderc/utils/git-sync-deps
	)
	set(SHADERC_SKIP_TESTS true)
	add_subdirectory(third_party/shaderc)
endif()


add_subdirectory(src)

add_library(glfwSysRes INTERFACE)
target_include_directories(glfwSysRes INTERFACE include/VKBASE/platform-dependent/SysResGLFW.h)
target_sources(glfwSysRes INTERFACE third_party/imgui/backends/imgui_impl_glfw.cpp)

add_library(androidSysRes INTERFACE)
target_include_directories(androidSysRes INTERFACE include/VKBASE/platform-dependent/AndroidSysRes.h)
target_sources(androidSysRes INTERFACE third_party/imgui/backends/imgui_impl_android.cpp)