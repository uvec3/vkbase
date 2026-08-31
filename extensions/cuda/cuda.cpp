#include "cuda.hpp"

#include <iostream>

#include "../../core/EngineBase.h"




namespace vkbase::cuda
{
#ifdef _WIN32
    PFN_vkGetMemoryWin32HandleKHR pfnVkGetMemoryWin32HandleKHR = nullptr;
#else
    PFN_vkGetMemoryFdKHR pfnVkGetMemoryFdKHR = nullptr;
#endif


    void deviceCreated()
    {
#ifdef _WIN32
        pfnVkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR");

        if (!pfnVkGetMemoryWin32HandleKHR) {
            // Handle error: Function not found. (Did you enable VK_KHR_external_memory_win32?)
        }
#else
        pfnVkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");

        if (!pfnVkGetMemoryFdKHR) {
            // Handle error: Function not found. (Did you enable VK_KHR_external_memory_fd?)
        }
#endif
    }


    void init()
    {
        vkbase::addDeviseExtension(VK_EXTERNAL_MEMORY_EXTENSION_NAME);
        vkbase::addDeviseExtension(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
        std::cout<<"Externtion registered\n";
        vkbase::addInitCallback(deviceCreated);
    }
}
