#pragma once
#include <vulkan/vulkan.h>
#include "../../core/EngineBase.h"

namespace vkbase
{
    struct Image
    {
        VkImage image=VK_NULL_HANDLE;
        VkImageView image_view=VK_NULL_HANDLE;
        VmaAllocation allocation=VK_NULL_HANDLE;
        VmaAllocationInfo info{};

        Image(uint32_t width, uint32_t height, VkFormat format=VK_FORMAT_R8G8B8A8_UNORM);

        Image (Image& other)=delete;

        Image (Image&& other)noexcept;

        [[nodiscard]] VkImageMemoryBarrier2 imageBarrier() const;

        [[nodiscard]] VkImageMemoryBarrier2 imageBarrierGraphics() const;

        void barrier_cmd(VkCommandBuffer cmd) const;

        void barrier_graphics_cmd(VkCommandBuffer cmd) const;

        ~Image();
    };
}
