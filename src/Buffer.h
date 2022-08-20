#pragma once

#include<vulkan/vulkan.hpp>
#include <stdexcept>//for runtime_error
#include <vector>
#include <fstream>
#include <memory>
#include "EngineBase.h"

class RTEngine;

namespace vkbase
{
    class Buffer
    {
    public:
        struct BufferAttributes
        {
            EngineBase* eng=nullptr;
            //size in bytes
            uint32_t size;
            //flags_bit //VK_BUFFER_USAGE_....
            VkBufferUsageFlags usage;
            //memory_property_flags //VK_MEMORY_PROPERTY_....
            VkMemoryPropertyFlags memProperties;
            //memory_heap_flags //VK_MEMORY_HEAP_....
            VkMemoryHeapFlags heapFlags;
        };


        BufferAttributes info;

        //buffer itself
        VkBuffer buffer{VK_NULL_HANDLE};
        //buffer memory
        VkDeviceMemory bufferMemory{VK_NULL_HANDLE };



        //Constructor
        Buffer(const BufferAttributes& bufferAttributes);
        //Constructor wrapper
        Buffer(EngineBase *eng, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties, VkMemoryHeapFlags heapFlags = 0) ;

        //open buffer for reading from host and get pointer to memory
        void* map(size_t from, size_t size);
        //unmap memory
        void unMap();

        //for implicit conversion to VkBuffer
        inline operator VkBuffer()
        {
            return buffer;
        }

        VkCommandBuffer createCommandForCopy(VkBuffer dstBuffer, uint32_t copySize);

        //copy data to buffer
        bool copyTo(VkBuffer dstBuffer, uint32_t copySize);

        ~Buffer();
    };

}
