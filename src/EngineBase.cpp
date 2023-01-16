#include <iostream>
#include <stdexcept>//for runtime_error
#include <functional>//for lambda
#include <cstdlib>//for macros EXIT
#include <vector>
#include <array>
#include <optional>//wraps object
#include <set>
#if VKBASE_SHADER_RUNTIME_COMPILE
    #include <shaderc/shaderc.hpp>
#endif

#include "EngineBase.h"


namespace vkbase
{
    void EngineBase::init(ISysRes& sysRes)
    {
        #if VKBASE_SHADER_RUNTIME_COMPILE
            std::cout << "Shader runtime compile enabled" << std::endl;
        #endif

        this->sysRes=&sysRes;
        createInstance();
        setupDebugMessenger();
        surface= this->sysRes->createSurface(instance);
        pickPhysicalDevice();
        queueFamilyIndices = findQueueFamilies(physicalDevice);
        createLogicalDevice();
        createCommandPool();
        createCommandPoolRest();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createFrameBuffers();//uses render pass
        createSyncObjects();
        setupLogic();
    }


    inline void EngineBase::createInstance()
    {
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


        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;


        auto extensionsGLWF = sysRes->getRequiredInstanceExtensions();

        if constexpr (enableValidationLayers)
            extensionsGLWF.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);//������=�VK_EXT_debug_utils�

        //������� ���-�� ����������
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsGLWF.size());
        //� ������ � ����
        createInfo.ppEnabledExtensionNames = extensionsGLWF.data();


        //if in Releaze
        if (enableValidationLayers)
        {
            //�������� ���� �� �������
            //count
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            //prt on array
            createInfo.ppEnabledLayerNames = validationLayers.data();

            //��������� �������
            //��������� ��� ����
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
            //���������� ���������
            populateDebugMessengerCreateInfo(debugCreateInfo);
            //�������� ���������� ���������� ������� ��� pNext ��������
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
        } else
        {//����� ������ �� ���������
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }


        //�������� instance ���� ������ �� ������� ��������� ����������
        if (vkCreateInstance(&createInfo/*info*/, nullptr/* ���������������� �������� �������*/,
                             &instance/*���������� �� ������*/) != VK_SUCCESS)
            throw std::runtime_error("Crate insanse failed");


        /*�������� ���������� api vulkan*/
        uint32_t extensionCount;
        //����������� ���������� �� �������� ������ �� ����������
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        //������� ����� ��� ��������� ����������
        auto *extensions = new VkExtensionProperties[extensionCount];
        //����� ����������� ���������� � �� ���������� � ���������� � ������
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);
        //�����
        std::cout << "available extensions:" << std::endl;
        for (uint32_t i = 0; i < extensionCount; ++i)
        {
            sysRes->info(extensions[i].extensionName);
        }
        delete[] extensions;
        /*����������*/
    }

    //�������� ���������� ���������
    inline void EngineBase::pickPhysicalDevice()
    {
        //���������� ���������
        uint32_t deviceCount= 0;
        //������ ���������� ��������� (���� ��� ������)
        vkEnumeratePhysicalDevices(instance/*���������� vulkan*/, &deviceCount/*���-��(_out)*/, nullptr/*������*/);
        //���� ���-�� ��������� �������������� vulkan =0 ��
        //���������� ��� ������, �� ����� �������� ���������� � ��������� ���������
        if (deviceCount == 0)
            throw std::runtime_error("failed to find GPUs with Vulkan support! ");
        //������������� ������� ��� �������� ��������� ���������� ��������� (��)
        std::vector<VkPhysicalDevice> devices(deviceCount);
        //������ ������
        vkEnumeratePhysicalDevices(instance/*���������� vulkan*/, &deviceCount/*���-�� (_out)*/,
                                   devices.data()/*������(_out)*/);

        //������ ���������� �� ���������
        for (VkPhysicalDevice &physDevice: devices)
        {
            //����� ������ ������ ���������� ��� ������� � isDeviseSuitable �������
            if (isDeviseSuitable(physDevice))
            {
                physicalDevice = physDevice;
                return;
            }
        }
        //���� return �� ������ �� ������ �� ���� ���������� �� ������������� ����������
        //� ��� ��������. ��� ����� ������������� ���������� � ��������� �����
        throw std::runtime_error("Physical devise has found, but do not corresponds to the parameters!");
    }

    //�������� �������(phys devise or GP) �� ����������� �������
    inline bool EngineBase::isDeviseSuitable(VkPhysicalDevice &physDevise)
    {
        //���������� ������ ��������� ��� �������� ������� ����������� ����������
        VkPhysicalDeviceProperties deviceProperties;
        //��������� ������� ����������� ����������
        vkGetPhysicalDeviceProperties(physDevise,/*out*/ &deviceProperties);

        //��� �������������� �������������* ������������ ��������� ���������
        VkPhysicalDeviceFeatures deviceFeatures;
        //������ ������������� � �������
        vkGetPhysicalDeviceFeatures(physDevise, /*out*/&deviceFeatures);

        //������ �������� �������� ��������
        QueueFamilyIndices indices = findQueueFamilies(physDevise);

        //�������� ������������ ����� ������
        bool swapChainAdequate = false;
        //���� ��� ���������� �������������� (����� ����� ������� ������)
        //�������� ������������� ���������� � ����������� ��� ������ ����� ������
        if (checkDeviceExtensionSupport(physDevise))
        {
            //����� ���������� ������ ���������
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physDevise);
            //� ���� �������� ���� �� 1 ������ ����������� �  ����� ������������� �� ��� ��
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        //���� ��� ������� �������, ����������� ���������� �������������� � ���� ������ �������� �� ������� ���������� ���������
        return indices.isComplete() && checkDeviceExtensionSupport(physDevise) && swapChainAdequate;
    }

    //�������� ���������� ����������� ���������

    inline bool EngineBase::checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        //������ ���������� � ����������
        uint32_t extensionCount;
        //����������
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        //������
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        //������ � ������
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
        //�������� �� �������
        for (const char *regExtension: deviceExtensions)
        {
            bool found{false};
            for (VkExtensionProperties &foundExtension: availableExtensions)
                if (strcmp(foundExtension.extensionName, regExtension) == 0)
                {
                    found = true;
                    break;
                }
            //���� ��� ����������� �� ������� �� ������� false
            if (!found)
                return false;
        }

        return true;
    }


    //�������� ����������� ��������
    inline void EngineBase::createLogicalDevice()
    {
        //������� ��������� �� ����������� �������� ��������(�������� �� �����)
        std::set<uint32_t> uniqueQueueFamilies{queueFamilyIndices.graphicsFamily.value(),
                                               queueFamilyIndices.presentFamily.value()};

        //������ �������� ������ ��� ������� ��� ����������� ����������(��� ������ ������� ��������� ��������)
        VkDeviceQueueCreateInfo *queueCreateInfos = new VkDeviceQueueCreateInfo[uniqueQueueFamilies.size()]{};
        //��������� ��������(0.0-1.0) ���� ���� ������� ����
        float queuePriority = 1.0f;

        int i{0};
        for (uint32_t queueFamily: uniqueQueueFamilies)
        {
            //��� ���������
            queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            //�������� ������� ��������� ������� �� ���������� ����������
            queueCreateInfos[i].queueFamilyIndex = queueFamily;
            //���������� �������� ����� ���������(����), ������� ���������� �������
            queueCreateInfos[i].queueCount = 1;
            //��������� ������ ����������� ��� ������ �� ��������( ���������� ������� ������ � queueCount)
            queueCreateInfos[i].pQueuePriorities = &queuePriority;
            ++i;
        }

        //����� ������� ��������� ���� �����!!
        VkPhysicalDeviceFeatures deviceFeatures = {/*.shaderFloat64= VK_TRUE*/};


        //������ ����� ����������� ��������� ��������
        VkDeviceCreateInfo createInfo = {};
        //���
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        //���������� �������� ��������
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(uniqueQueueFamilies.size());
        //��������� �� ������ �������� ��� �������
        createInfo.pQueueCreateInfos = queueCreateInfos;
        //��������� �������
        createInfo.pEnabledFeatures = &deviceFeatures;
        //��������� ����������
        //���������� ���������� ��� ����������� ����������
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        //���� ����������
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        //���� �������� ���� ��������
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else
        {
            createInfo.enabledLayerCount = 0;
        }

        auto err=vkCreateDevice(physicalDevice/*���������� ����������� ����������*/,
                                &createInfo/*��������� � ������� � ���������� ����������*/,
                                nullptr/*��������� �������� �������*/,
                                &logicalDevice/*�������� ���������� ����������� ����������*/);
        //�� � ������ ����� ������� ���������� ���������!
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

            }

            throw std::runtime_error("failed to create logical device! "+std::to_string(err));
        }
        //��������� ����������� ����������� ������� ����������� ����������
        vkGetDeviceQueue(logicalDevice, queueFamilyIndices.graphicsFamily.value(),
                         0/*������ �������(���� ������� ������ 1)*/, &graphicsQueue);
        //��������� ����������� ������� ����������� ����������� ����������
        vkGetDeviceQueue(logicalDevice, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
    }

    //�������� �����������




    //������ ��������, ���������� � ���������� ���������� ��� ���� ��������



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

        //��� �������
        std::cout << "Swap chain was recreated\n";
    }


    inline void EngineBase::cleanupSwapChain()
    {
        //delete framebuffers
        for (auto framebuffer: swapChainFramebuffers)
        {
            vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
        }

        //������ ��� ��������� ����������� ����
        for (auto imageView: swapChainImageViews)
        {
            vkDestroyImageView(logicalDevice, imageView, nullptr);
        }
        //����������� ����
        vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
    }


    inline void EngineBase::createSwapChain()
    {
        //������ �������
        SwapChainSupportDetails swapChainDetails = querySwapChainSupport(physicalDevice);
        //�� ���������� ������ ����� ������� �����
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainDetails.formats);
        //������ �����������
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainDetails.presentModes);
        sysRes->info("present mode: "+std::to_string(presentMode));
        //������ �����������
        extent = chooseSwapExtent(swapChainDetails.capabilities);

        //��������� ��������- ��� ����������� ���������� ������ � ������� ������
        /*swapChainDetails.capabilities.minImageCount-����������� ��������� ���������� ������ � ����
        swapChainDetails.capabilities.maxImageCount-������������ ���������� ������ � ����(0 �������� ��� ����������� ���)*/
        //������������ �� 1 ���� ������ ������������ (������ ����� ��� 2)
        uint32_t imageCount = MAX_FRAMES_IN_FLIGHT;
        //�������� �� ������ �� �������� ������������
        if (imageCount <= swapChainDetails.capabilities.minImageCount)
            imageCount = swapChainDetails.capabilities.minImageCount + 1;
        //��������� �� ���������� ���� ��������
        if (swapChainDetails.capabilities.maxImageCount > 0 && imageCount > swapChainDetails.capabilities.maxImageCount)
        {
            imageCount = swapChainDetails.capabilities.maxImageCount;
        }

        //���������� ���������
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        //�����������
        createInfo.surface = surface;
        //���������� �����������
        createInfo.minImageCount = imageCount;
        //������ �����
        createInfo.imageFormat = surfaceFormat.format;
        //�������� ������������
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        //���������� �����������
        createInfo.imageExtent = extent;
        //���������� �����
        createInfo.imageArrayLayers = 1;
        //�������� � ������������(��� ����� ����� ����� ��� �����������)
        /*� VK_IMAGE_USAGE_TRANSFER_SRC_BIT ���������, ��� ����������� ����� �������������� � �������� ��������� ������� ��������.
        � VK_IMAGE_USAGE_TRANSFER_DST_BIT ���������, ��� ����������� ����� �������������� � �������� ������ ���������� ������� ��������.
        � VK_IMAGE_USAGE_SAMPLED_BIT ���������, ��� ����������� ����� ������������ ��� �������� VkImageView,
        ����������� ��� ���������� ����� VkDescriptorSet, ���� VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
        ��� VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, � �����
        � VK_IMAGE_USAGE_STORAGE_BIT ���������,  ��� ����������� ����� ������������ ���
        �������� VkImageView, ����������� ��� ���������� ����� VkDescriptorSet
        ���� VK_DESCRIPTOR_TYPE_STORAGE_IMAGE.
        � VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ���������, ��� ����������� ����� ������������ ��� �������� VkImageView,
        ����������� ��� ������������� � �������� �������� ��� ������������ �������� � VkFramebuffer.
        � VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ���������, ��� ����������� ����� ������������ ���
        �������� VkImageView, ����������� ��� ������������� � �������� �������� ��� ����������� ������� /
        ��������� ��� ������� / ��������� � VkFramebuffer.
        � VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT ���������,
        ��� ������, ����������� � ����� �����������, ����� �������� � ������� VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT
        (��. ������������� ������ ��� ����� ��������� ����������). ���� ��� ����� ���������� ��� ������ �����������,
        ������� ����� ������������ ��� �������� VkImageView, ����������� ��� ������������� � �������� �����, ����������,
        ������� / ��������� ��� �������� ��������.
        � VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT ���������, ��� ����������� ����� ������������ ��� ��������
        VkImageView, ����������� ��� ���������� ����� VkDescriptorSet ���� VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        ���� ����������� �� ������� ��� ������� ��������; � �������������� � �������� �������� �������� � �������� ������.*/
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        //����� �������, ��� ������������ ������ ������� ��������
        /* //�� ����� �������� ����������� � ������� �������� �� ����������� �������, � ����� ���������� �� � ������� �����������
        (��� ������� ��������������� � ������������, ���� ��� ��� �������� ����� � ��� �� �������� �� ��� �������
        �� ���� ��� ������ ������� �������� ����������� ����� ���� ���������� ������ � ����� ����� ������������ ����� ��������������,aa
        ����� ��� ������� ����� ������������ ����������������� � ��������� ����, ��� �� �������� � ������� �� ����� �����
        ������� ������������������ ��� �������� ����� ��� ������������ ������
        ���� ������� ����� ����������������� � ������������ ���� ������)*/
        //������ �������� ��������

        //������ �������� � �����
        uint32_t queueFamilyIndicesArray[] = {queueFamilyIndices.graphicsFamily.value(),
                                              queueFamilyIndices.presentFamily.value()};
        //���� ������� �������� ����� � ��� �� ��������
        if (queueFamilyIndices.graphicsFamily == queueFamilyIndices.presentFamily)
        {
            //����������� ����������� ������ ��������� �������� � ����� ������������ ������ ���� ���� ������������� �������������� � ������ �������

            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            //��� ��� ������� ���� ���� �� ��� ������������� ���������� ������ ����� �������� ����� �������
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }
            //���� ��� ������ �������
        else
        {
            // ����������� ����� �������������� � ���������� ���������� �������� ��� ����� �������� ���� �������������.(������ ������������������)
            //����� ������������ ������� � ����� ���������
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            //� ���� (����������� ������ ���������� ������� ����� ������ ��������� ����� ����������� ����� ������������)
            //���������� ��������
            createInfo.queueFamilyIndexCount = 2;
            //������
            createInfo.pQueueFamilyIndices = queueFamilyIndicesArray;
        }

        //������������� �����������(������� � ��������� �����)
        //���� ��� �� ����� �� �������� ������� ��������������
        createInfo.preTransform = swapChainDetails.capabilities.currentTransform;
//        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR: The way in which the presentation engine treats the alpha component in the images is unknown to the Vulkan API. Instead, the application is responsible for setting the composite alpha blending mode using native window system commands. If the application does not set the blending mode using native window system commands, then a platform-specific default will be used.
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;


        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        //�������� ���� ������(����������� � cleanup)
        if (vkCreateSwapchainKHR(logicalDevice /*���������� ����������*/, &createInfo/*���������� ����*/, nullptr,
                                 &swapChain/*��������� �� ���������� ��� �������� �����������*/) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swap chain!");
        }

        //��������� ������� ������������ ����������� �� ����
        //������ ���������
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
        //������ � ������ swapChainImages
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());

        //���������� ������� � ��������
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;

        //��������� ��������� �� ����������� �������� ������ � �������
        const_cast<int &>(MAX_FRAMES_IN_FLIGHT) = imageCount;
    }

    //������� ������� ��������� ����� ������
    EngineBase::SwapChainSupportDetails EngineBase::querySwapChainSupport(VkPhysicalDevice device)
    {
        //����� ��������� ����� ������
        SwapChainSupportDetails details;

        //������ ����� � ��������� ������������ �����������
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        //������ �������� �����������
        uint32_t formatCount;
        //���-��
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        //���� ���-�� ������� �� ���� 0 �� ������ � ���������� ������
        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        //������ ������� �����������
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }
        //�������
        return details;
    }

    //����� �������

    inline VkSurfaceFormatKHR
    EngineBase::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        //����� ������������ �������
        for (VkSurfaceFormatKHR format: availableFormats)
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return format;
        //����� ��������� ������ ����������
        return availableFormats[0];
    }

    //����� �����������

    inline VkPresentModeKHR
    EngineBase::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    //������(���������� ) ����� ������(������ ������� ������������ ��� ���������� ������������� ����������� ������� ����� �������� �� �����)

    inline VkExtent2D EngineBase::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
    {
        //���� ������ ���������� ������ ���������� ��� ��� ����
        if (capabilities.currentExtent.width != UINT32_MAX)
            return capabilities.currentExtent;

        //� ��������� ������ ��� �������� ��� ����� ��������� ������� (������� �� ������� ����������)
        VkExtent2D actualExtent = {};
        //������� �� (����������� ��������� ������ � �������� ��(����������� ��������� ������ � ������� ���� ))
        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width, static_cast<uint32_t>(sysRes->getWidth())));
        //���� ��� � � ������� ���� ������ ������ ����� ���� � ��� �� ��� � �������� ���� �� ������ ���� �� ����, ���� ������ ��� �� ���
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, static_cast<uint32_t>(sysRes->getHeight())));

        return actualExtent;
    }

    //�������� �������� ��� ��������� �����������

    inline void EngineBase::createImageViews()
    {
        //��������� ������� ������� ���������� ����������� � ����
        swapChainImageViews.resize(swapChainImages.size());
        //��������� �� ����
        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            //��������� � ����� � imageView
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            //�����������
            createInfo.image = swapChainImages[i];
            //���������� ��� 1D ��������, 2D �������, 3D ������� � ���� ����.
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            //��������� ����� ���������� �������
            createInfo.format = swapChainImageFormat;
            //����� �������� ��������������(����������� �� ���������)(!!!�� ��� �������!!!*******************)
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            //�� ������� �� ����������������� 3D-�����������(��� 2 ����)
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            //�������� ������ ��� ��������� ����������� �� ���� ������( ������� � cleanup)
            if (vkCreateImageView(logicalDevice, &createInfo/*���������� ����*/, nullptr,
                                  &swapChainImageViews[i]/*��������� out*/) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

#if DEPTH_BUFFER
    void EngineBase::createDepthResources()
    {
        depthImages.resize(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            depthImages[i].create(
                reinterpret_cast<RTEngine*>( this),
                WIDTH,
                HEIGHT,
                VK_FORMAT_D32_SFLOAT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT| VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT,
                0,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_LAYOUT_UNDEFINED
                );
    }

#endif



    //�������� �������� �������

    inline void EngineBase::createFrameBuffers()
    {

        //������ ��������������� ���������� ������ � ����
        swapChainFramebuffers.resize(MAX_FRAMES_IN_FLIGHT);


        //������ �� ������������ � �������� �������
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            //������������ �����������

            VkImageView attachments[] = {swapChainImageViews[i]
#if DEPTH_BUFFER
                    ,depthImages[i].imageView
#endif
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            //�������� �������� ������� ���������� ������� �������
            framebufferInfo.renderPass = renderPass;
            //���������� ����������� � �������
            framebufferInfo.attachmentCount = sizeof(attachments) / sizeof(VkImageView);
            //����������� � ��������
            framebufferInfo.pAttachments = attachments;
            //������� �����������
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            //���������� ����� �����������
            framebufferInfo.layers = 1;
            //�������� �����������
            if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }

    }

    //�������� ���������� ����

    inline void EngineBase::createCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        //������� � ������� ����� ����� ���������� ��������(� ������ ������ �����������)
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        //����� �������� ��������
        /*���� ��� ��������� ����� ��� ��������� �����:
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: ����� �� ��, ��� ������ ������ ���������������� � ������ ��������� ����� ����� (����� �������� ��������� ��������� ������)
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: ��������� ���������� ������� ������ �� �����������, ��� ����� ����� ��� ��� ������ ���� �������� ������*/
        poolInfo.flags = 0; // Optional

        //������� ���������� ��� (Poll will destroy in cleanup function)
        if (vkCreateCommandPool(logicalDevice/*���������� ����������*/, &poolInfo/*����*/, nullptr,
                                &commandPool/*������ �� ���������� ����*/) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool!");
    }

    //������� ���������� ��� � ���������� ���������� �������� ��� ������������� ������ ����� ����
    void EngineBase::createCommandPoolRest()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        poolInfo.flags =
                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; // Optional
        if (vkCreateCommandPool(logicalDevice/*���������� ����������*/, &poolInfo/*����*/, nullptr,
                                &commandPoolRest/*������ �� ���������� ����*/) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool with rest flag!");
    }


    //�������� ���������

    inline void EngineBase::createSyncObjects()
    {
        //��� ������� ����������� � ���� ������ ���� ����� ��������� � �����
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

        fenceRenderFinished.resize(MAX_FRAMES_IN_FLIGHT);
        fenceImageAvailable.resize(MAX_FRAMES_IN_FLIGHT);

        //��� �������� ��������� ���������� ���������, �� ��� ���� �� ����� ������������ ����������
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;


        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        //���� ������������� �������� ��������� ������ �� ��������� ��� ��������� ������ ������ ���
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            //�������� ���� �������� ��������� ��� ������� ����� � ������
            if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fenceRenderFinished[i]) != VK_SUCCESS||
                vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fenceImageAvailable[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create semaphores!");
            }
        }
    }


    //����� ��������� ����
    void EngineBase::drawFrame()
    {
        static VkResult result;
        //current swap chain image index
        static uint32_t imageIndex;



//        //wait for fence
//        //if fence with index currentFrame is signaled
//        vkWaitForFences(logicalDevice, 1/*count*/, &fenceImageAvailable[currentFrame], VK_TRUE/*����� ��� � �������*/,
//                        UINT64_MAX);
//
//
//        //������� ����� � �������������� ���������(�� �������, � ������ �� �������� ������ vkWaitForFences ���� �� ������������ )
//        vkResetFences(logicalDevice, 1, &fenceImageAvailable[imageIndex]);

        result = vkAcquireNextImageKHR(logicalDevice, swapChain,
                                       UINT64_MAX,
                                       imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
                                       &imageIndex);




        /* VK_ERROR_OUT_OF_DATE_KHR: ������� ������ ����� ������������� � ������������ � ������ �� ����� �������������� ��� ����������.������ ���������� ����� ��������� ������� ����.
        VK_SUBOPTIMAL_KHR : ������� ������ ����� �� - �������� ����� �������������� ��� ��������� ������������� �� �����������, �� �������� ����������� �� �������� �������������.*/
        //��������� ���������� �������� ��������� ����������� �� ����
        if (result == VK_ERROR_OUT_OF_DATE_KHR)//���������� �������� � ������� ����
        {
            recreateSwapChain();
            return;
        }
            //���� VK_SUCCESS �� ����������, ���� �� ������� ����� ���������� �� ���������� ���� ���� � ����������� ���� ����� ��� ��������
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            std::runtime_error("failed to acquire swap chain image!");



        //����� ���������� ������(�������� ����� ����� �������� ��������� ���� �� ������)
        vkWaitForFences(logicalDevice, 1/*count*/, &fenceRenderFinished[imageIndex], VK_TRUE/*����� ��� � �������*/,
                        UINT64_MAX);
        //������� ����� � �������������� ���������(�� �������, � ������ �� �������� ������ vkWaitForFences ���� �� ������������ )
        vkResetFences(logicalDevice, 1, &fenceRenderFinished[imageIndex]);

        //
        drawPrepare();


        ///RENDERING
        //struct for rendering buffer submission
        static VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .commandBufferCount = 1,
        };

        //wait for image available semaphore
        VkSemaphore waitSemaphores[]{imageAvailableSemaphores[currentFrame]};
        submitInfo.pWaitSemaphores = waitSemaphores;

        //command buffer to submit
        submitInfo.pCommandBuffers = &cbRender[imageIndex];

        //������ �������� �� ������� ����� ��������(��� ����� �� ������ ���������� �� ������������ ��������)
        //(���� �� ������ ����������� ���� ����������� �� �������� � ����������� ������ ���������� ���� �� ���������� �������)
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT};

        //�������� ������� ��������� ������
        submitInfo.pWaitDstStageMask = waitStages;

        //���������� ��������� ������� ������������ ����� ���������� ����������
        submitInfo.signalSemaphoreCount = 1;
        //������ ��������� ���������
        //(������� ��������� ����������)
        submitInfo.pSignalSemaphores = &renderFinishedSemaphores[imageIndex];

        //�������� ������� ������(� ����������� ���������) � ����������� �������
        if (vkQueueSubmit(graphicsQueue/*�������*/, 1/*�-�� �������� � ��������*/,
                          &submitInfo/*ptr on array of struct*/, fenceRenderFinished[imageIndex]/*�����*/) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        //��������� ��������� �����������
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        //����� �������� ����� ����� ������� �����������
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphores[imageIndex];
        //���� �� ������� ��������
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        //������ ����������� ������� ���� ����������
        presentInfo.pImageIndices = &imageIndex;
        //������ VkResult ��� ������ ����
        presentInfo.pResults = nullptr; // Optional
        //� �������� ����������� � ������� �����������


        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
            windowResized)//���� �� ��� � ������� �� �����������
        {
            recreateSwapChain();
            windowResized = false;
        } else if (result !=
                   VK_SUCCESS)//���� ��� ��������� � ������������� � ��� ���� �������� ���������� �� ������������ ����������
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        //���������� ���������� ������(���������� CPU ����� ���� ��� ����������� ���� �� ����������)
        //��� �� ������ ������ ��� ��� � ���� ������ ������ ���� ����������� ����� ������������� ������������
        // vkQueueWaitIdle(presentQueue);
        //������� � ���������� ������ ��������� ��� ���������� �����
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    //������������ ������ � ����������� ������

    void EngineBase::cleanup()
    {
        //�������� ���������� �������� ����������� ���������
        //(���� ����� �� ������� � ��������� ���������� ��������� ���������� ������)
        vkDeviceWaitIdle(logicalDevice);

        //��������, ��� ��������� �����������
        //isDrawing = false;
#if DEPTH_BUFFER
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            depthImages[i].destroy();
#endif
        //���������� ��������� ���������(pipelines)
        //destroy();


        //����������� ���� � ������� �������� � ���
        cleanupSwapChain();

        //������� ������ �������
        vkDestroyRenderPass(logicalDevice, renderPass, nullptr);




        //�������� ��������� � �������
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
            //�����
            vkDestroyFence(logicalDevice, fenceRenderFinished[i], nullptr);
            vkDestroyFence(logicalDevice, fenceImageAvailable[i], nullptr);
        }

        //�������� ����������� ����
        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
        vkDestroyCommandPool(logicalDevice, commandPoolRest, nullptr);


        //����������� ����������� ����������
        vkDestroyDevice(logicalDevice, nullptr);
        //���� ���� �������� ���������� �� ����������
        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
        //����������� �����������
        vkDestroySurfaceKHR(instance, surface, nullptr);
        //��������� ����������(���������(instance))
        vkDestroyInstance(instance, nullptr /*�������� ����� ����� �������, ����� �� �����*/);

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


    EngineBase::QueueFamilyIndices EngineBase::findQueueFamilies(VkPhysicalDevice physicalDevice)
    {
        //output struct
        QueueFamilyIndices indices;

        //retrieve list of queue families
        uint32_t queueFamilyCount;

        //get count of queue families
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        //get array of queue families
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

        //iterating and checking properties
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            //check if queue family supports graphics operations
            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphicsFamily = i;

            //check if i-family supports presentation to surface
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

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

#endif
}
