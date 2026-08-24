#pragma once
#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include <queue>


namespace vkbase::sys
{

    void init();
    void destroy();

    bool handleEvents();

    //load files for Engine (return empty vector if file not found)
    std::vector<char> load_file(const char *path);

    //list of instance extensions which are necessary for platform
    std::vector<const char *> getRequiredInstanceExtensions();

    //create VkSurfaceKHR for output
    VkSurfaceKHR createSurface(VkInstance instance);

    int getWidth();

    int getHeight();

    void setWindowSize(int width, int height);

    float getDPI();

    void info(const char *str, int size);

    void error(const char *str, int size);

    void warning(const char *str, int size);

    void fullscreen(bool enable);

    void captureKeyboard();

    double getMilliseconds();

    std::vector<char> saveState();
}

