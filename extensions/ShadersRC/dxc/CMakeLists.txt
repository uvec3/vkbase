cmake_minimum_required(VERSION 3.16)
project(vkbase_dxc)
set(CMAKE_CXX_STANDARD 20)


add_library(ShadersDXC ${WEATHER_SHARED} ShadersDXC.cpp ShadersDXC.hpp)
target_link_libraries(ShadersDXC PRIVATE dxcompiler)
target_include_directories(ShadersDXC PRIVATE ${Vulkan_INCLUDE_DIRS})



