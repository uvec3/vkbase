#pragma once
#include <optional>
#include <array>
#include <map>
#include <vector>
#include <cstdint>
#include <string>
#include <thread>

#include <vulkan/vulkan.h>



namespace vkbase::compute
{
    class ComputeEng;
    class ComputePipeline;

    class Buffer
    {
    public:
        struct BufferAttributes
        {
            ComputeEng* eng=nullptr;
            //size in bytes
            uint32_t size=0;
            //flags_bit //VK_BUFFER_USAGE_....
            VkBufferUsageFlags usage=0;
            //memory_property_flags //VK_MEMORY_PROPERTY_....
            VkMemoryPropertyFlags memProperties=0;
            //memory_heap_flags //VK_MEMORY_HEAP_....
            VkMemoryHeapFlags heapFlags=0;
        };


        BufferAttributes info;

        //buffer itself
        VkBuffer buffer=VK_NULL_HANDLE;
        //buffer memory
        VkDeviceMemory bufferMemory=VK_NULL_HANDLE;

        void* hostPtr=nullptr;



        //create
        Buffer() = default;
        explicit Buffer(const BufferAttributes& bufferAttributes);
        //create wrapper
        Buffer(ComputeEng *eng, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties, VkMemoryHeapFlags heapFlags = 0) ;

        //open buffer for reading from host and get pointer to memory
        void* map();
        //unmap memory
        void unmap();

        //get pointer to buffer memory
        [[nodiscard]] void* data() const
        {
            return hostPtr;
        }

        //for implicit conversion to VkBuffer
        inline operator VkBuffer()
        {
            return buffer;
        }

        VkCommandBuffer createCommandForCopy(VkBuffer dstBuffer, uint64_t copySize, uint64_t offset=0,uint64_t offsetSrc=0 );

        //copy data to buffer
        bool copyTo(VkBuffer dstBuffer, uint64_t  copySize = 0 ,uint64_t offsetDst = 0,uint64_t offsetSrc =0);
        bool copyToAsync(VkBuffer dstBuffer, uint64_t copySize= 0,uint64_t offsetDst=0,uint64_t offsetSrc=0);

        void free();
    };

    class ComputeEng
    {
    //PUBLIC INTERFACE
    public:
        //constructor
        ComputeEng();
        //destructor
        virtual ~ComputeEng();
        //wait for all operations to complete
        void waitIdle();
        //create host buffer
        Buffer createHostBuffer(uint32_t size, bool uniform=false)
        {
            VkBufferUsageFlags usage = (uniform ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            return {this, size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        }

        Buffer createDeviceBuffer(uint32_t size, bool uniform=false)
        {
            VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            if(uniform)
                usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            return {this, size, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        }


    //FRIENDS
    public:
        friend class Image;
        friend class Buffer;
        friend class ComputePipeline;
        friend class CommandBuffer;

    //TYPES
    private:
        //struct stores queues indices on physical device
        struct QueueFamilyIndices
        {
            //for graphics queue
            std::optional<uint32_t> graphicsFamily;
            //for compute pipelines
            std::optional<uint32_t> computeFamily;

            //check if all queue families are found
            [[nodiscard]] bool isComplete() const
            {
                return graphicsFamily.has_value() && computeFamily.has_value();
            }
        };




    //CONSTANTS, SETTINGS
    protected:
        //assets (constructor will initialize it)
        std::map<std::string, std::string>& assets;

        //required extensions for device
        const std::vector<const char *> deviceExtensions{};
        //required features for device
        const VkPhysicalDeviceFeatures deviceFeatures{};

        //ValidationLayers
        static constexpr std::array<const char*, 1> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
        };

        #ifdef NDEBUG//Release
	        static constexpr bool enableValidationLayers = false;
        #else//DEBUG
            static constexpr bool enableValidationLayers = true;
        #endif


    //VARS
    protected:
        //VK instance
        VkInstance instance{nullptr};
        //debug messenger(response for debug messages callback)
        VkDebugUtilsMessengerEXT debugMessenger{VK_NULL_HANDLE};
        // Physical device handle
        VkPhysicalDevice physicalDevice;
        //logical device
        VkDevice device;
        //queue family indices
        QueueFamilyIndices queueFamilyIndices;
        //graphics queue handle (don't need to destroy as part of logical device)
        VkQueue graphicsQueue{VK_NULL_HANDLE};
        //present queue handle for image output command to the surface (don't need to destroy as part of logical device)
        VkQueue computeQueue{VK_NULL_HANDLE};
        //command pool
        VkCommandPool commandPool{VK_NULL_HANDLE};
        //command pool with support of overwriting single buffer without flush all pool
        VkCommandPool commandPoolReset{VK_NULL_HANDLE};

        //descriptor pool
        VkDescriptorPool descriptorPool;


        //SEMAPHORES (sync GPU & GPU) for each frame
        ////image available for rendering
        //std::vector<VkSemaphore> imageAvailableSemaphores;
        ////rendering finished and image can be presented
        //std::vector<VkSemaphore> renderFinishedSemaphores;

        //FENCES (sync CPU & GPU)
        std::vector<VkFence> fenceRenderFinished;

        std::map<std::string, ComputePipeline> pipelines;

        VkPhysicalDeviceProperties deviceProperties;


    //INTERNAL METHODS
    protected:
        //SETUP VULKAN DEVICES
        //create instance
        void createInstance();
        //find a suitable physical device
        void pickPhysicalDevice();

        //check if physical device supports all requirement s
        bool isDeviseSuitable(VkPhysicalDevice &physDevise);
        //check if physical device supports all required extensions
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        //create logical device
        void createLogicalDevice();
        //find necessary queue families for selected device
        inline QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physDevice);
        //find memory type
        uint32_t findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties,
                                 VkMemoryHeapFlags heapFlags);


        //COMMAND POOLS
        //create command pool
        void createCommandPool();
        //create command pool with support for buffer overwriting without the need to flush the entire pool
        void createCommandPoolReset();
        //create descriptor pool
        void createDescriptorPool();


        //SYNC OBJECTS

        ComputePipeline& getPipeline(const std::string& name);

        //DEBUG
        //extracts pointers on functions for create and destroy DebugUtilsMessengerEXT
        static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                                     const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                     const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
        static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                                  VkDebugUtilsMessengerEXT debugMessenger,
                                                  const VkAllocationCallbacks* pAllocator);
        //check if installed vulkan supported necessary validation layers
        static bool checkValidationLayerSupport();
        //calls for debug messages
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);
        void setupDebugMessenger();
        //  this structure uses twice: instance creation && DebugMessenger
        inline void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);




    //ADDITIONAL METHODS FOR INHERITANCE CLASSES
    protected:
        //retrieve device name
        static std::string getDeviceName(VkPhysicalDevice device);
        VkShaderModule createShaderModule(const std::vector<char> &code);
        VkShaderModule loadPrecompiledShader(const std::string &filename);
    };

    struct CommandBufferCache
    {
        VkDescriptorSet descriptorSet;
        std::vector<VkBuffer> buffers;
        VkPipeline pipeline;

        bool operator==(const CommandBufferCache &other) const
        {
            if(pipeline != other.pipeline)
                return false;
            if (buffers.size() != other.buffers.size())
                return false;
            for (int i = 0; i < buffers.size(); ++i)
                if (buffers[i] != other.buffers[i])
                    return false;
            return true;
        }
    };

    class CommandBuffer
    {
        friend class ComputePipeline;


        std::vector<CommandBufferCache> cache;
        ComputeEng *eng=nullptr;
        VkCommandBuffer buff=VK_NULL_HANDLE;
    public:
        CommandBuffer() = default;
        explicit CommandBuffer(ComputeEng &engine);
        void begin();

        template<typename ...Buffers>
        void operator()(std::string shader, std::array<int, 3> groupSize, Buffers &...buffersArgs);

        void end();
        void submit();
    };

    class ComputePipeline
    {
    public:
        ComputePipeline() = default;
    private:
        friend class CommandBuffer;
        friend class ComputeEng;

        VkPipeline pipeline=VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout=VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout=VK_NULL_HANDLE;

        ComputeEng* eng=nullptr;
        std::vector<VkDescriptorSetLayoutBinding> bindings;


        ComputePipeline(ComputeEng& engine, std::string shaderName);

        //destroy should be called explicitly
        void destroy();

        void bindBuffer(CommandBuffer& cmd, const std::vector<Buffer>& buffers);

        void createBindingsFromSpirv(const std::string& spirvBinCode);
    };



    template<typename... Buffers>
    void CommandBuffer::operator()(std::string shader, std::array<int,3> groupSize, Buffers &... buffersArgs)
    {
        std::vector<Buffer> buffers{buffersArgs...};

        //sync with previous dispatch
        if(!cache.empty())
        {
            vkCmdPipelineBarrier(buff, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
        }

        ComputePipeline &pipeline = eng->getPipeline(shader);

        vkCmdBindPipeline(buff, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);

        pipeline.bindBuffer(*this, buffers);

        if(groupSize[0] <= 0)
            groupSize[0]=1;
        if(groupSize[1] <= 0)
            groupSize[1]=1;
        if(groupSize[2] <= 0)
            groupSize[2]=1;

        //it might be that max group size is smaller than requested, so we need to split it
        VkPhysicalDeviceProperties& vkProperties = eng->deviceProperties;
        int maxGroupSizeX =static_cast<int>(vkProperties.limits.maxComputeWorkGroupSize[0]);
        int maxGroupSizeY = static_cast<int>(vkProperties.limits.maxComputeWorkGroupSize[1]);
        int maxGroupSizeZ = static_cast<int>(vkProperties.limits.maxComputeWorkGroupSize[2]);

        int splitX = (groupSize[0] - 1) / maxGroupSizeX + 1;
        int splitY = (groupSize[1] - 1) / maxGroupSizeY + 1;
        int splitZ = (groupSize[2] - 1) / maxGroupSizeZ + 1;

        int leftX = groupSize[0] - splitX * maxGroupSizeX;
        int leftY = groupSize[1] - splitY * maxGroupSizeY;
        int leftZ = groupSize[2] - splitZ * maxGroupSizeZ;


        if(groupSize[0] > vkProperties.limits.maxComputeWorkGroupSize[0])
        {
            int batchesCount = (groupSize[0] - 1) / vkProperties.limits.maxComputeWorkGroupSize[0] + 1;
            for(int i=0; i < batchesCount; ++i)
                if(i != batchesCount - 1)
                    vkCmdDispatchBase(buff, i*vkProperties.limits.maxComputeWorkGroupSize[0], 0, 0, vkProperties.limits.maxComputeWorkGroupCount[0], groupSize[1], groupSize[2]);
                else
                    vkCmdDispatchBase(buff, i*vkProperties.limits.maxComputeWorkGroupSize[0], 0, 0, groupSize[0] - i * vkProperties.limits.maxComputeWorkGroupCount[0], groupSize[1], groupSize[2]);
        }
        else
            vkCmdDispatch(buff, groupSize[0], groupSize[1], groupSize[2]);
    }

}