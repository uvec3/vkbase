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

add_library(vkbase_e  EngineBase.cpp
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

#if WINDOWS

# This will store all found files
set(ASSETS_ALL_FILES "")

# Iterate through directories
foreach(DIR IN LISTS ASSETS)
	if(EXISTS "${DIR}")
		file(GLOB_RECURSE DIR_FILES
				"${DIR}/*"
		)
		list(APPEND ASSETS_ALL_FILES ${DIR_FILES})
	else()
		message(WARNING "Directory not found: ${DIR}")
	endif()
endforeach()



add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/assets.cpp
		COMMAND ${CMAKE_COMMAND}
		"-DASSETS=${ASSETS}"
		-DDST_FILE=${CMAKE_CURRENT_BINARY_DIR}/assets.cpp
		-DVAR_NAME=assets
		-DNAMESPACE=vkbase
		-P ${CMAKE_CURRENT_SOURCE_DIR}/../CMake/create_assets.cmake
		COMMENT "creating assets.cpp"
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		DEPENDS ${ASSETS_ALL_FILES}
		)


target_sources(vkbase_e PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/assets.cpp)