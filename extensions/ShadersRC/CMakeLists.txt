cmake_minimum_required(VERSION 3.16)
project(vkbase)

find_package(Vulkan REQUIRED)
find_package(Vulkan REQUIRED COMPONENTS glslang)

add_subdirectory(dxc)
add_subdirectory(slang)

add_library (SPIRV-Reflect STATIC
		SPIRV-Reflect/spirv_reflect.c
		SPIRV-Reflect/spirv_reflect.h
)

target_include_directories(SPIRV-Reflect PUBLIC SPIRV-Reflect)

add_library(ShadersRC ${WEATHER_SHARED} ShadersRC.cpp ShadersRC.hpp
		ShaderType.hpp)
target_link_libraries(ShadersRC PRIVATE SPIRV-Reflect Vulkan::glslang)

if (TARGET Vulkan::glslang-default-resource-limits)
	target_link_libraries(ShadersRC PRIVATE Vulkan::glslang-default-resource-limits)
elseif (TARGET glslang::glslang-default-resource-limits)
	target_link_libraries(ShadersRC PRIVATE glslang::glslang-default-resource-limits)
else()
	target_link_libraries(ShadersRC PRIVATE glslang-default-resource-limits)
endif()
target_include_directories(ShadersRC PRIVATE ${Vulkan_INCLUDE_DIRS})