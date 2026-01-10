cmake_minimum_required(VERSION 3.16)
project(vkbase_dxc)
set(CMAKE_CXX_STANDARD 20)


add_library(ShadersSlang STATIC ShaderCompilerSlang.cpp ShaderCompilerSlang.hpp)
# keep existing Vulkan include dirs and add Slang include dirs when enabled below
target_include_directories(ShadersSlang PRIVATE ${Vulkan_INCLUDE_DIRS})


set(SLANG_ENABLE_TESTS OFF CACHE BOOL "Build Slang tests" FORCE)
set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "Build Slang tests" FORCE)
SET(SLANG_LIB_TYPE STATIC CACHE BOOL "Build Slang as a static library")
SET(SLANG_RHI_ENABLE_D3D12 OFF CACHE BOOL "" FORCE)
SET(SLANG_RHI_ENABLE_D3D11 OFF CACHE BOOL ""  FORCE)
SET(SLANG_RHI_ENABLE_AGILITY_SDK OFF CACHE BOOL ""  FORCE)
SET(SLANG_RHI_ENABLE_OPTIX OFF CACHE BOOL ""  FORCE)
add_subdirectory(slang)
target_link_libraries(ShadersSlang PUBLIC slang SPIRV-Tools-opt SPIRV-Tools-static)
