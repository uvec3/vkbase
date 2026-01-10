cmake_minimum_required(VERSION 3.20)
project(vkbase)
set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

add_library(Buffer Buffer.cpp Buffer.hpp)
target_link_libraries(Buffer PUBLIC ${Vulkan_LIBRARIES})
target_include_directories(Buffer PUBLIC ${Vulkan_INCLUDE_DIRS})