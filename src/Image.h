#pragma once
#include <vulkan/vulkan.h>//VK
#include "EngineBase.h"



namespace vkbase
{
    class EngineBase;

    class Image
    {
        EngineBase *eng = nullptr;

    public:
        VkImage image;
        VkDeviceMemory memory;
        VkImageView imageView;


        void create(
                EngineBase *eng, uint32_t width, uint32_t height, VkFormat format, VkMemoryPropertyFlags memProperty,
                VkImageUsageFlags usage = 0, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
                VkMemoryHeapFlags heapFlags = 0, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                bool tillingLinear = false
        );


        void destroy();
    };
}