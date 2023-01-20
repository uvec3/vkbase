
#include <iostream>
#include <stdexcept>//for runtime_error
#include <functional>//for lambda
#include <vector>
#include <array>
#include <optional>//wraps object
#include <set>
#include <cstring>

#if VKBASE_SHADER_RUNTIME_COMPILE
    #include <shaderc/shaderc.hpp>
#endif

#include "EngineBase.h"



namespace vkbase
{
    void EngineBase::init(ISysRes& pSysRes)
    {
        #if VKBASE_SHADER_RUNTIME_COMPILE
            std::cout << "Shader runtime compile enabled" << std::endl;
        #endif

        this->sysRes=&pSysRes;
        createInstance();
        setupDebugMessenger();
        surface= this->sysRes->createSurface(instance);
        pickPhysicalDevice();
        queueFamilyIndices = findQueueFamilies(physicalDevice);
        createLogicalDevice();

        createSwapChain();
        createImageViews();
        createRenderPass();
        createFrameBuffers();//uses render pass
        createSyncObjects();
        createCommandPool();
        createCommandPoolReset();
        setupLogic();
    }


    inline void EngineBase::createInstance()
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
        appInfo.apiVersion = VK_API_VERSION_1_2;


        //get system instance extensions
        auto extensionsGLWF = sysRes->getRequiredInstanceExtensions();
        //add debug utils extension(allows callback from vulkan)
        if constexpr (enableValidationLayers)
            extensionsGLWF.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        //application info
        createInfo.pApplicationInfo = &appInfo;
        //instance extensions
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsGLWF.size());
        createInfo.ppEnabledExtensionNames = extensionsGLWF.data();


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
            sysRes->info(extensions[i].extensionName);
        }
        delete[] extensions;
    }

    //find suitable physical device
    inline void EngineBase::pickPhysicalDevice()
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
            sysRes->info("device: " + getDeviceName(physDevice));
        }

        //find suitable device
        for (VkPhysicalDevice &physDevice: devices)
        {
            //get first suitable device
            if (isDeviseSuitable(physDevice))
            {
                physicalDevice = physDevice;
                return;
            }
        }
        //throw error if no suitable device
        throw std::runtime_error("Physical devise has found, but do not corresponds to the parameters!");
    }


    inline bool EngineBase::isDeviseSuitable(VkPhysicalDevice &physDevise)
    {
        //acquire device properties(can be used to find the best device)
//        VkPhysicalDeviceProperties deviceProperties;
//        vkGetPhysicalDeviceProperties(physDevise, &deviceProperties);


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


    inline bool EngineBase::checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        for (const char *requiredExtension: deviceExtensions)
        {
            bool found=false;
            for (VkExtensionProperties &availableExtension: availableExtensions)
                if (strcmp(availableExtension.extensionName, requiredExtension) == 0)
                {
                    found = true;
                    break;
                }

            //if some extension is not found check failed
            if (!found)
                return false;
        }

        //all extensions are found
        return true;
    }



    inline void EngineBase::createLogicalDevice()
    {
        //required queue families( set does not allow duplicates in case of graphics and present families are the same)
        std::set<uint32_t> uniqueQueueFamilies{queueFamilyIndices.graphicsFamily.value(),
                                               queueFamilyIndices.presentFamily.value()};

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



        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        //queues to create
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(uniqueQueueFamilies.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        //physical device features
        createInfo.pEnabledFeatures = &deviceFeatures;
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
                                &logicalDevice/*out handle*/);
        //error handling
        if (err != VK_SUCCESS)
        {
            switch (err)
            {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    sysRes->error("error: vkCreateDevice: VK_ERROR_OUT_OF_HOST_MEMORY");
                    break;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    sysRes->error("error: vkCreateDevice: VK_ERROR_OUT_OF_DEVICE_MEMORY");
                    break;
                case VK_ERROR_INITIALIZATION_FAILED:
                    sysRes->error("error: vkCreateDevice: VK_ERROR_INITIALIZATION_FAILED");
                    break;
                case VK_ERROR_EXTENSION_NOT_PRESENT:
                    sysRes->error("error: vkCreateDevice: VK_ERROR_EXTENSION_NOT_PRESENT");
                    break;
                case VK_ERROR_FEATURE_NOT_PRESENT:
                    sysRes->error("error: vkCreateDevice: VK_ERROR_FEATURE_NOT_PRESENT");
                    break;
                case VK_ERROR_TOO_MANY_OBJECTS:
                    sysRes->error("error: vkCreateDevice: VK_ERROR_TOO_MANY_OBJECTS");
                    break;
                case VK_ERROR_DEVICE_LOST:
                    sysRes->error("error: vkCreateDevice: VK_ERROR_DEVICE_LOST");
                    break;
                default:
                    sysRes->error("error: vkCreateDevice: unknown error");
                    break;
            }

            throw std::runtime_error("failed to create logical device! "+std::to_string(err));
        }

        //get queue handles of logical device
        vkGetDeviceQueue(logicalDevice,//logical device on which queues are created
                         queueFamilyIndices.graphicsFamily.value(),//queue family index
                         0,//exact queue index in family (0 if only one queue of this family on this device was created)
                         &graphicsQueue//out handle
                         );
        vkGetDeviceQueue(logicalDevice, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
    }

    //SETUP SWAP CHAIN
    void EngineBase::recreateSwapChain()
    {
        //frame buffer can not be zero
        if (sysRes->getWidth() == 0 || sysRes->getHeight() == 0)
            return;

        //wait present queue
        vkQueueWaitIdle(presentQueue);

        //cleanup existed swap chain
        cleanupSwapChain();

        //create again
        createSwapChain();


        createImageViews();

        createFrameBuffers();

        //for debug
        std::cout << "Swap chain was recreated\n";
    }


    inline void EngineBase::cleanupSwapChain()
    {
        //delete framebuffers
        for (auto framebuffer: swapChainFramebuffers)
        {
            vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
        }

        //delete image views
        for (auto imageView: swapChainImageViews)
        {
            vkDestroyImageView(logicalDevice, imageView, nullptr);
        }
        //delete swap chain
        vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
    }


    inline void EngineBase::createSwapChain()
    {
        //get swap chain details (capabilities, formats, present modes which are supported by physical device and surface)
        SwapChainSupportDetails swapChainDetails = querySwapChainDetails(physicalDevice);
        //choose which surface format to use
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainDetails.formats);
        //choose which present mode to use
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainDetails.presentModes);
        sysRes->info("present mode: "+std::to_string(presentMode));
        //choose extent
        VkExtent2D extent = chooseSwapExtent(swapChainDetails.capabilities);
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
        if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain/*out handle*/) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swap chain!");
        }

        //get swap chain images size (swap chain can create more than minImageCount images)
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
        //get swap chain images
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());

        //save swap chain details
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;

        sysRes->info("image count: "+std::to_string(imageCount));
    }


    EngineBase::SwapChainSupportDetails EngineBase::querySwapChainDetails(VkPhysicalDevice physDevice)
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
    EngineBase::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (VkSurfaceFormatKHR format: availableFormats)
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return format;
        //if there is no preferred format, return the first one
        return availableFormats[0];
    }


    inline VkPresentModeKHR
    EngineBase::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }


    inline VkExtent2D EngineBase::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
    {
        //when width and height are UINT32_MAX, it means that the surface size will be determined by the extent of a swapchain targeting the surface
        if (capabilities.currentExtent.width != UINT32_MAX)
            return capabilities.currentExtent;

        //we can to set the extent manually
        VkExtent2D actualExtent = {};
        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width, static_cast<uint32_t>(sysRes->getWidth())));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, static_cast<uint32_t>(sysRes->getHeight())));

        return actualExtent;
    }

    VkCompositeAlphaFlagBitsKHR EngineBase::chooseSwapChainCompositeAlpha(VkCompositeAlphaFlagsKHR availableAlpha)
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



    inline void EngineBase::createImageViews()
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
            if (vkCreateImageView(logicalDevice, &createInfo, nullptr,&swapChainImageViews[i]/*out handle*/) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    inline void EngineBase::createFrameBuffers()
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
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            //layers
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }

    }


    inline void EngineBase::createCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        //queue which will be used to submit command buffers to
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        //possible flags:
        //VK_COMMAND_POOL_CREATE_TRANSIENT_BIT specifies that command buffers allocated from the pool will be short-lived, meaning that they will be reset or freed in a relatively short timeframe. This flag may be used by the implementation to control memory allocation behavior within the pool.
        //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows any command buffer allocated from a pool to be individually reset to the initial state; either by calling vkResetCommandBuffer, or via the implicit reset when calling vkBeginCommandBuffer. If this flag is not set on a pool, then vkResetCommandBuffer must not be called for any command buffer allocated from that pool.
        //VK_COMMAND_POOL_CREATE_PROTECTED_BIT specifies that command buffers allocated from the pool are protected command buffers.
        poolInfo.flags = 0;

        //create poll (command poll will be destroyed in cleanup function)
        if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr,&commandPool/*out handle*/) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool!");
    }


    void EngineBase::createCommandPoolReset()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        //VK_COMMAND_POOL_CREATE_TRANSIENT_BIT specifies that command buffers allocated from the pool will be short-lived, meaning that they will be reset or freed in a relatively short timeframe. This flag may be used by the implementation to control memory allocation behavior within the pool.
        //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows any command buffer allocated from a pool to be individually reset to the initial state; either by calling vkResetCommandBuffer, or via the implicit reset when calling vkBeginCommandBuffer. If this flag is not set on a pool, then vkResetCommandBuffer must not be called for any command buffer allocated from that pool.
        //VK_COMMAND_POOL_CREATE_PROTECTED_BIT specifies that command buffers allocated from the pool are protected command buffers.

        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
                | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr,
                                &commandPoolReset/*out handle*/) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool with reset flag!");
    }



    inline void EngineBase::createSyncObjects()
    {
        //resize for each image
        imageAvailableSemaphores.resize(imageCount);
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
            if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fenceRenderFinished[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create sync objects!");
            }
        }
    }



    void EngineBase::drawFrame()
    {
        static VkResult result;
        //current swap chain image index
        static uint32_t imageIndex;


        //timeout specifies how long the function waits, in nanoseconds, if no image is available
        //If timeout is zero, then vkAcquireNextImageKHR does not wait, and will either successfully acquire an image, or fail and return VK_NOT_READY if no image is available.
        //If the specified timeout period expires before an image is acquired, vkAcquireNextImageKHR returns VK_TIMEOUT. If timeout is UINT64_MAX, the timeout period is treated as infinite, and vkAcquireNextImageKHR will block until an image is acquired or an error occurs.
        result = vkAcquireNextImageKHR(logicalDevice, swapChain,
                                       UINT64_MAX,//timeout
                                       imageAvailableSemaphores[currentFrame],//semaphore
                                       VK_NULL_HANDLE,//fence
                                       &imageIndex);//out image index

        sysRes->info("imageIndex = " + std::to_string(imageIndex));

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


        //virtual void for additional preparation in inherited classes
        drawPrepare();


        //RENDERING
        //struct for rendering buffer submission
        static VkSubmitInfo submitInfo
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO
        };

        //command buffer(s) to submit
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cbRender[imageIndex];

        //wait for image available semaphore
        VkSemaphore waitSemaphores[]{imageAvailableSemaphores[currentFrame]};
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
        vkWaitForFences(logicalDevice, 1/*count*/, &fenceRenderFinished[imageIndex], VK_TRUE/*wait all in array*/,
                        UINT64_MAX);
        //reset fence (will not pass through vkWaitForFences until it is signaled again)
        vkResetFences(logicalDevice, 1, &fenceRenderFinished[imageIndex]);

        //submit command buffer to graphics queue
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo/*ptr on array of struct*/, fenceRenderFinished[imageIndex]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        //presenting image to screen
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

        //present image to screen
        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||windowResized)
        {
            recreateSwapChain();
            windowResized = false;
        }
        else if (result != VK_SUCCESS)//other errors
        {
            throw std::runtime_error("failed to present swap chain image!");
        }


        //go to next semaphore for image available
        currentFrame = (currentFrame + 1) % imageCount;
    }



    void EngineBase::cleanup()
    {
        //wait for all queues on device to finish(cause errors otherwise)
        vkDeviceWaitIdle(logicalDevice);

#if DEPTH_BUFFER
        for (int i = 0; i < imageCount; ++i)
            depthImages[i].destroy();
#endif

        cleanupSwapChain();

        vkDestroyRenderPass(logicalDevice, renderPass, nullptr);


        for (int i = 0; i < imageCount; ++i)
        {
            vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(logicalDevice, fenceRenderFinished[i], nullptr);
        }


        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
        vkDestroyCommandPool(logicalDevice, commandPoolReset, nullptr);


        vkDestroyDevice(logicalDevice, nullptr);

        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
    }

    //DEBUG
    inline VKAPI_ATTR VkBool32 VKAPI_CALL
    EngineBase::debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData
    )
    {
        //extract this(function is static)
        auto thiz = reinterpret_cast<EngineBase *>(pUserData);

        //write message depending on severity
        switch (messageSeverity)
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                thiz->sysRes->error(pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                thiz->sysRes->warning(pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                thiz->sysRes->info(pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                thiz->sysRes->info(pCallbackData->pMessage);
                break;
            default:
                break;
        }

        return VK_FALSE;
    }


    inline void EngineBase::setupDebugMessenger()
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

    void EngineBase::DestroyDebugUtilsMessengerEXT(
            VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator
    )
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }

    }

    VkResult EngineBase::CreateDebugUtilsMessengerEXT(
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

    bool EngineBase::checkValidationLayerSupport()
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

    void EngineBase::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData=this;
    }

    VkShaderModule EngineBase::createShaderModule(const std::vector<char> &code)
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
        if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    }

    [[maybe_unused]] VkShaderModule EngineBase::loadPrecompiledShader(const std::string &filename)
    {
        std::vector<char> code=sysRes->load_file((filename+".spv").c_str());
        if (code.empty())
            throw std::runtime_error("failed to load precompiled shader file: "+filename+"spv");
        return createShaderModule(code);

    }


#if VKBASE_SHADER_RUNTIME_COMPILE
    VkShaderModule EngineBase::loadSourceShader(const std::string &filename, bool save)
    {
        std::vector<char> code=sysRes->load_file((filename).c_str());
        if (code.empty())
            throw std::runtime_error("failed to load source shader file: "+filename);

        std::vector<char> compiledCode=compileShader(code,filename);

        //save compiled shader

        return createShaderModule(compiledCode);
    }

    VkShaderModule EngineBase::loadShader(const std::string &filename)
    {
        return VK_NULL_HANDLE;
    }

    std::vector<char> EngineBase::compileShader(const std::vector<char>& source, const std::string &fileName)
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        //decide shader type by extension
        shaderc_shader_kind kind;
        std::string extension= fileName.substr(fileName.find_last_of('.') + 1);
        if (extension=="vert")
            kind=shaderc_shader_kind::shaderc_glsl_vertex_shader;
        else if (extension=="frag")
            kind=shaderc_shader_kind::shaderc_glsl_fragment_shader;
        else if (extension=="comp")
            kind=shaderc_shader_kind::shaderc_glsl_compute_shader;
        else if (extension=="tesc")
            kind=shaderc_shader_kind::shaderc_glsl_tess_control_shader;
        else if (extension=="tese")
            kind=shaderc_shader_kind::shaderc_glsl_tess_evaluation_shader;
        else if (extension=="geom")
            kind=shaderc_shader_kind::shaderc_glsl_geometry_shader;
        else if (extension=="rgen")
            kind=shaderc_shader_kind::shaderc_glsl_raygen_shader;
        else if (extension=="rint")
            kind=shaderc_shader_kind::shaderc_glsl_intersection_shader;
        else if (extension=="rahit")
            kind=shaderc_shader_kind::shaderc_glsl_anyhit_shader;
        else if (extension=="rchit")
            kind=shaderc_shader_kind::shaderc_glsl_closesthit_shader;
        else if (extension=="rmiss")
            kind=shaderc_shader_kind::shaderc_glsl_miss_shader;
        else if (extension=="rcall")
            kind=shaderc_shader_kind::shaderc_glsl_callable_shader;
        else if (extension=="mesh")
            kind=shaderc_shader_kind::shaderc_glsl_mesh_shader;
        else if (extension=="task")
            kind=shaderc_shader_kind::shaderc_glsl_task_shader;
        else if (extension=="spv")
            kind=shaderc_shader_kind::shaderc_spirv_assembly;
        else
            throw std::runtime_error("Unknown shader extension: "+extension);



        // Like -DMY_DEFINE=1
        //options.AddMacroDefinition( "MY_DEFINE", "1" );
        options.SetOptimizationLevel( shaderc_optimization_level_size );


        shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv( source.data() ,source.size(), kind, fileName.c_str(), options);

        if( module.GetCompilationStatus() != shaderc_compilation_status_success ) {
            throw std::runtime_error(module.GetErrorMessage());
        }

        //get compiled code as byte vector
        return std::vector<char>{reinterpret_cast<const char*>( module.cbegin()), reinterpret_cast<const char*>( module.cend()) };
    }
#endif

    EngineBase::QueueFamilyIndices EngineBase::findQueueFamilies(VkPhysicalDevice physDevice)
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

    uint32_t EngineBase::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkMemoryHeapFlags heapFlags)
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

    std::string EngineBase::getDeviceName(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        return properties.deviceName;
    }
}

