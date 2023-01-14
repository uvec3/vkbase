cmake_minimum_required(VERSION 3.16)
project(vkbase)
set(CMAKE_CXX_STANDARD 23)


set(VKBASE_SHADER_RUNTIME_COMPILE ON CACHE BOOL "Compile shaders at runtime extension for EngineBase")


if(VKBASE_SHADER_RUNTIME_COMPILE)
	find_package(Python COMPONENTS Interpreter REQUIRED)
	execute_process(
			COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/third_party/shaderc/utils/git-sync-deps
	)
	set(SHADERC_SKIP_TESTS true)
	add_subdirectory(third_party/shaderc)
endif()


add_subdirectory(src)