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
    const std::string& getDefaultFontName();

    void setDefaultFontSize(float size);
    void setDefaultFontName(const std::string& font_name);

    //  font_name is expected to be placed in assets: Fonts/font_name.ttf
    //
    //  It might be impossible to push the font on current frame if it haven't been used before
    //  in this case default font will be pushed and function will return false
    bool pushFont(const std::string& font_name, int size);
    //push default font of a specified size
    bool pushFont(int size);
}
