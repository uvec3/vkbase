cmake_minimum_required(VERSION 3.16)
project(vkbase_proj)


########################################
### PLATFORM DEPENDENT CONFIGURATION ###
########################################

if(${ANDROID})
#	list(APPEND PLATFORM_DEPEND_SOURCES )
else()
#	list(APPEND PLATFORM_DEPEND_SOURCES ../include/VKBASE/platform-dependent/SysResGLFW.cpp)
endif()

#################
### FIND LIBS ###
#################

#find vulkan
find_package(Vulkan REQUIRED)

##############
### TARGET ###
##############

add_library(vkbase_e  EngineBase.cpp ParallelTaskManager.cpp messages.cpp
		${PLATFORM_DEPEND_SOURCES})


###############
### INCLUDE ###
###############

target_include_directories(
		vkbase_e
		PUBLIC ${Vulkan_INCLUDE_DIRS}
		PUBLIC ${Vulkan_INCLUDE_DIRS}/../Third-Party/Include
		PUBLIC ../include
		PUBLIC ../external/glm
		PUBLIC ../external/imgui
		PUBLIC ../external/imgui/backends
)


###############
### LINKING ###
###############

#libraries
target_link_libraries(
		vkbase_e
		PUBLIC ${Vulkan_LIBRARIES}
)



