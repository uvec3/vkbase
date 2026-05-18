cmake_minimum_required(VERSION 3.16)
project(vkbase_dxc)


include(FetchContent)
FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
        URL_MD5 893b5203b862eb9bbd08553e24ff146a
        DOWNLOAD_EXTRACT_TIMESTAMP ON
)
set(BOOST_INCLUDE_LIBRARIES regex)
FetchContent_MakeAvailable(Boost)
#find_package(Boost REQUIRED COMPONENTS regex)

add_library(ShadersSlang STATIC ShaderCompilerSlang.cpp ShaderCompilerSlang.hpp)
# keep existing Vulkan include dirs and add Slang include dirs when enabled below
target_include_directories(ShadersSlang PUBLIC ${Vulkan_INCLUDE_DIRS})
include_directories(${Vulkan_INCLUDE_DIRS})

set(SLANG_ENABLE_TESTS OFF CACHE BOOL "Build Slang tests" FORCE)
set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "Build Slang tests" FORCE)
SET(SLANG_LIB_TYPE STATIC CACHE BOOL "Build Slang as a static library")

SET(SLANG_RHI_ENABLE_D3D12 OFF CACHE BOOL "" FORCE)
SET(SLANG_RHI_ENABLE_D3D11 OFF CACHE BOOL ""  FORCE)
SET(SLANG_RHI_ENABLE_AGILITY_SDK OFF CACHE BOOL ""  FORCE)
SET(SLANG_RHI_ENABLE_OPTIX OFF CACHE BOOL ""  FORCE)
SET(SLANG_ENABLE_GFX OFF CACHE BOOL ""  FORCE)

set(SLANG_ENABLE_CUDA OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_NVAPI OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_AFTERMATH OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_XLIB OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_DX_ON_VK OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_DXIL OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_SLANGD OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_SLANGC OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_SLANGI OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_SLANGRT OFF CACHE BOOL "" FORCE)

set(SLANG_ENABLE_REPLAYER OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_SLANG_RHI OFF CACHE BOOL "" FORCE)

add_subdirectory(slang)

target_link_libraries(ShadersSlang PUBLIC slang SPIRV-Tools-opt SPIRV-Tools-static Boost::regex)

