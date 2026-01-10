#pragma once
#include <optional>
#include <cstdint>
#include <vector>
#include <array>
#include <vulkan/vulkan.h>
#include "glm/glm/glm.hpp"




namespace vkbase
{
    struct EngineSettings
    {
        int preferableMonitor = 0;//---
        int preferableGPU= 0;//---
        int width = 0;//---
        int height = 0;//---
        bool fullscreen = false;//---

    };

    struct EngineParams
    {
        // system info
        bool mobileDevice = false;//---
        bool supportTouch = false;//---

        // engine modules info
        bool coreInitialized = false;//---
        bool imguiInitialized = false;//---


        EngineSettings settings;
    };



    //TYPES

    //struct stores queues indices on physical device
    struct QueueFamilyIndices
    {
        //for graphics queue
        std::optional<uint32_t> graphicsFamily;
        //for presenting
        std::optional<uint32_t> presentFamily;
        //for compute pipelines
        std::optional<uint32_t> computeFamily;
        //for transfer
        std::optional<uint32_t> transferFamily;

        //check if all queue families are found
        [[nodiscard]] bool isComplete() const
        {
            return graphicsFamily.has_value() && presentFamily.has_value()&& computeFamily.has_value()&& transferFamily.has_value();
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
        bool down = false;
        int64_t time;
        int64_t downTime;
        glm::vec2 downPos;
        int64_t internalID;
    };

    struct TouchData
    {
        std::array<FingerState, 10> fingerState;
        int imguiFinger = -1;
        FingerState imguiFingerState;
//            ImGui::Window=false;

        int fingersCount() const
        {
            int count = 0;
            for (auto &f: fingerState)
            {
                if (f.down)
                    count++;
            }
            return count;
        }
    };
}


