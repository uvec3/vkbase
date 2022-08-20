#pragma once

#include<vulkan/vulkan.hpp>//VK
#include <stdexcept>//for runtime_error
#include "EngineBase.h"

class EngineBase;

namespace vkbase
{

    class Image
    {
        EngineBase *eng = nullptr;

    public:
        VkImage image;
        VkDeviceMemory memory;
        VkImageView imageView;


        Image(
                EngineBase *eng, uint32_t width, uint32_t height, VkFormat format, VkMemoryPropertyFlags memProperty,
                VkImageUsageFlags usage = 0, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                VkMemoryHeapFlags heapFlags = 0, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                bool tillingLinear = false
        );


        ~Image();
    };
}