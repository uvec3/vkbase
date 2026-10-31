#pragma once


#include <map>
#include <utility>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include "DEFINES.hpp"
#include "SystemBackend.hpp"
#include "EngineParams.hpp"
#include "ParallelTaskManager.hpp"
#include "Event.hpp"

namespace vkbase
{
    struct Config
    {
        uint32_t min_img_in_swapchain=2;
        VkPresentModeKHR preferred_present_mode=VK_PRESENT_MODE_FIFO_KHR;
    };


    enum class MessageType
    {
        Error,
        Warning,
        Info
    };


    class GlobalAutoParallelTaskManager;
    extern GlobalAutoParallelTaskManager globalTaskManager;
    class GlobalAutoParallelTaskManager:protected AutoParallelTaskManager
    {
    public:
        GlobalAutoParallelTaskManager():AutoParallelTaskManager(128,INT32_MAX,false){}

        template<typename TaskFunc, typename FinisherFunc>
        friend void runTask(TaskFunc&& task, FinisherFunc&& taskFinisher);
        friend int destroy();
    };
    struct WaitToken;
    template<typename TaskFunc, typename FinisherFunc>
    void runTask(TaskFunc&& task, FinisherFunc&& taskFinisher)
    {
        globalTaskManager.runTask(std::forward<TaskFunc>(task), std::forward<FinisherFunc>(taskFinisher));
    }


    extern VkInstance instance;
    extern Config config;
    extern vk::Device device;
    extern VkPhysicalDevice physicalDevice;

    extern VmaAllocator vma;
    extern QueueFamilyIndices queueFamilyIndices;
    extern VkQueue graphicsQueue;
    extern VkQueue presentQueue;
    extern VkQueue computeQueue;
    extern VkQueue transferQueue;
    extern VkCommandPool commandPoolResetGraphics;
    extern VkCommandPool commandPoolResetCompute;
    extern VkCommandPool commandPoolResetTransfer;

    extern VkSemaphore graphicsTimelineSemaphore;
    extern VkSemaphore presentTimelineSemaphore;
    extern VkSemaphore computeTimelineSemaphore;
    extern VkSemaphore transferTimelineSemaphore;


    extern uint64_t graphicsTimelineSemaphore_counter;
    extern uint64_t presentTimelineSemaphore_counter;
    extern uint64_t computeTimelineSemaphore_counter;
    extern uint64_t transferTimelineSemaphore_counter;


    extern std::vector<VkFramebuffer> swapChainFramebuffers;
    extern VkExtent2D extent;
    extern double lastFrameTime;

    extern std::thread::id main_thread_id;

    //tmp
    extern VkRenderPass renderPass;
    extern std::vector<VkCommandBuffer> cbMain;
    extern uint32_t imageCount;



    struct WaitToken
    {
        VkSemaphore semaphore=VK_NULL_HANDLE;
        uint64_t value=0;


        bool wait(uint64_t timeout=UINT64_MAX) const
        {
            if (semaphore == VK_NULL_HANDLE)
                return true;
            VkSemaphoreWaitInfo wait_info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
            wait_info.semaphoreCount=1;
            wait_info.pSemaphores=&semaphore;
            wait_info.pValues=&value;

            return vkWaitSemaphores(device, &wait_info,timeout)==VK_SUCCESS;
        }
    };

    struct WaitPoint
    {
        WaitToken wait_token;
        VkPipelineStageFlags2 stage_flags=VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;

        VkSemaphoreSubmitInfo submit_info() const;
    };


    //performs an action on the main thread after the semaphore is signaled
    template<typename FinisherFunc>
    void doAfter(WaitToken wait_token, FinisherFunc&& taskFinisher)
    {
        vkbase::runTask([wait_token]()
        {
            (void)wait_token.wait();
            return std::tuple<>();
        },
        [taskFinisher=std::move(taskFinisher)](std::tuple<>) mutable {taskFinisher();});
    }

    void addDeviseExtension(const char* extension);
    int init(const std::string &appName);
    int destroy();
    void addNextSubmitWait(const WaitPoint& wait_point);
    WaitToken drawFrame();//check swapchain, process all changes, update all data, rendering and presentation
    bool handleEvents();
    void waitForRenderEnd(uint64_t timeout= UINT64_MAX);
    //create shader module from binary code(char array)
    [[maybe_unused]] VkShaderModule createShaderModule(const std::vector<char> &code);
    //create shader module from binary code(uint32_t array)
    [[maybe_unused]] VkShaderModule createShaderModule(const std::vector<uint32_t> &code);
    //create shader module from binary code (string_view)
    VkShaderModule createShaderModule(const std::string_view &code);
    //retrieve device name
    std::string getDeviceName(VkPhysicalDevice physicalDevice);

    uint64_t getCurrentFrame_host();
    uint64_t getLastRenderedFrame_device();

    WaitToken getPreviousFrameWaitToken();
    WaitToken getThisFrameWaitToken();
    void scheduleOnMainThread(const std::function<void()>& callback);



    //called before engine is initialized, use it to add device extensions
    int addBeforeInitCallback(const std::function<void()>& callback, int priority= 0);
    //called after the engine is initialized
    int addInitCallback(const std::function<void()>& callback, int priority= 0);
    //called before destroying of engine
    int addDestroyCallback(const std::function<void()>& callback, int priority= 0);
    //called in idle time between frames(it can be used for heavy computations)
    int addDrawPrepareCallback(const std::function<void(uint32_t)>& callback, int priority= 0);
    //called after rendering of next frame finished and before submit command buffer to queue again
    //it can be used for buffer overwriting
    //if possible it is better to use prepare callback for heavy computations which will be called before waiting for rendering of next frame finished
    //this should be used for updating data which are used in rendering
    int addBeforeSubmitCallback(const std::function<void( uint32_t)>& callback, int priority= 0);
    int addWriteMainBufferCallback(const std::function<void(VkCommandBuffer, uint32_t)>& callback, int priority= 0);
    int addSurfaceChangedHandler(const std::function<void()>& callback, int priority= 0);
    int addMessagesListener(const std::function<void(std::string_view,MessageType)>& listener,int priority=0);
    int addWriteBeforeRenderBufferCallback(const std::function<void(VkCommandBuffer, uint32_t)>& callback, int priority= 0);

    void removeBeforeInitCallback(int id);
    void removeInitCallback(int id);
    void removeDestroyCallback(int id);
    void removeDrawPrepareCallback(int id);
    void removeRewriteBuffersCallback(int id);
    void removeWriteMainBufferCallback(int id);
    void removeWriteBeforeRenderBufferCallback(int id);
    void removeSurfaceChangedHandler(int id);
    void removeMessagesListener(int id);

    void info(const std::string& str);
    void error(const std::string& str);
    void warning(const std::string& str);

    VkCommandBufferInheritanceInfo createMainBufferInheritanceInfo(uint32_t imageIndex);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkMemoryHeapFlags heapFlags);
    double timestampToSeconds(uint64_t timestampCount);
    const VkPhysicalDeviceProperties& getPhysicalDeviceProperties();




    class OneTimeCommandBuffer
    {
    public:
        VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
        OneTimeCommandBuffer(VkQueue& queue, VkCommandPool& commandPool);
        operator VkCommandBuffer();
        bool submit();

    private:
        VkQueue& queue;
        VkCommandPool& commandPool;
    };

    class OneTimeCommandBufferGraphics: public OneTimeCommandBuffer
    {
        public:OneTimeCommandBufferGraphics():OneTimeCommandBuffer(graphicsQueue,commandPoolResetGraphics){}
    };

    class OneTimeCommandBufferCompute: public OneTimeCommandBuffer
    {
        public:OneTimeCommandBufferCompute():OneTimeCommandBuffer(computeQueue,commandPoolResetCompute){}
    };

    class OneTimeCommandBufferTransfer: public OneTimeCommandBuffer
    {
        public:OneTimeCommandBufferTransfer():OneTimeCommandBuffer(transferQueue,commandPoolResetTransfer){}
    };


    template<VkQueue& queue,VkCommandPool& commandPool,VkSemaphore& timelineSemaphore,uint64_t& timelineSemaphoreCounter>
    class CommandBuffer
    {
    public:
        VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
        CommandBuffer()
        {
            if (commandPool!=VK_NULL_HANDLE)
            {
                init();
            }
        }
        operator VkCommandBuffer() const
        {
            return commandBuffer;
        }
        void begin()
        {
            if (!commandBuffer)
                init();

            vkResetCommandBuffer(commandBuffer,0);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            vkBeginCommandBuffer(commandBuffer, &beginInfo);
        }

        void end()
        {
            vkEndCommandBuffer(commandBuffer);
        }

        WaitToken submit_async(const std::vector<WaitPoint>& wait_points = {})
        {
            std::vector<VkSemaphoreSubmitInfo> waitSemaphoreInfos;
            for (auto wait_point:wait_points)
            {
                waitSemaphoreInfos.push_back(wait_point.submit_info());
            }

            VkSemaphoreSubmitInfo signalSemaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
            signalSemaphoreInfo.semaphore=timelineSemaphore;
            signalSemaphoreInfo.value=++timelineSemaphoreCounter;
            signalSemaphoreInfo.stageMask=VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;


            VkCommandBufferSubmitInfo commandBufferSubmitInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
            commandBufferSubmitInfo.commandBuffer=commandBuffer;


            VkSubmitInfo2 submitInfo
            {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .waitSemaphoreInfoCount = static_cast<uint32_t>(waitSemaphoreInfos.size()),
                .pWaitSemaphoreInfos=waitSemaphoreInfos.data(),
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos = &commandBufferSubmitInfo,
                .signalSemaphoreInfoCount = 1,
                .pSignalSemaphoreInfos = &signalSemaphoreInfo,
            };


            //submit command buffer to graphics queue
            if (vkQueueSubmit2(graphicsQueue, 1, &submitInfo, nullptr) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to submit draw command buffer!");
            }

            return WaitToken{timelineSemaphore,timelineSemaphoreCounter};
        }

    private:

        void init()
        {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = commandPool;
            allocInfo.commandBufferCount = 1;

            vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
        }
    };

    template<VkQueue& queue,VkCommandPool& commandPool,VkSemaphore& timelineSemaphore,uint64_t& timelineSemaphoreCounter>
    class DynamicCommandBuffer:public CommandBuffer<queue,commandPool,timelineSemaphore,timelineSemaphoreCounter>
    {
        using Super=CommandBuffer<queue,commandPool,timelineSemaphore,timelineSemaphoreCounter>;

    public:

        DynamicCommandBuffer(std::function<void(VkCommandBuffer)> write_fn):
            write_fn(std::move(write_fn)),
            Super()
        {}

        WaitToken submit_async(const std::vector<WaitPoint>& wait_points = {})
        {
            this->begin();
            write_fn(this->commandBuffer);
            this->end();

            return Super::submit_async(wait_points);
        }

    private:
        std::function<void(VkCommandBuffer)> write_fn;

        //hide begin end
        void begin()
        {
            if (!Super::commandBuffer)
                Super::init();

            vkResetCommandBuffer(Super::commandBuffer,0);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(Super::commandBuffer, &beginInfo);
        }


        using Super::end;
    };

    using CommandBufferGraphics=CommandBuffer<graphicsQueue,commandPoolResetGraphics,graphicsTimelineSemaphore,graphicsTimelineSemaphore_counter>;
    using CommandBufferCompute= CommandBuffer<graphicsQueue,commandPoolResetGraphics,graphicsTimelineSemaphore,graphicsTimelineSemaphore_counter>;
    using CommandBufferTransfer=CommandBuffer<graphicsQueue,commandPoolResetGraphics,graphicsTimelineSemaphore,graphicsTimelineSemaphore_counter>;

    using DynamicCommandBufferGraphics=DynamicCommandBuffer<graphicsQueue,commandPoolResetGraphics,graphicsTimelineSemaphore,graphicsTimelineSemaphore_counter>;
    using DynamicCommandBufferCompute= DynamicCommandBuffer<graphicsQueue,commandPoolResetGraphics,graphicsTimelineSemaphore,graphicsTimelineSemaphore_counter>;
    using DynamicCommandBufferTransfer=DynamicCommandBuffer<graphicsQueue,commandPoolResetGraphics,graphicsTimelineSemaphore,graphicsTimelineSemaphore_counter>;




}
