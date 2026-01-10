cmake_minimum_required(VERSION 3.16)
project(vkbase)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(SDL)
find_package(Vulkan REQUIRED)

add_library(SDL_backend_core SDL_backend.cpp )

target_include_directories(SDL_backend_core
		PRIVATE ${Vulkan_INCLUDE_DIRS}
		PUBLIC ${SDL2_INCLUDE_DIRS})

if(DEV)#use shared for development and static for release
	set(SDL_LIB_NAME SDL2::SDL2)
else ()
	set(SDL_LIB_NAME SDL2::SDL2-static)
endif ()

target_link_libraries(SDL_backend_core
		PRIVATE ${Vulkan_LIBRARIES}
		PUBLIC SDL2main ${SDL_LIB_NAME})

add_library(SDL_backend_imgui imgui_impl_sdl.cpp)
target_include_directories(SDL_backend_imgui
		PRIVATE ../../extensions/imgui/imgui
		PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(SDL_backend_imgui
		PUBLIC SDL2main ${SDL_LIB_NAME}
		PRIVATE ${Vulkan_LIBRARIES})

#copy sdl2 shared lib to app
#add_custom_command(TARGET SDL_backend_core POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different
#		$<TARGET_FILE:SDL2>
#		$<TARGET_FILE_DIR:app>)