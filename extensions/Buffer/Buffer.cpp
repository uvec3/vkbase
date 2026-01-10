#include "Buffer.hpp"
#include <stdexcept>//for runtime_error
#include "vulkan/vulkan.h"

namespace vkbase
{

    void Buffer::create(const BufferAttributes& bufferAttributes)
    {
        if(buffer)
            destroy();


        info = bufferAttributes;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = info.size;
        bufferInfo.usage = info.usage;
        //VK_SHARING_MODE_EXCLUSIVE - buffer will be used by only one queue family at a time
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer!");
        }

        //get memory requirements for the buffer
        //      VkDeviceSize    size;               required size for buffer
        //      VkDeviceSize    alignment;          alignment, in bytes, of the offset within the allocation required for the resource.
        //      uint32_t        memoryTypeBits;     is a bitmask and contains one bit set for every supported memory type for the resource.
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements/*out*/);


        //structure describing the memory allocation parameters
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, info.memProperties, info.heapFlags);

        //allocate memory for the buffer (vkFreeMemory frees memory)
        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate vertex buffer memory");

        //bind buffer to memory
        //offset is the offset from the start of the memory where the buffer should be bound.
        vkBindBufferMemory(device, buffer, bufferMemory, 0);

    }

    void Buffer::create( uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties, VkMemoryHeapFlags heapFlags)
    {
        if(!buffer)
            destroy();
        create(BufferAttributes{size, usage, memProperties, heapFlags});
    }




    void* Buffer::map()
    {
        if(!data)
        {
            //size can be VK_WHOLE_SIZE
            vkMapMemory(device, bufferMemory, 0, VK_WHOLE_SIZE, 0, &data/*ptr on out ptr*/);

        }
        return data;
    }

    void Buffer::unmap()
    {
        //hide the memory
        vkUnmapMemory(device, bufferMemory);
        data=nullptr;
    }

    VkCommandBuffer Buffer::createCommandForCopy(VkBuffer dstBuffer, uint32_t copySize)
    {
        //allocate command buffer
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPoolResetGraphics;
        allocInfo.commandBufferCount = 1;


        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        //Write command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        //VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT - command buffer will be submitted only once (for optimization)
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        //start recording commands
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = copySize;
        vkCmdCopyBuffer(commandBuffer, buffer, dstBuffer, 1, &copyRegion);

        //end recording commands
        vkEndCommandBuffer(commandBuffer);

        return commandBuffer;
    }

    bool Buffer::copyTo(VkBuffer dstBuffer, uint32_t copySize)
    {
        //create command buffer
        VkCommandBuffer commandBuffer = createCommandForCopy(dstBuffer, copySize);

        //submit command buffer to the queue
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        //pointer to an array of command buffers
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        //wait for the queue to finish the command buffer
        vkQueueWaitIdle(graphicsQueue);
        //free COMMAND buffer
        vkFreeCommandBuffers(device, commandPoolResetGraphics, 1, &commandBuffer);
        return true;
    }

    void Buffer::destroy()
    {
        if(buffer)
        {
            vkDestroyBuffer(device, buffer, nullptr);
            vkFreeMemory(device, bufferMemory, nullptr);
            buffer = VK_NULL_HANDLE;
            bufferMemory = VK_NULL_HANDLE;
        }
    }
}