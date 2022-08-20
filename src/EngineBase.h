#pragma once

#include <vulkan/vulkan.hpp>
#include <iostream>
#include <stdexcept>//for runtime_error
#include <functional>//for lambda
#include <cstdlib>//for macros EXIT
#include <vector>
#include <array>
#include <optional>//wraps object
#include <set>
#include <algorithm>//for min & max
#include <fstream>
#include <thread>
#include <chrono>
#define _USE_MATH_DEFINES
#include <math.h>
#include "VulkanValidationLayer.h"//my class for check validation layer
#include "MyVkAdditional.h"
#include "Image.h"
#include "ISysRes.h"


namespace vkbase
{
    class EngineBase
    {
    public:
//TYPES
        //struct for storing info about swapchain
        struct SwapChainSupportDetails
        {
            //Basic features of the surface (minimum / maximum number of images in the exchange chain,
            //min/max width and height of images)
            VkSurfaceCapabilitiesKHR capabilities;
            // Surface formats (pixel format, color space)
            std::vector<VkSurfaceFormatKHR> formats;
            // Available presentation modes
            std::vector<VkPresentModeKHR> presentModes;
        };

//CONSTANTS
        //required extensions for device
        const std::vector<const char *> deviceExtensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        //window size
        int WIDTH = 1000;
        int HEIGHT = 500;

        //the desired number of frames to be processed simultaneously (corrected when creating a swapchain with restrictions)
        const int MAX_FRAMES_IN_FLIGHT = 1;


//VARS
        //VK instance
        vk::Instance instance{nullptr};
        //debug messenger
        VkDebugUtilsMessengerEXT debugMessenger{VK_NULL_HANDLE};
        // Physical device handle
        // physical device handle will be implicitly destroyed
        //in vkDestroyInstance
        vk::PhysicalDevice physicalDevice;
        //logical device
        vk::Device logicalDevice;
        //queue family indices
        QueueFamilyIndices queueFamilyIndices;
        //graphics queue handle (no need to destroy)
        VkQueue graphicsQueue{VK_NULL_HANDLE};
        //present queue handle for image output command to the surface (no need to destroy)
        VkQueue presentQueue{VK_NULL_HANDLE};
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

        #if DEPTH_BUFFER
        //Depth buffer
        std::vector<Image> depthImages;
        #endif //DEPTH_BUFFER

        //output image format
        VkFormat swapChainImageFormat;
        //output image extent
        VkExtent2D swapChainExtent;
        //Render pass
        VkRenderPass renderPass{VK_NULL_HANDLE};
        //command pool
        VkCommandPool commandPool{VK_NULL_HANDLE};
        //command pool with support for buffer overwriting without the need to flush the pool
        VkCommandPool commandPoolRest{VK_NULL_HANDLE};
        //command buffers for each frame in the swapchain(released when the command pool is destroyed)
        std::vector<vk::CommandBuffer> cbRender;

    //SEMAPHORES (sync GPU & GPU) for each frame
        //image available for rendering
        std::vector<VkSemaphore> imageAvailableSemaphores;
        //rendering finished and image can be presented
        std::vector<VkSemaphore> renderFinishedSemaphores;

    //FENCES (sync CPU & GPU)
        std::vector<VkFence> inFlightFences;



        uint32_t currentFrame{0};

        bool windowResized{false};
        //Graphic pipelines
        //std::vector<DrawProgram> drawingMods;

        //representation of system (contains platform-dependent methods for loading files, creating surface, etc.)
        ISysRes& sysRes;

//METHODS
    public:
        //initialization of the engine
        void init(ISysRes& sysRes, int width, int height);
        //check swapchain, process all changes, update all data, rendering and presentation
        void drawFrame();
        //change extent of swapchain
        void setExtent(int width, int height);
    protected:

    private:
    //SETUP VULKAN DEVICES
        //create instance
        void createInstance();

        //find a suitable physical device
        void pickPhysicalDevice();

        //----------------------------------------------------------------------------------------------------------------------
        bool isDeviseSuitable(VkPhysicalDevice &physDevise);

        //----------------------------------------------------------------------------
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        //create logical device
        void createLogicalDevice();

    //SWAP CHAIN
        void recreateSwapChain();
        void cleanupSwapChain();
        void createSwapChain();
        //create image views for each image in the swapchain
        void createImageViews();
        //query swapchain support details
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

        //choose swapchain surface format
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

        //choose swapchain present mode
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

        //choose extent
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
        //create framebuffers for each image in the swapchain
        void createFrameBuffers();
#if DEPTH_BUFFER//create depth buffer
        void createDepthResources();
#endif


    //RENDER PASS
        //describe the render pass
        void createRenderPass();

        //create command pool
        void createCommandPool();

        //create command pool with support for buffer overwriting without the need to flush the pool
        void createCommandPoolRest();

        //create command buffers for rendering for each frame in the swapchain
        void createCbRender();

        //create sync objects (semaphores and fences)
        void createSyncObjects();


    //DEBUG
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

        void setupDebugMessenger();

    //DESTROY
        //clear all resources and destroy all Vulkan objects
        void cleanup();
    };
}