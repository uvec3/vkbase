#pragma once
#include <optional>
#include <array>
#include <map>

#include <vulkan/vulkan.h>
#include <imgui.h>
#if VKBASE_SHADER_RUNTIME_COMPILE
#include <shaderc/shaderc.hpp>
#endif

#include "../include/VKBASE/ISysRes.h"
#include "Image.h"
#include "glm/vec2.hpp"


namespace vkbase
{

    class EngineBase
    {
    //PUBLIC INTERFACE
    public:
        //constructor
        EngineBase(ISysRes& pSysRes);
        //destructor
        virtual ~EngineBase();
        //check swapchain, process all changes, update all data, rendering and presentation
        void drawFrame();
        //show ui window with info about engine state
        void showDebugWindow();


        glm::vec2 surfaceSize;
        glm::vec2 surfaceCenter;

    //FRIENDS
    public:
        friend class Image;
        friend class Buffer;

    //TYPES
    private:
        //struct stores queues indices on physical device
        struct QueueFamilyIndices
        {
            //for graphics queue
            std::optional<uint32_t> graphicsFamily;
            //for presenting
            std::optional<uint32_t> presentFamily;
            //for compute pipelines
            std::optional<uint32_t> computeFamily;

            //check if all queue families are found
            [[nodiscard]] bool isComplete() const
            {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }
        };

        //struct for storing info about surface capabilities on selected physical device
        struct SwapChainSupportDetails
        {
            //Basic features of the surface (minimum / maximum  number of images in the exchange chain,
            //min/max/current width and height of images, supported transforms)
            VkSurfaceCapabilitiesKHR capabilities;
            // Surface formats (pixel format, color space)
            std::vector<VkSurfaceFormatKHR> formats;
            // Available presentation modes
            std::vector<VkPresentModeKHR> presentModes;
        };

        struct FingerState
        {
            glm::vec2 pos;
            bool down;
            int64_t time;
            int64_t downTime;
            glm::vec2 downPos;
        };

        struct TouchData
        {
            std::array<FingerState,10> fingerState;
            int imguiFinger=-1;
            FingerState imguiFingerState;
//            ImGui::Window=false;

            int fingersCount() const
            {
                int count=0;
                for(auto& f : fingerState)
                {
                    if(f.down)
                        count++;
                }
                return count;
            }


        };

    //CONSTANTS, SETTINGS
    protected:
        //assets (constructor will initialize it)
        std::map<std::string, std::string>& assets;

        //the desired number of frames to be processed simultaneously (corrected when creating a swapchain with restrictions)
        uint32_t imageCount = 2;

        //required extensions for device
        const std::vector<const char *> deviceExtensions{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

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
        //representation of system (contains platform-dependent methods for loading files, creating surface, etc.)
        ISysRes* sysRes;
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
        //output image format
        VkFormat swapChainImageFormat;
        //output image extent
        VkExtent2D extent;
        //Render pass
        VkRenderPass renderPass{VK_NULL_HANDLE};
        //command pool
        VkCommandPool commandPool{VK_NULL_HANDLE};
        //command pool with support of overwriting single buffer without flush all pool
        VkCommandPool commandPoolReset{VK_NULL_HANDLE};
        //command buffers for main rendering in the swapchain(released when the command pool is destroyed)
        std::vector<VkCommandBuffer> cbRender;
        //main command buffer (rewrite every frame) (call other command buffers(cbRender and ui) )
        std::vector<VkCommandBuffer> cbMain;

        //SEMAPHORES (sync GPU & GPU) for each frame
        //image available for rendering
        std::vector<VkSemaphore> imageAvailableSemaphores;
        //rendering finished and image can be presented
        std::vector<VkSemaphore> renderFinishedSemaphores;

        //FENCES (sync CPU & GPU)
        std::vector<VkFence> fenceRenderFinished;
        //current frame
        uint32_t currentFrame{0};

        bool windowResized{false};

        //IMGUI
        VkDescriptorPool imguiDescriptorPool{VK_NULL_HANDLE};
        std::vector<VkCommandBuffer> cbIMGUI;
        ImGuiIO* imguiIO;
        ImDrawData* imguiDrawData= nullptr;
        //TOUCHES
        EngEvents engEvents;
        TouchData touchData;


    //VIRTUAL METHODS
    protected:
        //called before submit command buffer
        virtual void drawPrepare() {};
        //
        virtual void handleTouch(TouchEvent& touchEvent) {};
    //INTERNAL METHODS
    protected:
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
        //find memory type
        uint32_t findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties,
                                 VkMemoryHeapFlags heapFlags);

        //SWAP CHAIN
        void createSwapChain();
        virtual void recreateSwapChain();
        void cleanupSwapChain();
        //create image views for each image in the swapchain
        void createImageViews();
        //query swapchain support details
        SwapChainSupportDetails querySwapChainDetails(VkPhysicalDevice physDevice);
        //choose swapchain surface format
        static  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        //choose swapchain present mode
        static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
        //choose extent
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
        //choose composite alpha
        static VkCompositeAlphaFlagBitsKHR chooseSwapChainCompositeAlpha(VkCompositeAlphaFlagsKHR availableAlpha);
        //create framebuffers for each image in the swapchain
        void createFrameBuffers();

        //COMMAND POOLS
        //create command pool
        void createCommandPool();
        //create command pool with support for buffer overwriting without the need to flush the entire pool
        void createCommandPoolReset();
        void createRenderBuffers();
        void createMainCommandBuffers();
        void rewriteMainBuffer(uint32_t imageIndex);

        //SYNC OBJECTS
        void createSyncObjects();

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

        //IMGUI
        void initIMGUI();
        void cleanupIMGUI();
        void createIMGUIDescriptorPool();
        void createIMGUICommandBuffers();
        void renderIMGUI();
        void rewriteIMGUICommandBuffer(uint32_t imageIndex);

        //TOUCHES
        void processTouches();

    //ADDITIONAL METHODS FOR INHERITANCE CLASSES
    protected:
        //create shader module from binary code
        [[maybe_unused]] VkShaderModule createShaderModule(const std::vector<char> &code);
        //read shader file using ISysRes interface
        [[maybe_unused]] VkShaderModule loadPrecompiledShader(const std::string& filename);
        #if VKBASE_SHADER_RUNTIME_COMPILE//use shaders compile library in engine
            [[maybe_unused]] std::vector<char> compileShader(const std::vector<char>& source, const std::string &fileName);

            //read source shader file using ISysRes interface and compile it
            [[maybe_unused]] VkShaderModule loadSourceShader(const std::string &filename, bool save= false);

            //try find compiled shader file, if not found - compile it from source
            [[maybe_unused]] VkShaderModule loadShader(const std::string& filename);
        #endif
        //retrieve device name
        static std::string getDeviceName(VkPhysicalDevice device);
    };
}