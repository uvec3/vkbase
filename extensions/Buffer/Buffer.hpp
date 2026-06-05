#pragma once
#include <vulkan/vulkan.h>
#include "../../core/EngineBase.h"


namespace vkbase
{
    class Buffer
    {
    public:

        Buffer()=default;
        Buffer (Buffer& other)=delete;
        Buffer (Buffer&& other)noexcept;
        explicit Buffer(VkDeviceSize size,VkBufferUsageFlags usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        VmaAllocationCreateFlags flags=0,const std::vector<uint32_t>& queue_families={});
        ~Buffer();

        Buffer& operator=(const Buffer& other)=delete;
        Buffer& operator=(Buffer&& other);


        operator VkBuffer() const
        {
            return get();
        }

        char* map();
        void unmap();
        void copy_to_buffer_cmd(VkCommandBuffer cmd, const Buffer& dst_buffer, VkDeviceSize copy_size=0, VkDeviceSize src_offset=0, VkDeviceSize dst_offset=0);
        void copy_to_buffer(const Buffer& dst_buffer, VkDeviceSize copy_size=0, VkDeviceSize src_offset=0, VkDeviceSize dst_offset=0);
        [[nodiscard]] VkDeviceSize size() const;
        VkBuffer get() const;
        [[nodiscard]] VkMemoryPropertyFlags getMemoryPropertyFlags() const;
        [[nodiscard]] bool isDeviceLocal() const;
        [[nodiscard]] VkMemoryRequirements getBufferMemoryRequirements() const;
        std::string print_as_uint32_array(const std::string& name);


    private:
        VkBuffer buffer=VK_NULL_HANDLE;
        VmaAllocation allocation=VK_NULL_HANDLE;
        VmaAllocationInfo info{};
        VkDeviceSize m_size;
    };
}
