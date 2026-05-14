cmake_minimum_required(VERSION 3.16)
project(vkbase)


add_library(VkComputeEng
		ComputeEng.cpp ComputeEng.h)
target_link_libraries(VkComputeEng Vulkan::Vulkan)
target_include_directories(VkComputeEng PUBLIC ../../core/glm)
