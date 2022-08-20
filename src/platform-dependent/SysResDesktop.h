#pragma once


#include <vulkan/vulkan.h>
#include "../ISysRes.h"


class SysResDesktop: public ISysRes
{
    std::vector<char> load_file(const char *path) override;

    VkSurfaceKHR createSurface(VkInstance instance) override;

public:
    std::vector<const char *> getRequiredInstanceExtensions() override;
};


