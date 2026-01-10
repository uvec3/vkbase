
#ifndef MANDELBROD_ANDROIDSYSRES_H
#define MANDELBROD_ANDROIDSYSRES_H

#include "../ISysRes.h"
#include <vulkan/vulkan.h>
#include <imgui.h>
#include <imgui_impl_android.h>


class AndroidSysRes: public ISysRes
{
private:
    android_app* appInfo;
public:
    AndroidSysRes(android_app* appInfo)
    {
        this->appInfo = appInfo;
    }

    virtual std::vector<char> load_file(const char* path) override
    {
        std::vector<char> buffer;
        //asset list
        AAsset* asset = AAssetManager_open(appInfo->activity->assetManager, path, AASSET_MODE_BUFFER);
        size_t size = AAsset_getLength(asset);
        buffer.resize(size);
        AAsset_read(asset, buffer.data(), size);
        AAsset_close(asset);
        return buffer;
    }

    virtual VkSurfaceKHR createSurface(VkInstance instance) override
    {
        VkSurfaceKHR surface;
        VkAndroidSurfaceCreateInfoKHR createInfo{
                .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .window = appInfo->window};
        vkCreateAndroidSurfaceKHR(instance, &createInfo, nullptr, &surface);
        return surface;
    }

    //list of instance extensions which are necessary for platform
    virtual std::vector<const char *> getRequiredInstanceExtensions()  override
    {
        return {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
    }

    virtual int getWidth() override
    {
        return ANativeWindow_getWidth(appInfo->window);
    }

    virtual int getHeight() override
    {
        return ANativeWindow_getHeight(appInfo->window);
    }

    virtual void info(const std::string& message) override
    {
        LOGI("%s", message.c_str());
    }

    virtual void error(const std::string& message) override
    {
        LOGE("%s", message.c_str());
    }

    virtual void warning(const std::string& message) override
    {
        LOGW("%s", message.c_str());
    }

    void initIMGUIPlatformBackend() override
    {
        ImGui_ImplAndroid_Init(appInfo->window);
    }

    void shutdownIMGUIPlatformBackend() override
    {
        ImGui_ImplAndroid_Shutdown();
    }

    void newFrameIMGUIPlatformBackend() override
    {
        ImGui_ImplAndroid_NewFrame();
    }

};


#endif //MANDELBROD_ANDROIDSYSRES_H
