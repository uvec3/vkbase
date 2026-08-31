cmake_minimum_required(VERSION 3.20)
project(vkbase)

find_package(Vulkan REQUIRED)

add_library(vkbase_Buffer Buffer.cpp Buffer.hpp)
target_link_libraries(vkbase_Buffer PUBLIC ${Vulkan_LIBRARIES}  vkbase_e)
target_include_directories(vkbase_Buffer PUBLIC ${Vulkan_INCLUDE_DIRS} )


add_library(vkbase::buffer ALIAS vkbase_Buffer)
