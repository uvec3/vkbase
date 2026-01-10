#pragma once

#include <optional>
#include <array>
#include <map>
#include <vector>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "glm/glm/vec2.hpp"
#include "SystemBeckend.hpp"
#include "EngineParams.hpp"
#include "Event.hpp"
#include "DEFINES.hpp"

namespace vkbase
{
    class OnDataUpdateReceiver: public AbstractEventReceiver<uint32_t>
    {
    public:
        OnDataUpdateReceiver();
    protected:
        virtual void onUpdateData(uint32_t imageIndex)=0;
    };

    class OnLogicUpdateReceiver: public AbstractEventReceiver<uint32_t>
    {
    public:
        OnLogicUpdateReceiver();
    protected:
        virtual void onUpdateLogic(uint32_t imageIndex)=0;
    };

    class OnSurfaceChangedReceiver: public AbstractEventReceiver<>
    {
    public:
        OnSurfaceChangedReceiver();
    protected:
        virtual void onSurfaceChanged()=0;
    };

    class OnCleanupReceiver: public AbstractEventReceiver<>
    {
    public:
        OnCleanupReceiver();
    protected:
        virtual void onCleanup()=0;
    };

    enum class MessageType
    {
        Error,
        Warning,
        Info
    };

    extern std::map<std::string, std::string> assets;
    extern VkInstance instance;

    extern VkDevice device;
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


    extern std::vector<VkFramebuffer> swapChainFramebuffers;
    extern VkExtent2D extent;
    extern double lastFrameTime;

    //tmp
    extern VkRenderPass renderPass;
    extern std::vector<VkCommandBuffer> cbMain;
    extern uint32_t imageCount;

    void addDeviseExtension(const char* extension);
    int init(const std::string &appName);
    int destroy();
    void drawFrame();//check swapchain, process all changes, update all data, rendering and presentation
    bool handleEvents();
    void waitForRenderEnd(uint64_t timeout= UINT64_MAX);
    //create shader module from binary code(char array)
    [[maybe_unused]] VkShaderModule createShaderModule(const std::vector<char> &code);
    //create shader module from binary code(uint32_t array)
    [[maybe_unused]] VkShaderModule createShaderModule(const std::vector<uint32_t> &code);
    //read shader file using ISysRes interface
    [[maybe_unused]] VkShaderModule loadPrecompiledShader(const std::string &filename);
    //retrieve device name
    std::string getDeviceName(VkPhysicalDevice physicalDevice);
    int addInitCallback(const std::function<void()>& callback, int priority= 0);
    //called before destroying of engine
    int addDestroyCallback(const std::function<void()>& callback, int priority= 0);
    //called in idle time between frames(it can be used for heavy computations)
    int addDrawPrepareCallback(const std::function<void(uint32_t)>& callback, int priority= 0);
    //called after rendering of next frame finished and before submit command buffer to queue again
    //it can be used for buffer overwriting
    //if possible it is better to use prepare callback for heavy computations which will be called before waiting for rendering of next frame finished
    //this should be used for updating data which are used in rendering
    int addRewriteBuffersCallback(const std::function<void( uint32_t)>& callback, int priority= 0);
    int addWriteMainBufferCallback(const std::function<void(VkCommandBuffer, uint32_t)>& callback, int priority= 0);
    int addTouchHandler(const std::function<void(const sys::TouchEvent&)>& callback, int priority= 0);
    int addSurfaceChangedHandler(const std::function<void()>& callback, int priority= 0);
    int addMessagesListener(const std::function<void(std::string_view,MessageType)>& listener,int priority=0);
    void removeInitCallback(int id);
    void removeDestroyCallback(int id);
    void removeDrawPrepareCallback(int id);
    void removeRewriteBuffersCallback(int id);
    void removeWriteMainBufferCallback(int id);
    void removeTouchHandler(int id);
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
        OneTimeCommandBuffer(VkQueue queue, VkCommandPool commandPool);
        operator VkCommandBuffer();
        bool submit();

    private:
        VkQueue queue;
        VkCommandPool commandPool;
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

}