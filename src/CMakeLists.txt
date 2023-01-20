cmake_minimum_required(VERSION 3.16)
project(vkbase_proj)



######################################
###PLATFORM DEPENDENT CONFIGURATION###
######################################

if(${ANDROID})
	list(APPEND PLATFORM_DEPEND_SOURCES )
else()
#	list(APPEND PLATFORM_DEPEND_SOURCES ../include/VKBASE/platform-dependent/SysResGLFW.cpp)
endif()



##########
###FIND###
##########

#find vulkan
find_package(Vulkan REQUIRED)


############
###TARGET###
############

add_library(vkbase_e Buffer.cpp EngineBase.cpp Image.cpp
		${PLATFORM_DEPEND_SOURCES}
		)


#############
###INCLUDE###
#############

target_include_directories(
		vkbase_e
		PUBLIC ${Vulkan_INCLUDE_DIRS}
		PUBLIC ${Vulkan_INCLUDE_DIRS}/../Third-Party/Include
		PUBLIC ../include
		PUBLIC ../third_party/glm

)


#############
###LINKING###
#############

#libraries
target_link_libraries(
		vkbase_e
		PUBLIC ${Vulkan_LIBRARIES}
)

if(VKBASE_SHADER_RUNTIME_COMPILE)
	target_link_libraries(vkbase_e PUBLIC shaderc)
endif()


#################
###DEFINITIONS###
#################

if(VKBASE_SHADER_RUNTIME_COMPILE)
	target_compile_definitions(vkbase_e PUBLIC VKBASE_SHADER_RUNTIME_COMPILE=1)
endif()

