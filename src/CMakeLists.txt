cmake_minimum_required(VERSION 3.16)
project(vkbase_proj)




######################################
###PLATFORM DEPENDENT CONFIGURATION###
######################################

if(${ANDROID})
	list(APPEND PLATFORM_DEPEND_SOURCES platform-dependent/AndroidSysRes.cpp)
else()
	list(APPEND PLATFORM_DEPEND_SOURCES platform-dependent/SysResDesktop.cpp)
endif()



##########
###FIND###
##########

#find vulkan
find_package(Vulkan REQUIRED)


############
###TARGET###
############

add_library(vkbase_e Buffer.cpp EngineBase.cpp Image.cpp MyVkAdditional.cpp
		${PLATFORM_DEPEND_SOURCES}
		)


#############
###INCLUDE###
#############

target_include_directories(
		vkbase_e
		PUBLIC ${Vulkan_INCLUDE_DIRS}
)


#############
###LINKING###
#############

#libraries
target_link_libraries(
		vkbase_e
		PUBLIC ${Vulkan_LIBRARIES}
)