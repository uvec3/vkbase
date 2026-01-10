cmake_minimum_required(VERSION 3.16)
project(vkbase)
set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

add_subdirectory(dxc)
add_subdirectory(slang)

add_library (SPIRV-Reflect STATIC
		SPIRV-Reflect/spirv_reflect.c
		SPIRV-Reflect/spirv_reflect.h
)

target_include_directories(SPIRV-Reflect PUBLIC SPIRV-Reflect)

add_library(ShadersRC ${WEATHER_SHARED} ShadersRC.cpp ShadersRC.hpp
		ShaderType.hpp)
target_link_libraries(ShadersRC PRIVATE SPIRV-Reflect)
target_include_directories(ShadersRC PRIVATE ${Vulkan_INCLUDE_DIRS})