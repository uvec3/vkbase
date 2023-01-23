#include "Image.h"
#include <stdexcept>//for runtime_error

namespace vkbase
{

    void Image::create(
            EngineBase *eng, uint32_t width, uint32_t height, VkFormat format, VkMemoryPropertyFlags memProperty,
            VkImageUsageFlags usage, VkImageAspectFlags aspectFlags, VkMemoryHeapFlags heapFlags,
            VkImageLayout imageLayout, bool tillingLinear
    )
    {
        this->eng = eng;

        //Create image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

        //type of image:
        //VK_IMAGE_TYPE_1D,
        //VK_IMAGE_TYPE_2D,
        //VK_IMAGE_TYPE_3D
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        //extent of image
        imageInfo.extent.width = width;//optional
        imageInfo.extent.height = height;//optional
        //depth of image (only for 3D images)
        imageInfo.extent.depth = 1;
        //count of mip levels
        imageInfo.mipLevels = 1;
        //count of layers
        imageInfo.arrayLayers = 1;
        //format of image
        imageInfo.format = format;

        /*The tiling field can have one of two values:
         VK_IMAGE_TILING_LINEAR: texels are laid out in lines.
         VK_IMAGE_TILING_OPTIMAL: Texels are laid out in an implementation-defined order for optimal access.
         If you want to have direct access to texels in image memory, you must use VK_IMAGE_TILING_LINEAR*/
        imageInfo.tiling = tillingLinear ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;

        /*There are only two possible values for the initialLayout of an image(on this stage):
        VK_IMAGE_LAYOUT_UNDEFINED: Not usable by the GPU and the very first transition will discard the texels.
        VK_IMAGE_LAYOUT_PREINITIALIZED: Not usable by the GPU, but the first transition will preserve the texels.
        For either of these initial layouts, any image subresources must be transitioned to another layout before they are accessed by the device
        */;
        imageInfo.initialLayout = imageLayout;

        //describes the usage of the image
        imageInfo.usage = usage;
        //multisampling
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        //image will be used only by one queue family
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(eng->device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image!");
        }

        //get memory requirements for the image
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(eng->device, image, &memRequirements);

        //Allocate memory for the image
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = eng->findMemoryType( memRequirements.memoryTypeBits, memProperty, heapFlags);

        if (vkAllocateMemory(eng->device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate image memory!");
        }

        //bind image to memory
        vkBindImageMemory(eng->device, image, memory, 0);




        //IMAGE VIEW (representation of image for pipeline)
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(eng->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture image view!");
        }
    }

    void Image::destroy()
    {
        vkDestroyImageView(eng->device, imageView, nullptr);
        vkDestroyImage(eng->device, image, nullptr);
        vkFreeMemory(eng->device, memory, nullptr);
    }


}