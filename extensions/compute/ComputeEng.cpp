#include <iostream>
#include <stdexcept>//for runtime_error
#include <functional>//for lambda
#include <vector>
#include <array>
#include <optional>//wraps object
#include <set>
#include <cstring>
#include <map>


#include "ComputeEng.h"

extern std::map<std::string, std::string > __comp_engine_assets_var__;

namespace vkbase::compute
{
    ComputeEng::ComputeEng(): assets{__comp_engine_assets_var__}
    {
        queueFamilyIndices = findQueueFamilies(physicalDevice);

        createSyncObjects();
        createCommandPool();
        createCommandPoolReset();

        createDescriptorPool();
    }


    inline void ComputeEng::createCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        //queue which will be used to submit command buffers to
        poolInfo.queueFamilyIndex = queueFamilyIndices.computeFamily.value();
        //possible flags:
        //VK_COMMAND_POOL_CREATE_TRANSIENT_BIT specifies that command buffers allocated from the pool will be short-lived, meaning that they will be reset or freed in a relatively short timeframe. This flag may be used by the implementation to control memory allocation behavior within the pool.
        //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows any command buffer allocated from a pool to be individually reset to the initial state; either by calling vkResetCommandBuffer, or via the implicit reset when calling vkBeginCommandBuffer. If this flag is not set on a pool, then vkResetCommandBuffer must not be called for any command buffer allocated from that pool.
        //VK_COMMAND_POOL_CREATE_PROTECTED_BIT specifies that command buffers allocated from the pool are protected command buffers.
        poolInfo.flags = 0;

        //create poll (command poll will be destroyed in cleanup function)
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool/*out handle*/) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool!");
    }


    void ComputeEng::createCommandPoolReset()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.computeFamily.value();
        //VK_COMMAND_POOL_CREATE_TRANSIENT_BIT specifies that command buffers allocated from the pool will be short-lived, meaning that they will be reset or freed in a relatively short timeframe. This flag may be used by the implementation to control memory allocation behavior within the pool.
        //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows any command buffer allocated from a pool to be individually reset to the initial state; either by calling vkResetCommandBuffer, or via the implicit reset when calling vkBeginCommandBuffer. If this flag is not set on a pool, then vkResetCommandBuffer must not be called for any command buffer allocated from that pool.
        //VK_COMMAND_POOL_CREATE_PROTECTED_BIT specifies that command buffers allocated from the pool are protected command buffers.

        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
                | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        if (vkCreateCommandPool(device, &poolInfo, nullptr,
                                &commandPoolReset/*out handle*/) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool with reset flag!");
    }




    ComputeEng::QueueFamilyIndices ComputeEng::findQueueFamilies(VkPhysicalDevice physDevice)
    {
        //output struct
        QueueFamilyIndices indices;

        //retrieve list of queue families
        uint32_t queueFamilyCount;

        //get count of queue families
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);

        //get array of queue families
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilyProperties.data());

        //iterating and checking properties
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            //check if queue family supports graphics operations
            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphicsFamily = i;

            //check if queue family supports compute operations
            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                indices.computeFamily = i;

            //if all properties are found, break
            if (indices.isComplete())
                break;
        }

        return indices;
    }

    uint32_t ComputeEng::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkMemoryHeapFlags heapFlags)
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

    std::string ComputeEng::getDeviceName(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        std::string vendorName;
        return properties.deviceName+std::string(" (")+std::to_string(properties.deviceID)+std::string(")")+std::string(" (")+std::to_string(properties.vendorID)+std::string(")");
    }

//    [[maybe_unused]] VkShaderModule ComputeEng::loadPrecompiledShader(const std::string &filename)
//    {
//        auto code_str = assets[filename+".spv"];
//        std::vector<char> code(code_str.begin(), code_str.end());
//        if (code.empty())
//        {
//            std::string error = "failed to load precompiled shader file: " + filename + ".spv";
//            throw std::runtime_error(error);
//        }
//            return createShaderModule(code);
//    }

    ComputeEng::~ComputeEng()
    {
        //wait for all queues on device to finish(cause errors otherwise)
        vkDeviceWaitIdle(device);


        //destroy pipelines
        for (auto &pipeline : pipelines)
        {
            pipeline.second.destroy();
        }

        //for (int i = 0; i < imageCount; ++i)
        //{
        //    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        //    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        //    vkDestroyFence(device, fenceRenderFinished[i], nullptr);
        //}

        vkDestroyDescriptorPool(device,descriptorPool, nullptr);


        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyCommandPool(device, commandPoolReset, nullptr);


        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroyInstance(instance, nullptr);
    }



    void ComputeEng::createDescriptorPool()
    {
        VkDescriptorPoolSize pool_sizes[] =
                {
                        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
                };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * sizeof(pool_sizes)/sizeof(pool_sizes[0]);
        pool_info.poolSizeCount = (uint32_t)sizeof(pool_sizes)/sizeof(pool_sizes[0]);
        pool_info.pPoolSizes = pool_sizes;
        if(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool)!=VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor pool");

    }

    void ComputeEng::waitIdle()
    {
        vkDeviceWaitIdle(device);
    }

    ComputePipeline & ComputeEng::getPipeline(const std::string& name)
    {
        if(!pipelines.contains(name))
        {
            pipelines[name] = ComputePipeline(*this, name);
        }

        return pipelines.at(name);
    }


    Buffer::Buffer(const BufferAttributes& bufferAttributes)
    {
        info = bufferAttributes;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = info.size;
        bufferInfo.usage = info.usage;
        //VK_SHARING_MODE_EXCLUSIVE - buffer will be used by only one queue family at a time
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


        if (vkCreateBuffer(info.eng->device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer!");
        }

        //get memory requirements for the buffer
        //      VkDeviceSize    size;               required size for buffer
        //      VkDeviceSize    alignment;          alignment, in bytes, of the offset within the allocation required for the resource.
        //      uint32_t        memoryTypeBits;     is a bitmask and contains one bit set for every supported memory type for the resource.
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(info.eng->device, buffer, &memRequirements/*out*/);


        //structure describing the memory allocation parameters
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = info.eng->findMemoryType( memRequirements.memoryTypeBits, info.memProperties, info.heapFlags);

        //allocate memory for the buffer (vkFreeMemory frees memory)
        if (vkAllocateMemory(info.eng->device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate vertex buffer memory");

        //bind buffer to memory
        //offset is the offset from the start of the memory where the buffer should be bound.
        vkBindBufferMemory(info.eng->device, buffer, bufferMemory, 0);

        if(info.memProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            //map memory to host
            map();
        }

    }

    Buffer::Buffer(ComputeEng *eng, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties, VkMemoryHeapFlags heapFlags):
            Buffer(BufferAttributes{eng, size, usage, memProperties, heapFlags})
    {    }



    void* Buffer::map()
    {
        //size can be VK_WHOLE_SIZE
        vkMapMemory(info.eng->device, bufferMemory, 0, VK_WHOLE_SIZE, 0, &hostPtr/*ptr on out ptr*/);
        return hostPtr;
    }

    void Buffer::unmap()
    {
        //hide the memory
        vkUnmapMemory(info.eng->device, bufferMemory);
    }

    VkCommandBuffer Buffer::createCommandForCopy(VkBuffer dstBuffer, uint64_t copySize, uint64_t offsetDst,uint64_t offsetSrc)
    {
        //allocate command buffer
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = info.eng->commandPoolReset;
        allocInfo.commandBufferCount = 1;


        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(info.eng->device, &allocInfo, &commandBuffer);

        //Write command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        //VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT - command buffer will be submitted only once (for optimization)
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        //start recording commands
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = offsetSrc; // Optional
        copyRegion.dstOffset = offsetDst; // Optional
        copyRegion.size = copySize;
        vkCmdCopyBuffer(commandBuffer, buffer, dstBuffer, 1, &copyRegion);

        //end recording commands
        vkEndCommandBuffer(commandBuffer);

        return commandBuffer;
    }

    bool Buffer::copyTo(VkBuffer dstBuffer, uint64_t copySize,uint64_t offsetDst,uint64_t offsetSrc)
    {
        //create COMMAND buffer
        if(copySize == 0)
            copySize = info.size;

        VkCommandBuffer commandBuffer = createCommandForCopy(dstBuffer, copySize,offsetDst, offsetSrc);

        //submit command buffer to the queue
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        //pointer to an array of command buffers
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(info.eng->computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
        //wait for the queue to finish the command buffer
        vkQueueWaitIdle(info.eng->computeQueue);
        //free COMMAND buffer
        vkFreeCommandBuffers(info.eng->device, info.eng->commandPoolReset, 1, &commandBuffer);

        return true;
    }

    void Buffer::free()
    {
        vkDestroyBuffer(info.eng->device, buffer, nullptr);
        vkFreeMemory(info.eng->device, bufferMemory, nullptr);
    }

    bool Buffer::copyToAsync(VkBuffer dstBuffer, uint64_t copySize, uint64_t offsetDst, uint64_t offsetSrc)
    {
        std::thread t([=,this](){
            copyTo(dstBuffer, copySize, offsetDst, offsetSrc);
        });
        t.detach();

        return true;
    }


    CommandBuffer::CommandBuffer(ComputeEng &engine) : eng{&engine}
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = eng->commandPool;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(eng->device, &allocInfo, &buff) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate command buffers!");

    }

    void CommandBuffer::begin()
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(buff, &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer!");

        for (auto &c : cache)
            vkFreeDescriptorSets(eng->device, eng->descriptorPool, 1, &c.descriptorSet);
        cache.clear();
    }

    void CommandBuffer::end()
    {
        if (vkEndCommandBuffer(buff) != VK_SUCCESS)
            throw std::runtime_error("Failed to record command buffer!");
    }

    void CommandBuffer::submit()
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &buff;

        if (vkQueueSubmit(eng->computeQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
            throw std::runtime_error("Failed to submit draw command buffer!");
    }

//    void ComputePipeline::createBindingsFromSpirv(const std::string &spirvBinCode)
//    {
//        //desasemble spirv
//        std::vector<uint32_t> spirvCode{reinterpret_cast<const uint32_t*>(spirvBinCode.data()), reinterpret_cast<const uint32_t*>(spirvBinCode.data()+spirvBinCode.size())};
//
//        spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_5);
//        spv_text text = nullptr;
//        spv_result_t error =
//                spvBinaryToText(context, spirvCode.data(), spirvCode.size(), SPV_BINARY_TO_TEXT_OPTION_NONE, &text, nullptr);
//        spvContextDestroy(context);
//
//        if (error != SPV_SUCCESS)
//            throw std::runtime_error("Failed to disassemble spirv");
//
//        std::string textStr{text->str, text->length};
//
//        //std::cout<<textStr<<std::endl;
//
//        int pos=0;
//
//        while(pos=textStr.find("Binding", pos), pos!=std::string::npos)
//        {
//            int bind_pos=pos+=7;
//            int binding=std::stoi(textStr.substr(pos, textStr.find('\n', pos)-pos));
//            //go two lines above
//            pos=textStr.rfind('\n', pos);
//            pos=textStr.rfind('\n', pos-1);
//            int left = textStr.rfind(' ', pos-1);
//            std::string type=textStr.substr(left+1, pos-left-1);
//            //std::cout<<binding<<" "<<type<<std::endl;
//
//            VkDescriptorSetLayoutBinding bindingDesc;
//            bindingDesc.binding=binding;
//            bindingDesc.descriptorCount=1;
//            if(type=="BufferBlock")
//                bindingDesc.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//            else if(type=="Block")
//                bindingDesc.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//            else
//                throw std::runtime_error("Unknown descriptor type: "+type);
//            bindingDesc.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;
//            bindingDesc.pImmutableSamplers=nullptr;
//
//            bindings.push_back(bindingDesc);
//
//            pos=bind_pos;
//        }
//
//        //sort bindings
//        std::sort(bindings.begin(), bindings.end(), [](const VkDescriptorSetLayoutBinding& a, const VkDescriptorSetLayoutBinding& b){return a.binding<b.binding;});
//
//    }

    void ComputePipeline::bindBuffer(CommandBuffer &cmd, const std::vector<Buffer> &buffers)
    {
        if(buffers.size() != bindings.size())
            throw std::runtime_error("Shader takes " + std::to_string(bindings.size()) + " arguments, but " + std::to_string(buffers.size()) + " were provided");
        for(int i=0; i < buffers.size(); i++)
        {
            if(bindings[i].descriptorType==VK_DESCRIPTOR_TYPE_STORAGE_BUFFER && !(buffers[i].info.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
                throw std::runtime_error("Argument "+std::to_string(i)+" requires a storage buffer, but a different type was provided");
            if(bindings[i].descriptorType==VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER && !(buffers[i].info.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
                throw std::runtime_error("Argument "+std::to_string(i)+" requires a uniform buffer, but a different type was provided");
        }


        CommandBufferCache currentCacheInfo;
        currentCacheInfo.pipeline=pipeline;
        currentCacheInfo.buffers.resize(buffers.size());
        for(int i=0; i < buffers.size(); i++)
            currentCacheInfo.buffers[i]=buffers[i].buffer;



        for(uint32_t i=0;i<cmd.cache.size(); i++)
            if(cmd.cache[i] == currentCacheInfo)
            {
                vkCmdBindDescriptorSets(cmd.buff, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &cmd.cache[i].descriptorSet, 0, nullptr);
                return;
            }



        //create descriptor set
        VkDescriptorSet descriptorSet;
        VkDescriptorSetAllocateInfo descriptorInfo{};
        descriptorInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorInfo.descriptorPool=eng->descriptorPool;
        descriptorInfo.descriptorSetCount=1;
        descriptorInfo.pSetLayouts=&descriptorSetLayout;
        vkAllocateDescriptorSets(eng->device, &descriptorInfo, &descriptorSet);

        std::vector<VkDescriptorBufferInfo> descriptorBufferInfo(buffers.size());

        for(int i=0; i < buffers.size(); i++)
            descriptorBufferInfo[i]={buffers[i].buffer,0, buffers[i].info.size};


        std::vector<VkWriteDescriptorSet> writeDescriptorSets(buffers.size());

        for(uint32_t i=0; i < buffers.size(); i++)
            writeDescriptorSets[i]={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptorSet, i, 0, 1, bindings[i].descriptorType, nullptr, &descriptorBufferInfo[i], nullptr};


        vkUpdateDescriptorSets(eng->device,writeDescriptorSets.size(),writeDescriptorSets.data(),0, nullptr);

        vkCmdBindDescriptorSets(cmd.buff, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);


        currentCacheInfo.descriptorSet=descriptorSet;
        cmd.cache.push_back(currentCacheInfo);
    }

    void ComputePipeline::destroy()
    {
        if(pipeline!=VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(eng->device, pipelineLayout, nullptr);
            vkDestroyDescriptorSetLayout(eng->device, descriptorSetLayout, nullptr);
            vkDestroyPipeline(eng->device, pipeline, nullptr);
        }
    }

    ComputePipeline::ComputePipeline(ComputeEng &engine, std::string shaderName) :eng{&engine}
    {
        std::string shaderSrcName="shaders_bin/"+shaderName+".comp.spv";
        auto& shaderCode=eng->assets[shaderSrcName];
        createBindingsFromSpirv(shaderCode);

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
        descriptorSetLayoutCreateInfo.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.bindingCount=bindings.size();
        descriptorSetLayoutCreateInfo.pBindings=bindings.data();
        vkCreateDescriptorSetLayout(eng->device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);


        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount=1;
        pipelineLayoutCreateInfo.pSetLayouts=&descriptorSetLayout;
        vkCreatePipelineLayout(eng->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);




        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        pipelineInfo.stage.module = eng->loadPrecompiledShader("shaders_bin/"+shaderName+".comp");
        pipelineInfo.stage.pName = "main";
        pipelineInfo.layout=pipelineLayout;
        pipelineInfo.flags=VK_PIPELINE_CREATE_DISPATCH_BASE;


        if(vkCreateComputePipelines(eng->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline)!=VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline "+shaderName);



        //destroy shader module
        vkDestroyShaderModule(eng->device, pipelineInfo.stage.module, nullptr);

        //get properties
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(eng->physicalDevice, &properties);

    }
}

