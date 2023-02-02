#pragma once

#include "../ISysRes.h"
#include <vulkan/vulkan.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <SDL.h>

#if ANDROID
    #include <android/log.h>
    #define LOGI(...) \
        ((void)__android_log_print(ANDROID_LOG_INFO, "threaded_app", __VA_ARGS__))
    #define LOGE(...) \
        ((void)__android_log_print(ANDROID_LOG_ERROR, "threaded_app", __VA_ARGS__))
    #define LOGW(...) \
        ((void)__android_log_print(ANDROID_LOG_WARN, "threaded_app", __VA_ARGS__))
#else
    #include <stdio.h>
#include <stdexcept>

    #define LOGI(...) \
        ((void)printf(__VA_ARGS__), printf("\n"))
    #define LOGE(...)   \
        ((void)fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n"))
    #define LOGW(...)   \
        ((void)fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n"))
#endif

class SysResSDL: public ISysRes
{
private:
    SDL_Window* window;
    int w, h;


public:
    SysResSDL(SDL_Window* window)
    {
        this->window = window;
        //disable SDL mouse simulation with touch events
        SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
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
        SDL_GetWindowSize(window, &w, nullptr);
        return w;
    }

    virtual int getHeight() override
    {
        SDL_GetWindowSize(window, nullptr, &h);
        return h;
    }

    virtual float getDPI() override
    {
        float dpi = 1.0f;
        SDL_GetDisplayDPI(0, &dpi, nullptr, nullptr);
        return dpi;
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

    void handleEvent(SDL_Event& event)
    {

        ImGuiIO& io = ImGui::GetIO();
        
        switch (event.type)
        {

            //touch events
            case SDL_FINGERDOWN:
            {


                engEvents->touchEvents.push({event.tfinger.fingerId, {event.tfinger.x*w, event.tfinger.y*h}, event.tfinger.timestamp, TouchEvent::DOWN});
                return;
            }

            case SDL_FINGERUP:
            {
                info("touch id: " + std::to_string(event.tfinger.fingerId));

                engEvents->touchEvents.push({event.tfinger.fingerId, {event.tfinger.x*w, event.tfinger.y*h}, event.tfinger.timestamp, TouchEvent::UP});
                return;
            }

            case SDL_FINGERMOTION:
            {
                engEvents->touchEvents.push({event.tfinger.fingerId, {event.tfinger.x*w, event.tfinger.y*h}, event.tfinger.timestamp, TouchEvent::MOVE});
                return;
            }
        }



        ImGui_ImplSDL2_ProcessEvent(&event);

    }

    void captureKeyboard() override
    {
        SDL_SetTextInputRect(nullptr);
        SDL_StartTextInput();
    }

    uint64_t getMilliseconds() override
    {
        return SDL_GetTicks64();
    }

};

