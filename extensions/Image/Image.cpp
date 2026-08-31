#include "Image.hpp"
#include <stdexcept>
#include "vulkan/vulkan.h"

namespace vkbase
{
    Image::Image(uint32_t width, uint32_t height, VkFormat format)
    {
        VkImageCreateInfo image_create_info{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        image_create_info.flags=0;
        image_create_info.imageType=VK_IMAGE_TYPE_2D;
        image_create_info.format=format;
        image_create_info.extent=VkExtent3D{.width = width,.height = height,.depth = 1};
        image_create_info.mipLevels=1;
        image_create_info.arrayLayers=1;
        image_create_info.samples=VK_SAMPLE_COUNT_1_BIT;
        image_create_info.tiling=VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage=VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
        image_create_info.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;

        uint32_t queueFamilyIndices[2];
        if (vkbase::queueFamilyIndices.computeFamily==vkbase::queueFamilyIndices.graphicsFamily)
            image_create_info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        else
        {
            queueFamilyIndices[0]=vkbase::queueFamilyIndices.computeFamily.value();
            queueFamilyIndices[1]=vkbase::queueFamilyIndices.graphicsFamily.value();
            image_create_info.sharingMode=VK_SHARING_MODE_CONCURRENT;
            image_create_info.queueFamilyIndexCount=2;
            image_create_info.pQueueFamilyIndices=queueFamilyIndices;
        }


        VmaAllocationCreateInfo allocation_create_info{};
        allocation_create_info.usage=VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        vmaCreateImage(vkbase::vma,&image_create_info,&allocation_create_info,&image,&allocation,&info);


        VkImageViewCreateInfo view_create_info{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_create_info.format = format;
        view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_create_info.subresourceRange.baseMipLevel = 0;
        view_create_info.subresourceRange.levelCount = 1;
        view_create_info.subresourceRange.baseArrayLayer = 0;
        view_create_info.subresourceRange.layerCount = 1;
        view_create_info.image=image;
        vkCreateImageView(vkbase::device,&view_create_info,nullptr,&image_view);
    }

    Image::Image(Image&& other) noexcept
    {
        image=other.image;
        image_view=other.image_view;
        allocation=other.allocation;
        info=other.info;

        other.image=VK_NULL_HANDLE;
    }

    VkImageMemoryBarrier2 Image::imageBarrier() const
    {
        VkImageMemoryBarrier2 imgBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,            // from UNDEFINED
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT|VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS
            }
        };
        return imgBarrier;
    }

    VkImageMemoryBarrier2 Image::imageBarrierGraphics() const
    {
        VkImageMemoryBarrier2 imgBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,            // from UNDEFINED
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS
            }
        };
        return imgBarrier;
    }

    void Image::barrier_cmd(VkCommandBuffer cmd) const
    {
        auto barrier=imageBarrier();

        VkDependencyInfo depInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
        };

        vkCmdPipelineBarrier2(cmd, &depInfo);
    }

    void Image::barrier_graphics_cmd(VkCommandBuffer cmd) const
    {
        auto barrier=imageBarrierGraphics();

        VkDependencyInfo depInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
        };

        vkCmdPipelineBarrier2(cmd, &depInfo);
    }

    Image::~Image()
    {
        if (image)
        {
            vkDestroyImageView(vkbase::device,image_view,nullptr);
            vmaDestroyImage(vkbase::vma,image,allocation);
            image=VK_NULL_HANDLE;
        }
    }
}
