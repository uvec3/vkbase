#pragma once
#include "imgui/imgui.h"
#include <cstdint>
#include <vulkan/vulkan.h>
#include <functional>
#include <string>

namespace vkbase::imgui
{

    void initIMGUI();

    void cleanupIMGUI();

    void createIMGUIDescriptorPool();

    void createIMGUICommandBuffers();

    void renderIMGUI();

    void rewriteIMGUICommandBuffer(uint32_t imageIndex);

    bool showDebugWindow();

    VkDescriptorSetLayout getIMGUIDescriptorSetLayout();

    void dockWindow(const char* A, const char* B, float proportion);

    int addOnUiCallback(const std::function<void()>& callback, int priority= 0);
    void removeOnUiCallback(int id);

    float getDefaultFontSize();
    void setDefaultFontSize(float size);
    void pushFont(const std::string& font_name, int size);
    void setDefaultFont(const std::string& font_name, int size);
}
