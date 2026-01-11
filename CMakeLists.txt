cmake_minimum_required(VERSION 3.16)
project(vkbase)
set(CMAKE_CXX_STANDARD 20)

##include ability of shader compilation inside engine
#set(VKBASE_SHADER_RUNTIME_COMPILE OFF  BOOL "Compile shaders at runtime extension for EngineBase")
message(STATUS "VKBASE_SHADER_RUNTIME_COMPILE: ${VKBASE_SHADER_RUNTIME_COMPILE}")



#add engine base library
add_subdirectory(backends/SDL_backend)
add_subdirectory(core)
add_subdirectory(extensions/imgui)
add_subdirectory(extensions/Buffer)
add_subdirectory(extensions/ShadersRC)
#add_subdirectory(extensions/compute)



##System backend for GLFW desktop
#add_library(glfwSysRes third_party/imgui/backends/imgui_impl_glfw.cpp)
#target_include_directories(glfwSysRes
#		PUBLIC third_party/imgui
#		PUBLIC third_party/imgui/backends
#		INTERFACE include/VKBASE/platform-dependent)
#target_link_libraries(glfwSysRes PUBLIC glfw)
#
#
#
##System backend for android game activity
#add_library(androidSysRes third_party/imgui/backends/imgui_impl_android.cpp)
#target_include_directories(androidSysRes
#		PUBLIC third_party/imgui
#		PUBLIC third_party/imgui/backends
#		INTERFACE include/VKBASE/platform-depen