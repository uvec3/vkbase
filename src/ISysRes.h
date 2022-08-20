#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>

//system-dependent methods interface
//necessary for creating engine
//you can use one of existed classes which implement this interface
//or define your own if here is no class for your platform
class ISysRes
{
public:
    //load files for Engine
    virtual std::vector<char> load_file(const char *path) = 0;

    //list of instance extensions which are necessary for platform
    virtual std::vector<const char *> getRequiredInstanceExtensions() = 0;

    //create VkSurfaceKHR for output
    virtual VkSurfaceKHR createSurface(VkInstance instance) = 0;
};
