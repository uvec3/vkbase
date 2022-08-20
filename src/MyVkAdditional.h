#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <fstream>
#include <optional>

namespace vkbase
{
    struct QueueFamilyIndices
    {
        //for graphics queue
        std::optional<uint32_t> graphicsFamily;
        //for presenting
        std::optional<uint32_t> presentFamily;
        //for compute pipelines
        std::optional<uint32_t> computeFamily;

        //check if all queue families are found
        bool isComlete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };


    inline QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
    {


        //output struct
        QueueFamilyIndices indices;

        //retrieve list of queue families

        uint32_t queueFamilyCount;
        //get count of queue families
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        //get array of queue families
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.data());

        //iterating and checking properties
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            //check if queue family supports graphics operations
            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphicsFamily = i;

            //check if family i supports presentation to surface
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport)
                indices.presentFamily = i;

            //if all properties are found, break
            if (indices.isComlete())
                break;
        }


        return indices;
    }


    //    binding= binding in shader
    //    type=VK_DESCRIPTOR_TYPE_...
    //    flags=VK_SHADER_STAGE_...
    inline VkDescriptorSetLayoutBinding
    descriptorSetLayoutBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags flags)
    {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = type;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = flags;
        layoutBinding.pImmutableSamplers = nullptr;

        return layoutBinding;
    };


    inline VkWriteDescriptorSet writeDescriptorSetBuffer(
            const VkDescriptorBufferInfo *bufferInfo, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType,
            uint32_t binding
    )
    {

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;

        descriptorWrite.descriptorType = descriptorType;
        descriptorWrite.descriptorCount = 1;

        descriptorWrite.pBufferInfo = bufferInfo;
        descriptorWrite.pImageInfo = nullptr; // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional

        return descriptorWrite;
    }


    //create shader module from binary code
    inline VkShaderModule createShaderModule(const std::vector<char> &code, VkDevice logicalDevice)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        //cast char array to uint32_t array pointer
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        //creating a module
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    }


    inline uint32_t findMemoryType(
            VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties,
            VkMemoryHeapFlags heapFlags
    )
    {
        //query available types memory on the device (gets an array of memory types and heaps)
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties/*out*/);


        //typeFilter bits in order from the end means under what indices the memory can be selected
        //the memory type must support all properties and belong to a heap with supported flags
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1 << i))&& (memProperties.memoryTypes[i].propertyFlags & properties) == properties &&
                (memProperties.memoryHeaps[memProperties.memoryTypes[i].heapIndex].flags && heapFlags) == heapFlags)
                return i;
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }
}