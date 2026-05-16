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

add_library(vkbase_e  EngineBase.cpp ParallelTaskManager.cpp
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


##################
## ENGINE ASSETS #
##################

include(${CMAKE_CURRENT_SOURCE_DIR}/../CMake/CMakeRC.cmake)

cmrc_add_resource_library(vkbase_assets_rc NAMESPACE vkbase_assets)

foreach(DIR IN LISTS ASSETS)
	if(EXISTS "${DIR}")
		get_filename_component(DIR_PARENT "${DIR}" DIRECTORY)
		file(GLOB_RECURSE DIR_FILES CONFIGURE_DEPENDS
				"${DIR}/*"
		)
		if(DIR_FILES)
			cmrc_add_resources(vkbase_assets_rc WHENCE "${DIR_PARENT}" ${DIR_FILES})
		endif()
	else()
		message(WARNING "Directory not found: ${DIR}")
	endif()
endforeach()

target_link_libraries(vkbase_e PUBLIC vkbase_assets_rc)
