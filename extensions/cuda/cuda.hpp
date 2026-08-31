#pragma once
#include <vulkan/vulkan_core.h>
#include <cuda_runtime.h>

#ifdef _WIN32

    #define WIN32_LEAN_AND_MEAN
    #define VK_USE_PLATFORM_WIN32_KHR
    #include <windows.h>

    #include <vulkan/vulkan_win32.h>

    // Windows constants
    typedef HANDLE NativePlatformHandle;
constexpr  VkExternalMemoryHandleTypeFlagBits VK_EXTERNAL_MEMORY_HANDLE_TYPE_PLATFORM =
    VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
constexpr  cudaExternalMemoryHandleType CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_PLATFORM =
    cudaExternalMemoryHandleTypeOpaqueWin32;
static const char* VK_EXTERNAL_MEMORY_EXTENSION_NAME =
    VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME;

#else

#include <unistd.h>

// Linux constants
typedef int NativePlatformHandle;
const VkExternalMemoryHandleTypeFlagBits VK_EXTERNAL_MEMORY_HANDLE_TYPE_PLATFORM =
    VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
const cudaExternalMemoryHandleType CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_PLATFORM =
    cudaExternalMemoryHandleTypeOpaqueFd;
const char* VK_EXTERNAL_MEMORY_EXTENSION_NAME =
    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME;
#endif




namespace vkbase::cuda
{

#ifdef _WIN32
    extern PFN_vkGetMemoryWin32HandleKHR pfnVkGetMemoryWin32HandleKHR;
#else
    extern PFN_vkGetMemoryFdKHR pfnVkGetMemoryFdKHR;
#endif

    //registers required Vulkan extensions
    //must be called before vkbase::init()
    void init();
}
