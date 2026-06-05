#include "Buffer.hpp"
#include <stdexcept>//for runtime_error
#include "vulkan/vulkan.h"

namespace vkbase
{
    Buffer::Buffer(Buffer&& other) noexcept
    {
        *this= std::move(other);
    }

    Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags,
                   const std::vector<uint32_t>& queue_families)
    {
        VkBufferCreateInfo buffer_create_info{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_create_info.usage =usage;
        buffer_create_info.size = size;
        if (queue_families.size()>1)
        {
            buffer_create_info.sharingMode=VK_SHARING_MODE_CONCURRENT;
            buffer_create_info.queueFamilyIndexCount=queue_families.size();
            buffer_create_info.pQueueFamilyIndices=queue_families.data();
        }

        VmaAllocationCreateInfo allocation_create_info{};
        allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
        allocation_create_info.flags = flags;

        vmaCreateBuffer(vkbase::vma,&buffer_create_info,&allocation_create_info,&buffer,&allocation,&info);
        this->m_size=size;
    }

    char* Buffer::map()
    {
        void* data;
        vmaMapMemory(vkbase::vma, allocation, &data);
        return reinterpret_cast<char*>(data);
    }

    void Buffer::unmap()
    {
        vmaUnmapMemory(vkbase::vma, allocation);
    }

    void Buffer::copy_to_buffer_cmd(VkCommandBuffer cmd, const Buffer& dst_buffer, VkDeviceSize copy_size,
        VkDeviceSize src_offset, VkDeviceSize dst_offset)
    {
        VkBufferCopy copy_region{};
        copy_region.srcOffset=src_offset;
        copy_region.dstOffset=dst_offset;
        if (copy_size)
            copy_region.size=copy_size;
        else
            copy_region.size=this->m_size;

        assert(copy_region.size<=dst_buffer.size());

        vkCmdCopyBuffer(cmd, buffer, dst_buffer.buffer, 1, &copy_region);
    }

    void Buffer::copy_to_buffer(const Buffer& dst_buffer, VkDeviceSize copy_size, VkDeviceSize src_offset,
        VkDeviceSize dst_offset)
    {
        vkbase::OneTimeCommandBufferTransfer cb;
        copy_to_buffer_cmd(cb, dst_buffer, copy_size, src_offset, dst_offset);
        cb.submit();
    }

    VkDeviceSize Buffer::size() const
    {
        return m_size;
    }

    VkBuffer Buffer::get() const
    {
        return  buffer;
    }

    VkMemoryPropertyFlags Buffer::getMemoryPropertyFlags() const
    {
        VkMemoryPropertyFlags memPropFlags;
        vmaGetAllocationMemoryProperties(vkbase::vma, allocation, &memPropFlags);
        return memPropFlags;
    }

    bool Buffer::isDeviceLocal() const
    {
        return getMemoryPropertyFlags()&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    VkMemoryRequirements Buffer::getBufferMemoryRequirements() const
    {
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(vkbase::device,buffer,&requirements);
        return requirements;
    }

    std::string Buffer::print_as_uint32_array(const std::string& name)
    {
        auto count=m_size/sizeof(uint32_t);
        std::string result;
        result+=name+" = { ";
        auto data=reinterpret_cast<uint32_t*>(map());
        for (VkDeviceSize i=0;i<count;++i)
        {
            result+=std::to_string(data[i]);
            if (i!=count-1)
                result+=", ";
        }
        result+=" }";
        unmap();
        return result;
    }

    Buffer::~Buffer()
    {
        if (buffer!=VK_NULL_HANDLE)
            vmaDestroyBuffer(vkbase::vma, buffer, allocation);
    }

    Buffer& Buffer::operator=(Buffer&& other)
    {
        if (buffer!=VK_NULL_HANDLE)
            vmaDestroyBuffer(vkbase::vma, buffer, allocation);

        buffer=other.buffer;
        allocation=other.allocation;
        info=other.info;
        m_size=other.m_size;

        other.buffer=VK_NULL_HANDLE;

        return *this;
    }
}
