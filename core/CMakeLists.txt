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
		PUBLIC ../external/imgui
		PUBLIC ../external/imgui/backends
)



###############
### FETCH #####
###############

# --- FETCH GLM ---
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1  # Or any stable tag/branch version
)
FetchContent_MakeAvailable(glm)

# --- FETCH VMA (Vulkan Memory Allocator) ---
FetchContent_Declare(
    vulkanmemoryallocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        v3.3.0 # Or the latest stable release tag
)
FetchContent_MakeAvailable(vulkanmemoryallocator)

###############
### LINKING ###
###############

#libraries
target_link_libraries(
		vkbase_e
		PUBLIC ${Vulkan_LIBRARIES}
		PUBLIC	glm::glm
		PUBLIC VulkanMemoryAllocator
)



