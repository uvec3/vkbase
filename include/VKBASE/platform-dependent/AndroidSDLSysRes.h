#pragma once

#include "../ISysRes.h"
#include <vulkan/vulkan.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <SDL.h>
#include <android/log.h>

#define LOGI(...) \
    ((void)__android_log_print(ANDROID_LOG_INFO, "threaded_app", __VA_ARGS__))
#define LOGE(...) \
    ((void)__android_log_print(ANDROID_LOG_ERROR, "threaded_app", __VA_ARGS__))
#define LOGW(...) \
    ((void)__android_log_print(ANDROID_LOG_WARN, "threaded_app", __VA_ARGS__))


class AndroidSDLSysRes: public ISysRes
{
private:
    SDL_Window* window;
public:
    AndroidSDLSysRes(SDL_Window* window)
    {
        this->window = window;
    }


    virtual std::vector<char> load_file(const char* path) override
    {
        std::vector<char> buffer;

        SDL_RWops *rw = SDL_RWFromFile(path, "rb");
        if (rw != nullptr)
        {
            auto res_size = SDL_RWsize(rw);
            buffer.resize(res_size);
            Sint64 nb_read_total = 0, nb_read = 1;
            char* buf = buffer.data();
            while (nb_read_total < res_size && nb_read != 0)
            {
                nb_read = SDL_RWread(rw, buf, 1, (res_size - nb_read_total));
                nb_read_total += nb_read;
                buf += nb_read;
            }
            SDL_RWclose(rw);
        }

        return buffer;
    }

    virtual VkSurfaceKHR createSurface(VkInstance instance) override
    {
        VkSurfaceKHR surface;
        if (SDL_Vulkan_CreateSurface(window, instance, &surface) == 0)
            throw std::runtime_error("Failed to create surface");
        return surface;
    }

    //list of instance extensions which are necessary for platform
    virtual std::vector<const char *> getRequiredInstanceExtensions()  override
    {
        std::vector<const char *> extensions;
        uint32_t extensions_count = 0;
        SDL_Vulkan_GetInstanceExtensions(window, &extensions_count, nullptr);
        extensions.resize(extensions_count);
        SDL_Vulkan_GetInstanceExtensions(window, &extensions_count, extensions.data());
        return extensions;
    }

    virtual int getWidth() override
    {
        int w;
        SDL_GetWindowSize(window, &w, nullptr);
        return w;
    }

    virtual int getHeight() override
    {
        int h;
        SDL_GetWindowSize(window, nullptr, &h);
        return h;
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
        ImGui_ImplSDL2_InitForVulkan(window);
    }

    void shutdownIMGUIPlatformBackend() override
    {
        ImGui_ImplSDL2_Shutdown();
    }

    void newFrameIMGUIPlatformBackend() override
    {
        ImGui_ImplSDL2_NewFrame(window);
    }

};

