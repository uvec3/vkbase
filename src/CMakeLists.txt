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

add_library(vkbase_e Buffer.cpp EngineBase.cpp Image.cpp
		${PLATFORM_DEPEND_SOURCES}

		../third_party/imgui/imgui.cpp
		../third_party/imgui/imgui_draw.cpp
		../third_party/imgui/imgui_demo.cpp
		../third_party/imgui/imgui_tables.cpp
		../third_party/imgui/imgui_widgets.cpp
		../third_party/imgui/backends/imgui_impl_vulkan.cpp)


###############
### INCLUDE ###
###############

target_include_directories(
		vkbase_e
		PUBLIC ${Vulkan_INCLUDE_DIRS}
		PUBLIC ${Vulkan_INCLUDE_DIRS}/../Third-Party/Include
		PUBLIC ../include
		PUBLIC ../third_party/glm
		PUBLIC ../third_party/imgui
		PUBLIC ../third_party/imgui/backends
)


###############
### LINKING ###
###############

#libraries
target_link_libraries(
		vkbase_e
		PUBLIC ${Vulkan_LIBRARIES}
)

if(VKBASE_SHADER_RUNTIME_COMPILE)
	target_link_libraries(vkbase_e PUBLIC shaderc)
endif()


###################
### DEFINITIONS ###
###################

if(VKBASE_SHADER_RUNTIME_COMPILE)
	target_compile_definitions(vkbase_e PUBLIC VKBASE_SHADER_RUNTIME_COMPILE=1)
endif()
##################
## ENGINE ASSETS #
##################

add_custom_command(OUTPUT assets.cpp
		COMMAND ${CMAKE_COMMAND}
		"-DASSETS=${ASSETS}"
		-DDST_FILE=${CMAKE_CURRENT_BINARY_DIR}/assets.cpp
		-DVAR_NAME=__engine_assets_var__
		-P ${CMAKE_CURRENT_SOURCE_DIR}/../CMake/create_assets.cmake
		COMMENT "creating assets.cpp"
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		DEPENDS ${ASSETS}
		)
target_sources(vkbase_e PRIVATE assets.cpp)