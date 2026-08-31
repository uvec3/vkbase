cmake_minimum_required(VERSION 3.20)
project(vkbase)

find_package(Vulkan REQUIRED)

add_library(vkbase_Image Image.cpp Image.hpp)
target_link_libraries(vkbase_Image PUBLIC ${Vulkan_LIBRARIES}  vkbase_e)
target_include_directories(vkbase_Image PUBLIC ${Vulkan_INCLUDE_DIRS} )

add_library(vkbase::image ALIAS vkbase_Image)
