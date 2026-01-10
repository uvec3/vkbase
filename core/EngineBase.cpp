#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include "EngineBase.h"
#include <iostream>
#include <stdexcept>//for runtime_error
#include <functional>//for lambda
#include <vector>
#include <array>
#include <optional>//wraps object
#include <set>
#include <cstring>
#include <map>
#include <chrono>
#include <streambuf>




namespace vkbase
{
    //describe the render pass
    void createRenderPass();
    //SETUP VULKAN DEVICES
    //create instance
    void createInstance();
    //find a suitable physical device
    void pickPhysicalDevice();
    //check if physical device supports all requirements
    bool isDeviseSuitable(VkPhysicalDevice &physDevise);
    //check if physical device supports all required extensions
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    //create logical device
    void createLogicalDevice();
    //find necessary queue families for selected device
    inline QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physDevice);

    void createSwapChain();
    void recreateSwapChain();
    void cleanupSwapChain();
    void createImageViews();
    SwapChainSupportDetails querySwapChainDetails(VkPhysicalDevice physDevice);
    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
    static VkCompositeAlphaFlagBitsKHR chooseSwapChainCompositeAlpha(VkCompositeAlphaFlagsKHR availableAlpha);
    void createFrameBuffers();
    //create command pool with support for buffer overwriting without the need to flush the entire pool
    void createCommandPoolResetGraphics();
    void createCommandPoolResetCompute();
    void createCommandPoolResetTransfer();
    void createMainCommandBuffers();
    void rewriteMainBuffer(uint32_t imageIndex);
    void createTimestampsQueryPool();

    //SYNC OBJECTS
    void createSyncObjects();

    //DEBUG
    //extracts pointers on functions for create and destroy DebugUtilsMessengerEXT
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                          const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                          const VkAllocationCallbacks *pAllocator,
                                          VkDebugUtilsMessengerEXT *pDebugMessenger);

    void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                       VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks *pAllocator);

    //check if installed vulkan supported necessary validation layers
    bool checkValidationLayerSupport();
    //calls for debug messages
    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

    void setupDebugMessenger();

    //  this structure uses twice: instance creation && DebugMessenger
    inline void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

    //EVENTS
    Event<> onInitEvent;
    Event<> onDestroyEvent;
    Event<uint32_t> onDrawPrepareEvent;
    Event<uint32_t> onDeviceSyncEvent;
    Event<VkCommandBuffer, uint32_t> onWriteMainBufferEvent;
    Event<const sys::TouchEvent&> onTouchEvent;
    Event<> onSurfaceChangedEvent;
    Event<std::string_view, MessageType> onMessage;


    //the desired number of frames to be processed simultaneously (corrected when creating a swapchain with restrictions)
    uint32_t imageCount = 1;//{dep swapchain}
    double lastFrameTime;

    glm::vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};//{dep render pass}

    //required extensions for device
    std::vector<const char *> deviceExtensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    //required features for device creation
    VkPhysicalDeviceFeatures deviceFeatures{};
    VkPhysicalDeviceVulkan11Features deviceFeatures11{ };
    VkPhysicalDeviceVulkan12Features deviceFeatures12{ };
    VkPhysicalDeviceVulkan13Features deviceFeatures13{.synchronization2 = VK_TRUE };
    VkPhysicalDeviceVulkan14Features deviceFeatures14{ };

    //ValidationLayers
    static constexpr std::array<const char *, 1> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

    #ifdef NDEBUG//Release
        static constexpr bool enableValidationLayers = false;
    #else//Debug
        static constexpr bool enableValidationLayers = true;
    #endif


    //VARS
    std::string m_appName{"VulkanApp"};
    //VK instance
    VkInstance instance{VK_NULL_HANDLE};
    //debug messenger(response for debug messages callback)
    VkDebugUtilsMessengerEXT debugMessenger{VK_NULL_HANDLE};
    // Physical device handle
    VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
    //logical device
    VkDevice device{VK_NULL_HANDLE};
    //Device properties
    VkPhysicalDeviceProperties physicalDeviceProperties{};
    //queue family indices
    QueueFamilyIndices queueFamilyIndices;
    //graphics queue handle
    VkQueue graphicsQueue{VK_NULL_HANDLE};
    //present queue handle for image output command to the surface
    VkQueue presentQueue{VK_NULL_HANDLE};
    //
    VkQueue computeQueue{VK_NULL_HANDLE};

    VkQueue transferQueue{VK_NULL_HANDLE};
    //surface
    VkSurfaceKHR surface{VK_NULL_HANDLE};
    //swapchain
    VkSwapchainKHR swapChain{VK_NULL_HANDLE};
    //array of image handles for the swapchain
    std::vector<VkImage> swapChainImages;
    //array of framebuffers for each image in the swapchain (attached to render pass)
    std::vector<VkFramebuffer> swapChainFramebuffers;
    //array of image views for each image in the swapchain
    std::vector<VkImageView> swapChainImageViews;
    //output image format
    VkFormat swapChainImageFormat;
    //output image extent
    VkExtent2D extent;
    //Render pass
    VkRenderPass renderPass{VK_NULL_HANDLE};
    //command pool
    VkCommandPool commandPool{VK_NULL_HANDLE};
    //command pool with support of overwriting single buffer without flush all pool
    VkCommandPool commandPoolResetGraphics{VK_NULL_HANDLE};
    VkCommandPool commandPoolResetCompute{VK_NULL_HANDLE};
    VkCommandPool commandPoolResetTransfer{VK_NULL_HANDLE};
    //main command buffer (rewrite every frame) (call other command buffers(cbRender and ui) )
    std::vector<VkCommandBuffer> cbMain;

    //SEMAPHORES (sync GPU & GPU) for each frame
    //image available for rendering
    std::vector<VkSemaphore> imageAvailableSemaphores;
    int currenImageAvailableSemaphore=0;
    //rendering finished and image can be presented
    std::vector<VkSemaphore> renderFinishedSemaphores;

    //timestamps
    VkQueryPool mTimeQueryPool{VK_NULL_HANDLE};

    //FENCES (sync CPU & GPU)
    std::vector<VkFence> fenceRenderFinished;
    //current frame
    uint32_t currentFrame{0};

    bool windowResized{false};
    float timestampPeriod{0.0f};

    //TOUCHES
    sys::EngEvents engEvents;
    TouchData touchData;

    //VMA
    VmaAllocator vma;


    void info_internal(const char* str, int len)
    {
        sys::info(str,len);
        onMessage.call(std::string_view(str,len),MessageType::Info);
    }

    void error_internal(const char* str, int len)
    {
        sys::error(str,len);
        onMessage.call(std::string_view(str,len),MessageType::Error);
    }

    void warning_internal(const char* str, int len)
    {
        sys::warning(str,len);
        onMessage.call(std::string_view(str,len),MessageType::Warning);
    }

    class InfoStream: public std::basic_streambuf<char>
    {
    public:
        int overflow(int c) override
        {
            info_internal(reinterpret_cast<const char *>(&c), 1);
            return c;
        }
    } infoStream;

    class ErrorStream: public std::basic_streambuf<char>
    {
    public:
        int overflow(int c) override
        {
            error_internal(reinterpret_cast<const char *>(&c), 1);
            return c;
        }
    } errorStream;
    
    inline void redirectOut()
    {
        std::cout.rdbuf(&infoStream);
        std::cerr.rdbuf(&errorStream);
        //std::cout.set_rdbuf(&infoStream);
        //std::cerr.set_rdbuf(&errorStream);
    }

    void addDeviseExtension(const char* extension)
    {
        deviceExtensions.push_back(extension);
    }

    inline void init_VMA()
    {
        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.flags = 0;
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_4;
        allocatorCreateInfo.physicalDevice = physicalDevice;
        allocatorCreateInfo.device = device;
        allocatorCreateInfo.instance = instance;

        vmaCreateAllocator(&allocatorCreateInfo, &vma);
    }


    int init(const std::string &appName)
    {
        m_appName = appName;
        redirectOut();//redirect std::cout and std::cerr for handling messages by engine
        sys::init();//init system dependent part
        createInstance();//create vulkan instance
        setupDebugMessenger();//setup debugging for vulkan
        surface= sys::createSurface(instance);//create surface (depends on platform)
        pickPhysicalDevice();//look for suitable physical device
        queueFamilyIndices = findQueueFamilies(physicalDevice);//find necessary queue families
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createFrameBuffers();//uses render pass
        createSyncObjects();
        createCommandPoolResetGraphics();//create command pool with ability to reset single command buffer
        createCommandPoolResetCompute();
        createCommandPoolResetTransfer();
        createTimestampsQueryPool();
        createMainCommandBuffers();//crates main command buffers (rewrite every frame)
        init_VMA();
        onInitEvent.call();//call init events
        return 0;
    }


    inline void createInstance()
    {
        //check if validation layers are available
        if constexpr (enableValidationLayers)
            if(!checkValidationLayerSupport())
                throw std::runtime_error("validation layers requested, but not available!");

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_4;


        //get system instance extensions
        std::vector<const char* > instanceExtensions = sys::getRequiredInstanceExtensions();
        //add debug utils extension (allows callback from vulkan)
        if constexpr (enableValidationLayers)
            instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();

        //validation layers
        if (enableValidationLayers)
        {
            //count
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            //ptr on array
            createInfo.ppEnabledLayerNames = validationLayers.data();
            //debug create info describe what kind of messages to send and to which callback
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
            populateDebugMessengerCreateInfo(debugCreateInfo);

            //debug messenger goes to pNext of instance create info
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
        } else
        {//no validation layers
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        //create instance
        if (vkCreateInstance(&createInfo, nullptr/*callback*/, &instance/*out handle*/) != VK_SUCCESS)
            throw std::runtime_error("Failed to create instance!");


        //enumerate instance extensions
        uint32_t extensionCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);//get size
        auto *extensions = new VkExtensionProperties[extensionCount];//allocate array
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);//get extensions

        std::cout << "available extensions:" << std::endl;
        for (uint32_t i = 0; i < extensionCount; ++i)
        {
            std::cout<< extensions[i].extensionName << std::endl;
        }
        delete[] extensions;
    }

    //find suitable physical device
    inline void pickPhysicalDevice()
    {
        //acquire number of devices(gpus)
        uint32_t deviceCount= 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        //throw error if no devices
        if (deviceCount == 0)
            throw std::runtime_error("failed to find GPUs with Vulkan support! ");

        //acquire available devices
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (VkPhysicalDevice &physDevice: devices) {
            std::cout<< "device: " << getDeviceName(physDevice);
        }

        //find suitable discrete device
        for (VkPhysicalDevice &physDevice: devices)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physDevice, &properties);
            //return first suitable device
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && isDeviseSuitable(physDevice))
            {
                physicalDevice = physDevice;
                return;
            }
        }

        //if no discrete device find any suitable
        for (VkPhysicalDevice &physDevice: devices)
        {
            if (isDeviseSuitable(physDevice))
            {
                physicalDevice = physDevice;
                return;
            }
        }

        //throw error if no suitable device found
        throw std::runtime_error("failed to find a suitable device!");
    }


    inline bool isDeviseSuitable(VkPhysicalDevice &physDevise)
    {
        //acquire device properties(can be used to find the best device)
        //VkPhysicalDeviceProperties deviceProperties;
        //vkGetPhysicalDeviceProperties(physDevise, &deviceProperties);

        //check extensions support (defined in deviceExtensions)
        if (!checkDeviceExtensionSupport(physDevise))
            return false;


        //check if swap chain sufficient
        //get swap chain details for surface with current device
        SwapChainSupportDetails swapChainSupport = querySwapChainDetails(physDevise);
        //at least one image format and one present mode is required
        if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
            return false;


        //find device queue families
        QueueFamilyIndices indices = findQueueFamilies(physDevise);
        if(!indices.isComplete())//if all queue families are found for device
            return false;

        //all checks passed
        return true;
    }


    inline bool checkDeviceExtensionSupport(VkPhysicalDevice dev)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, availableExtensions.data());

        for (const char *requiredExtension: deviceExtensions)
        {
            bool found=false;
            for (VkExtensionProperties &availableExtension: availableExtensions)
                if (strcmp(availableExtension.extensionName, requiredExtension) == 0)
                {
                    found = true;
                    break;
                }

            //if some extension not found
            if (!found)
                return false;
        }
        //all extensions are found
        return true;
    }


    inline void createLogicalDevice()
    {
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
        //required queue families( set does not allow duplicates in case of graphics and present families are the same)
        std::set<uint32_t> uniqueQueueFamilies{queueFamilyIndices.graphicsFamily.value(),
                                               queueFamilyIndices.presentFamily.value(),
                                               queueFamilyIndices.transferFamily.value()};

        //create queue create info for each queue family
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(uniqueQueueFamilies.size());
        //priority of queue (0.0f - 1.0f)
        float queuePriority = 1.0f;

        int i{0};
        for (uint32_t queueFamily: uniqueQueueFamilies)
        {
            queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            //queue family index
            queueCreateInfos[i].queueFamilyIndex = queueFamily;
            //how many queues of this family to create
            queueCreateInfos[i].queueCount = 1;
            //array of priorities for each queue (same size as queueCount)
            queueCreateInfos[i].pQueuePriorities = &queuePriority;

            ++i;
        }


        VkPhysicalDeviceFeatures2 enabled_features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        // Choose the core 1.0 features you want (subset of supported.features)
        enabled_features2.features = deviceFeatures; // or set selectively
        enabled_features2.pNext = &deviceFeatures11;

        deviceFeatures11.sType =VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        deviceFeatures11.pNext = &deviceFeatures12;

        deviceFeatures12.sType =VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        deviceFeatures12.pNext = &deviceFeatures13;

        deviceFeatures13.sType =VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        deviceFeatures13.pNext = &deviceFeatures14;

        deviceFeatures14.sType =VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
        deviceFeatures14.pNext = nullptr;


        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        //queues to create
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(uniqueQueueFamilies.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        //physical device features
        createInfo.pEnabledFeatures = nullptr;
        //using VkPhysicalDeviceFeatures2 instead
        createInfo.pNext = &enabled_features2;
        //physical device extensions
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        //validation layers
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else
        {
            createInfo.enabledLayerCount = 0;
        }

        auto err=vkCreateDevice(physicalDevice,//physical device to create logical device for
                                &createInfo,
                                nullptr,
                                &device/*out handle*/);
        //error handling
        if (err != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device! "+std::to_string(err));
        }

        //get queue handles of logical device
        vkGetDeviceQueue(device,//logical device on which queues are created
                         queueFamilyIndices.graphicsFamily.value(),//queue family index
                         0,//exact queue index in family (0 if only one queue of this family on this device was created)
                         &graphicsQueue//out handle
                         );
        vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
        vkGetDeviceQueue(device, queueFamilyIndices.computeFamily.value(), 0, &computeQueue);
        vkGetDeviceQueue(device, queueFamilyIndices.transferFamily.value(), 0, &transferQueue);

        //get timestamp period

        timestampPeriod = physicalDeviceProperties.limits.timestampPeriod;
    }

    //SETUP SWAP CHAIN
    void recreateSwapChain()
    {
        if(extent.width==0 || extent.height==0)
            return;

        //wait present queue
        vkQueueWaitIdle(presentQueue);

        //cleanup existed swap chain
        cleanupSwapChain();

        //create swap chain again
        createSwapChain();

        createImageViews();

        createFrameBuffers();
        //surface changed events call
        onSurfaceChangedEvent.call();
    }

    inline void cleanupSwapChain()
    {
        if(swapChain!=VK_NULL_HANDLE)
        {
            //delete framebuffers
            for (auto framebuffer: swapChainFramebuffers)
            {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
            }

            //delete image views
            for (auto imageView: swapChainImageViews)
            {
                vkDestroyImageView(device, imageView, nullptr);
            }
            //delete swap chain
            vkDestroySwapchainKHR(device, swapChain, nullptr);
        }
    }

    inline void createSwapChain()
    {
        //get swap chain details (capabilities, formats, present modes which are supported by physical device and surface)
        SwapChainSupportDetails swapChainDetails = querySwapChainDetails(physicalDevice);
        //choose which surface format to use
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainDetails.formats);
        //choose which present mode to use
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainDetails.presentModes);
        std::cout<<"present mode: "<<std::to_string(presentMode)<<'\n';
        //choose extent
        extent = chooseSwapExtent(swapChainDetails.capabilities);
        std::cout<<extent.width<<"x"<<extent.height<<"\n";

        //choose compositeAlpha
        VkCompositeAlphaFlagBitsKHR compositeAlpha=chooseSwapChainCompositeAlpha(swapChainDetails.capabilities.supportedCompositeAlpha);

        //select frames in flight count
        //correct imageCount in respect to min and max available values
        if (imageCount <= swapChainDetails.capabilities.minImageCount)
            imageCount = swapChainDetails.capabilities.minImageCount ;
        // (0 - means no limitation)
        if (swapChainDetails.capabilities.maxImageCount != 0 && imageCount > swapChainDetails.capabilities.maxImageCount)
            imageCount = swapChainDetails.capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        //surface onto which the swapchain will present images
        createInfo.surface = surface;
        // minimum number of presentable images that the application needs
        createInfo.minImageCount = imageCount;
        //selected image format
        createInfo.imageFormat = surfaceFormat.format;
        //selected color space (how to interpret pixel values)
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        //selected extent
        createInfo.imageExtent = extent;
        //how many layers image has
        createInfo.imageArrayLayers = 1;
        //image usage of images in swap chain
        //VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT specifies that the image can be used to create a VkImageView suitable for use as a color or resolve attachment in a VkFramebuffer
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


        uint32_t queueFamilyIndicesArray[] = {queueFamilyIndices.graphicsFamily.value(),
                                              queueFamilyIndices.presentFamily.value()};

        //The swap chain will be used across graphics and present queues
        //if graphics and present queues are different from we need to synchronize access to images in swap chain or use VK_SHARING_MODE_CONCURRENT
        if (queueFamilyIndices.graphicsFamily == queueFamilyIndices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = sizeof(queueFamilyIndicesArray) / sizeof(uint32_t);
            createInfo.pQueueFamilyIndices = queueFamilyIndicesArray;
        }

        //image transformation(rotate and mirror)
        createInfo.preTransform = swapChainDetails.capabilities.currentTransform;
        //VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR: The way in which the presentation engine treats the alpha component in the images is unknown to the Vulkan API. Instead, the application is responsible for setting the composite alpha blending mode using native window system commands. If the application does not set the blending mode using native window system commands, then a platform-specific default will be used.
        createInfo.compositeAlpha = compositeAlpha;
        //how incoming present requests will be processed and queued internally
        createInfo.presentMode = presentMode;
        //indicating the alpha compositing mode to use when this surface is composited together with other surfaces on certain window systems
        createInfo.clipped = VK_TRUE;
        //old swap chain
        createInfo.oldSwapchain = VK_NULL_HANDLE;
        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain/*out handle*/) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swap chain!");
        }

        //get swap chain images size (swap chain can create more than minImageCount images)
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        //get swap chain images
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        //save swap chain details
        swapChainImageFormat = surfaceFormat.format;

        info("image count: " + std::to_string(imageCount));
    }


    SwapChainSupportDetails querySwapChainDetails(VkPhysicalDevice physDevice)
    {
        //what capabilities does the swap chain have with this physical device and surface
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &details.capabilities);

        //get the available formats for the surface with this physical device
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, nullptr);
        if (formatCount != 0)//get the formats if there are any
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, details.formats.data());
        }

        //get available presentation modes for the surface with this physical device
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0)//get if there are any
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, details.presentModes.data());
        }


        return details;
    }



    inline VkSurfaceFormatKHR
    chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (VkSurfaceFormatKHR format: availableFormats)
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return format;
        //if there is no preferred format, return the first one
        return availableFormats[0];
    }


    inline VkPresentModeKHR
    chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
                return availablePresentMode;
            }
        }

        return availablePresentModes[0];
    }


    inline VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
    {
        //when width and height are UINT32_MAX, it means that the surface size will be determined by the extent of a swapchain targeting the surface
        if (capabilities.currentExtent.width != UINT32_MAX)
            return capabilities.currentExtent;

        //we can set extent manually
        VkExtent2D actualExtent = {};
        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width, static_cast<uint32_t>(sys::getWidth())));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, static_cast<uint32_t>(sys::getHeight())));

        return actualExtent;
    }

    VkCompositeAlphaFlagBitsKHR chooseSwapChainCompositeAlpha(VkCompositeAlphaFlagsKHR availableAlpha)
    {
        std::vector<VkCompositeAlphaFlagBitsKHR> preferredAlpha = {
                VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        };

        for(auto preferred : preferredAlpha)
        {
            if(availableAlpha & preferred)
                return preferred;
        }

        throw std::runtime_error("No suitable composite alpha");
    }

    inline void createImageViews()
    {
        //resize for each image
        swapChainImageViews.resize(imageCount);

        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            //remapping the color channels
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            //the set of mipmap levels and array layers to be accessible to the view
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            //create image view
            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]/*out handle*/) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    inline void createFrameBuffers()
    {

        //resize array for each image
        swapChainFramebuffers.resize(imageCount);


        //go through each image
        for (size_t i = 0; i < imageCount; ++i)
        {
            //image attachments
            VkImageView attachments[] = {
                    swapChainImageViews[i]
                    #if DEPTH_BUFFER
                    ,depthImages[i].imageView
                    #endif
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            //framebuffer is created for specific render pass
            framebufferInfo.renderPass = renderPass;
            //attach image view
            framebufferInfo.attachmentCount = sizeof(attachments) / sizeof(VkImageView);
            framebufferInfo.pAttachments = attachments;
            //extent
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            //layers
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPoolResetGraphics()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        //VK_COMMAND_POOL_CREATE_TRANSIENT_BIT specifies that command buffers allocated from the pool will be short-lived, meaning that they will be reset or freed in a relatively short timeframe. This flag may be used by the implementation to control memory allocation behavior within the pool.
        //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows any command buffer allocated from a pool to be individually reset to the initial state; either by calling vkResetCommandBuffer, or via the implicit reset when calling vkBeginCommandBuffer. If this flag is not set on a pool, then vkResetCommandBuffer must not be called for any command buffer allocated from that pool.
        //VK_COMMAND_POOL_CREATE_PROTECTED_BIT specifies that command buffers allocated from the pool are protected command buffers.

        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
                | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPoolResetGraphics) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool with reset flag for graphics!");
    }

    void createCommandPoolResetCompute()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.computeFamily.value();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
                | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPoolResetCompute) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool with reset flag for compute!");
    }

    void createCommandPoolResetTransfer()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily.value();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
                | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPoolResetTransfer) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool with reset flag for transfer!");
    }




    void createMainCommandBuffers()
    {
        //command buffers for each frame
        cbMain.resize(imageCount);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        //command pool from which command buffers will be allocated
        allocInfo.commandPool = commandPoolResetGraphics;
        /*VK_COMMAND_BUFFER_LEVEL_PRIMARY: primary command buffers are the ones that can be submitted to a queue for execution.
        VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be called from the primary command buffers.*/
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        //number of command buffers to allocate
        allocInfo.commandBufferCount = (uint32_t)cbMain.size();

        //create command buffers
        if (vkAllocateCommandBuffers(device, &allocInfo, cbMain.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate main command buffers!");
        }
        vkResetCommandBuffer(cbMain[0], 0);
    }

    void rewriteMainBuffer(uint32_t imageIndex)
    {
        VkCommandBuffer& cb = cbMain[imageIndex];

        vkResetCommandBuffer(cb, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        //         VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT specifies that each recording of the command buffer will only be submitted once, and the command buffer will be reset and recorded again between each submission.
        //         VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT specifies that a secondary command buffer is considered to be entirely inside a render pass. If this is a primary command buffer, then this bit is ignored.
        //         VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT specifies that a command buffer can be resubmitted to a queue while it is in the pending state, and recorded into multiple primary command buffers.
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Optional
        // for secondary buffer only (indicates which state to inherit from calling primary command buffer)
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(cbMain[imageIndex], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording main command buffer!");
        }
        // record commands
        {
            //render pass
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

            renderPassInfo.renderPass = renderPass;

            renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = extent;

            if(clearColor.a !=0.f)
            {
                renderPassInfo.clearValueCount=1;
                VkClearColorValue pClearColor{};
                pClearColor.float32[0]=clearColor.r;
                pClearColor.float32[1]=clearColor.g;
                pClearColor.float32[2]=clearColor.b;
                pClearColor.float32[3]=clearColor.a;

                //VkClearDepthStencilValue clearDepth{};
                //clearDepth.depth = 1.0f;
                //clearDepth.stencil = 0;

                VkClearValue clearValue{};
                clearValue.color=pClearColor;
                renderPassInfo.pClearValues=&clearValue;
            }
            else
            {
                renderPassInfo.clearValueCount=0;
                renderPassInfo.pClearValues=nullptr;
            }

            vkCmdResetQueryPool(cb, mTimeQueryPool, imageIndex*2, 2);
            vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, mTimeQueryPool, imageIndex*2);

            //VK_SUBPASS_CONTENTS_INLINE: Render pass commands will be inlined in the primary command buffer itself, and no secondary command buffers will be executed.
            //VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: Render pass commands will be executed from secondary command buffers.
            vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
            {
                onWriteMainBufferEvent.call(cb, imageIndex);
            }

            vkCmdEndRenderPass(cb);
            vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mTimeQueryPool, imageIndex*2+1);
        }

        if (vkEndCommandBuffer(cb) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }


    void createRenderPass()
    {
        //color attachment
        VkAttachmentDescription colorAttachment{};
        //format of the attachment (swap chain image)
        colorAttachment.format = swapChainImageFormat;
        //color attachment samples (VK_SAMPLE_COUNT_1_BIT: no multisampling)
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        //determine what to do with the data(color and depth) in the attachment before rendering
        if(clearColor.a==0.0f)
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        else
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        //determine what to do with the data(color and depth) in the attachment after rendering
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        //what to do with the stencil data in the attachment before rendering
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        //what to do with the stencil data in the attachment after rendering
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

        //what layout the image will have before the render pass starts (VK_IMAGE_LAYOUT_UNDEFINED: don't care)
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //image layout image will have after the render pass
        //VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
        //VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
        //VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destination for a memory copy operation
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        //array of attachments
        std::vector<VkAttachmentDescription> attachments = {colorAttachment};

        //reference to the color attachment in the array(uses by subpass)
        VkAttachmentReference colorAttachmentRef{};
        //index of the attachment in the array
        colorAttachmentRef.attachment = 0;
        //image layout
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


        //describe subpass (only one now)
        VkSubpassDescription subpass{};
        //what type of pipeline supported by this subpass
        //VK_PIPELINE_BIND_POINT_COMPUTE specifies binding as compute pipeline.
        //VK_PIPELINE_BIND_POINT_GRAPHICS specifies binding as a graphics pipeline.
        //VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR specifies binding as a ray tracing pipeline.
        //VK_PIPELINE_BIND_POINT_SUBPASS_SHADING_HUAWEI specifies binding as a subpass shading pipeline.
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        //attachment references
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        //Other possible attachments that are used by subpass
        //pInputAttachments: Attachments that are read from a shader
        //pResolveAttachments: Attachments used for multisampling color attachments
        //pDepthStencilAttachment: Attachment for depth and stencil data
        //pPreserveAttachments: Attachments that are not used by this subpass, but for which the data must be preserved



        //describe dependencies between subpasses
        VkSubpassDependency dependency{};

        //is the subpass index of the first subpass in the dependency
        //(If srcSubpass is equal to VK_SUBPASS_EXTERNAL, the first synchronization scope includes commands that occur earlier in submission order than the vkCmdBeginRenderPass used to begin the render pass instance)
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;/*beginning*/

        //the subpass index of the second subpass in the dependency
        //(If dstSubpass is equal to VK_SUBPASS_EXTERNAL, the second synchronization scope includes commands that occur later in submission order than the vkCmdEndRenderPass used to end the render pass instance. )
        dependency.dstSubpass = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        //describe render pass
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

        //array of all attachments
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.attachmentCount = attachments.size();

        //subpasses
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        //dependencies between pairs of subpasses
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        //create render pass
        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create render pass!");
        }
    }


    inline void createSyncObjects()
    {
        //resize for each image
        imageAvailableSemaphores.resize(imageCount+1);
        renderFinishedSemaphores.resize(imageCount);
        fenceRenderFinished.resize(imageCount);

        //semaphore creation info( for now has no required parameters)
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;


        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        //create fence in signaled state( so it will not block at first pass)
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        //create all in one loop
        for (int i = 0; i < imageCount; ++i)
        {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &fenceRenderFinished[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create sync objects!");
            }
        }

        for (int i = 0; i < imageAvailableSemaphores.size(); ++i)
        {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create imageAvailableSemaphores objects!");
            }
        }
    }


    void retrieveFrameTime(uint32_t i)
    {
        uint64_t buffer[2];
        VkResult result = vkGetQueryPoolResults(vkbase::device, mTimeQueryPool, i*2, 2, sizeof(uint64_t) * 2, buffer, sizeof(uint64_t),
                                                VK_QUERY_RESULT_64_BIT);
        if (result == VK_SUCCESS)
        {
            lastFrameTime=timestampToSeconds(buffer[1] - buffer[0]);
        }
        else
        {
            lastFrameTime=0;
        }
    }

    void drawFrame()
    {
        //check extent
        auto newExtent=chooseSwapExtent(querySwapChainDetails(physicalDevice).capabilities);
        if(newExtent.width==0 || newExtent.height==0)//if minimized do nothing
            return;
        else if(newExtent.width!=extent.width || newExtent.height!=extent.height)//if extent changed
        {
            recreateSwapChain();
            //return;
        }

        VkResult result;


        //get current swap chain image index
        static uint32_t imageIndex;
        //timeout specifies how long the function waits, in nanoseconds, if no image is available
        //If timeout is zero, then vkAcquireNextImageKHR does not wait, and will either successfully acquire an image, or fail and return VK_NOT_READY if no image is available.
        //If the specified timeout period expires before an image is acquired, vkAcquireNextImageKHR returns VK_TIMEOUT. If timeout is UINT64_MAX, the timeout period is treated as infinite, and vkAcquireNextImageKHR will block until an image is acquired or an error occurs.
        result = vkAcquireNextImageKHR(device, swapChain,
                                       UINT64_MAX,//timeout
                                       imageAvailableSemaphores[currenImageAvailableSemaphore],//semaphore
                                       VK_NULL_HANDLE,//fence
                                       &imageIndex);//out image index



        //VK_SUCCESS is returned if an image became available.
        //VK_SUBOPTIMAL_KHR is returned if an image became available, and the swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully./VK_SUBOPTIMAL_KHR is returned if an image became available, and the swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.
        //VK_NOT_READY is returned if timeout is zero and no image was available.
        //VK_TIMEOUT is returned if timeout is greater than zero and less than UINT64_MAX, and no image became available within the time allowed.

        //If the swapchain images no longer match native surface properties. Applications need to create a new swapchain for the surface to continue presenting if VK_ERROR_OUT_OF_DATE_KHR is returned.
        //VK_ERROR_OUT_OF_DATE_KHR
        //VK_ERROR_OUT_OF_HOST_MEMORY
        //VK_ERROR_OUT_OF_DEVICE_MEMORY
        //VK_ERROR_DEVICE_LOST
        //VK_ERROR_SURFACE_LOST_KHR is returned if the surface becomes no longer available.

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("failed to acquire swap chain image!");

        //call prepare events before waiting for render finished fence for this image
        onDrawPrepareEvent.call(imageIndex);

        //RENDERING
        //struct for rendering buffer submission
        static VkSubmitInfo submitInfo
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO
        };

        //command buffer(s) to submit
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cbMain[imageIndex];
        //wait for image available semaphore
        VkSemaphore waitSemaphores[]{imageAvailableSemaphores[currenImageAvailableSemaphore]};
        //count of semaphores to wait
        submitInfo.waitSemaphoreCount = sizeof(waitSemaphores) / sizeof(VkSemaphore);
        //array of semaphores to wait
        submitInfo.pWaitSemaphores = waitSemaphores;
        //array of corresponding pipeline stages for each semaphore (where to wait)
        //pointer to an array of pipeline stages at which each corresponding semaphore wait will occur
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.pWaitDstStageMask = waitStages;

        //semaphores which will be signaled when the command buffers for this batch have completed execution
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphores[imageIndex];


        //wait for fence to be signaled (previous render finished for this frame) (make cpu wait and don't draw more frames than is in swap chain)
        //called just before vkQueueSubmit so everything is ready for next frame drawing at this point
        vkWaitForFences(device, 1/*count*/, &fenceRenderFinished[imageIndex], VK_TRUE/*wait all in array*/,
                        UINT64_MAX);
        //reset fence (will not pass through vkWaitForFences until it is signaled again)
        vkResetFences(device, 1, &fenceRenderFinished[imageIndex]);

        retrieveFrameTime(imageIndex);


        onDeviceSyncEvent.call(imageIndex);
        rewriteMainBuffer(imageIndex);

        //submit command buffer to graphics queue
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo/*ptr on array of struct*/, fenceRenderFinished[imageIndex]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        //presenting image to the screen
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        //wait for render finished semaphore
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphores[imageIndex];
        //swap chain to present image to
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        //index of image in swap chain to present
        presentInfo.pImageIndices = &imageIndex;
        //result of presentation for each swap chain
        presentInfo.pResults = nullptr; // Optional

        //present image to the screen
        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || windowResized)
        {
            recreateSwapChain();
            windowResized = false;
        }
        else if (result != VK_SUCCESS)//other errors
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        //go to the next semaphore for image available
        currentFrame = (currentFrame + 1) % imageCount;
        currenImageAvailableSemaphore=(currenImageAvailableSemaphore+1)%static_cast<int>(imageAvailableSemaphores.size());
    }


    //DEBUG
    inline VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData
    )
    {
        //write message depending on severity
        switch (messageSeverity)
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                error(pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                warning(pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                info(pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                info(pCallbackData->pMessage);
                break;
            default:
                break;
        }

        return VK_FALSE;
    }


    inline void setupDebugMessenger()
    {
        //if validation layers disabled - do nothing
        if (!enableValidationLayers)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);
        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger/*out handle*/) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to set up debug messenger!");
        }
    }

    void DestroyDebugUtilsMessengerEXT(
            VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator
    )
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }

    }

    VkResult CreateDebugUtilsMessengerEXT(
            VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
            const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger
    )
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    bool checkValidationLayerSupport()
    {
        //get vulkan available layers(currently installed on system)
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);//get size
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());//get data

        //check that all necessary layers are in the list
        for (const char* checkingLayers : validationLayers)
        {
            bool found= false;
            for (uint32_t i=0;i<layerCount;++i)
            {
                if (strcmp(checkingLayers, availableLayers[i].layerName) == 0)
                {
                    found= true;
                    break;
                }
            }
            if (!found)
                return false;
        }
        return true;
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    VkShaderModule createShaderModule(const std::vector<char> &code)
    {
        if(code.size()%4!=0)
            throw std::runtime_error("Incorrect spir-v shader code size: "+std::to_string(code.size()));

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        //cast char array to uint32_t array pointer
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        //creating a module
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    }

    VkShaderModule createShaderModule(const std::vector<uint32_t> &code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size()*4;
        createInfo.pCode = code.data();

        //creating module
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create shader module!");
        }
        return shaderModule;
    }


    [[maybe_unused]] VkShaderModule loadPrecompiledShader(const std::string &filename)
    {
        auto code_str = assets[filename + ".spv"];
        std::vector<char> code(code_str.begin(), code_str.end());
        if (code.empty())
            throw std::runtime_error("failed to load precompiled shader file: " + filename + "spv");
        return createShaderModule(code);
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physDevice)
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

            if (queueFamilyProperties[i].queueFlags&VK_QUEUE_TRANSFER_BIT)
                indices.transferFamily = i;

            //check if i-family supports presentation to surface
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, i, surface, &presentSupport);

            if (presentSupport)
                indices.presentFamily = i;

            //if all properties are found, break
            if (indices.isComplete())
                break;
        }

        return indices;
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkMemoryHeapFlags heapFlags)
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

    std::string getDeviceName(VkPhysicalDevice physicalDevice) {
        VkPhysicalDeviceProperties p;
        vkGetPhysicalDeviceProperties(physicalDevice, &p);
        std::string vendorName;
        return std::format("{} ({}) ({})", p.deviceName, p.deviceID, p.vendorID);
    }


    int destroy()
    {
        //wait for all queues on device to finish(cause errors otherwise)
        vkDeviceWaitIdle(device);

        onDestroyEvent.call();

        vmaDestroyAllocator(vma);

        cleanupSwapChain();

        vkDestroyRenderPass(device, renderPass, nullptr);


        for (auto & imageAvailableSemaphore : imageAvailableSemaphores)
        {
            vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        }

        for (int i = 0; i < imageCount; ++i)
        {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, fenceRenderFinished[i], nullptr);
        }

        vkDestroyQueryPool(vkbase::device, mTimeQueryPool, nullptr);

        vkDestroyCommandPool(device, commandPoolResetGraphics, nullptr);
        vkDestroyCommandPool(device, commandPoolResetCompute, nullptr);
        vkDestroyCommandPool(device, commandPoolResetTransfer, nullptr);

        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        sys::destroy();

        return 0;
    }

    VkCommandBufferInheritanceInfo createMainBufferInheritanceInfo(uint32_t imageIndex)
    {
        //command buffer will be submitted as secondary buffer, so we need to specify inheritance info
        VkCommandBufferInheritanceInfo inheritanceInfo{};
        inheritanceInfo.sType= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.renderPass = renderPass;
        inheritanceInfo.subpass = 0;
        inheritanceInfo.framebuffer = swapChainFramebuffers[imageIndex];
        inheritanceInfo.occlusionQueryEnable = VK_FALSE;
        inheritanceInfo.queryFlags = 0;
        inheritanceInfo.pipelineStatistics = 0;

        return inheritanceInfo;
    }

    int addInitCallback(const std::function<void()> &callback, int priority)
    {
        return onInitEvent.addCallback(callback, priority);
    }

    int addDestroyCallback(const std::function<void()> &callback, int priority)
    {
        return onDestroyEvent.addCallback(callback, priority);
    }

    int addDrawPrepareCallback(const std::function<void(uint32_t)> &callback, int priority)
    {
        return onDrawPrepareEvent.addCallback(callback, priority);
    }

    int addRewriteBuffersCallback(const std::function<void(uint32_t)> &callback, int priority)
    {
        return onDeviceSyncEvent.addCallback(callback, priority);
    }

    int addWriteMainBufferCallback(const std::function<void(VkCommandBuffer, uint32_t)> &callback, int priority)
    {
        return onWriteMainBufferEvent.addCallback(callback, priority);
    }

    int addTouchHandler(const std::function<void(const sys::TouchEvent &)> &callback, int priority)
    {
        return onTouchEvent.addCallback(callback, priority);
    }

    int addSurfaceChangedHandler(const std::function<void()> &callback, int priority)
    {
        return onSurfaceChangedEvent.addCallback(callback, priority);
    }

    int addMessagesListener(const std::function<void(std::string_view, MessageType)> &listener, int priority)
    {
        return onMessage.addCallback(listener, priority);
    }

    void removeInitCallback(int id)
    {
        onInitEvent.removeCallback(id);
    }

    void removeDestroyCallback(int id)
    {
        onDestroyEvent.removeCallback(id);
    }

    void removeDrawPrepareCallback(int id)
    {
        onDrawPrepareEvent.removeCallback(id);
    }

    void removeTouchHandler(int id)
    {
        onTouchEvent.removeCallback(id);
    }

    void removeWriteMainBufferCallback(int id)
    {
        onWriteMainBufferEvent.removeCallback(id);
    }

    void removeSurfaceChangedHandler(int id)
    {
        onSurfaceChangedEvent.removeCallback(id);
    }

    void removeMessagesListener(int id)
    {
        onMessage.removeCallback(id);
    }

    void info(const std::string &str)
    {
        info_internal(str.c_str(),static_cast<int>(str.size()));
        info_internal("\n",1);
    }

    void error(const std::string &str)
    {
        error_internal(str.c_str(), static_cast<int>(str.size()));
        error_internal("\n",1);
    }

    void warning(const std::string &str)
    {
        warning_internal(str.c_str(), static_cast<int>(str.size()));
        warning_internal("\n",1);
    }

    bool handleEvents()
    {
        return sys::handleEvents();
    }

    double timestampToSeconds(uint64_t timestampCount)
    {
        return (double)timestampCount*timestampPeriod / 1000'000'000.0;
    }


    const VkPhysicalDeviceProperties & getPhysicalDeviceProperties()
    {
        return physicalDeviceProperties;
    }

    OneTimeCommandBuffer::OneTimeCommandBuffer(VkQueue queue, VkCommandPool commandPool):queue(queue),commandPool(commandPool)
    {
        assert(commandPool!=VK_NULL_HANDLE && "Command pool is not created!");
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
    }

    OneTimeCommandBuffer::operator struct VkCommandBuffer_T*()
    {
        return commandBuffer;
    }


    bool OneTimeCommandBuffer::submit()
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
            return false;

        vkQueueWaitIdle(queue);
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        return true;
    }


    void createTimestampsQueryPool()
    {
        VkQueryPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        createInfo.pNext = nullptr; // Optional
        createInfo.flags = 0; // Reserved for future use, must be 0!

        createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        createInfo.queryCount = 2*imageCount;

        VkResult result = vkCreateQueryPool(vkbase::device, &createInfo, nullptr, &mTimeQueryPool);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create time query pool!");
        }

        OneTimeCommandBufferGraphics commandBuffer;
        vkCmdResetQueryPool(commandBuffer.commandBuffer, mTimeQueryPool, 0, 2*imageCount);
        commandBuffer.submit();
    }

    void waitForRenderEnd(uint64_t timeout)
    {
        vkWaitForFences(device, fenceRenderFinished.size(), fenceRenderFinished.data(), VK_TRUE, timeout);
    }

    OnDataUpdateReceiver::OnDataUpdateReceiver(): AbstractEventReceiver<uint32_t>{onDrawPrepareEvent, WRAP_MEMBER_FUNC(onUpdateData)}{}

    OnLogicUpdateReceiver::OnLogicUpdateReceiver(): AbstractEventReceiver<uint32_t>{onDeviceSyncEvent, WRAP_MEMBER_FUNC(onUpdateLogic)}{}

    OnSurfaceChangedReceiver::OnSurfaceChangedReceiver() : AbstractEventReceiver<>{onSurfaceChangedEvent, WRAP_MEMBER_FUNC(onSurfaceChanged)}{}

    OnCleanupReceiver::OnCleanupReceiver(): AbstractEventReceiver<>{onDestroyEvent, WRAP_MEMBER_FUNC(onCleanup)}{}
}
