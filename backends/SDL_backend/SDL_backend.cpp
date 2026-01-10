#include <iostream>

#include "../../core/SystemBeckend.hpp"
#include "../../core/Event.hpp"
#include "SDL_vulkan.h"
#include <vulkan/vulkan.h>
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

    #include <cstdio>
    #include <stdexcept>

    #define LOGI(...) \
            ((void)printf(__VA_ARGS__))
    #define LOGE(...)   \
            ((void)fprintf(stderr, __VA_ARGS__))
    #define LOGW(...)   \
            ((void)fprintf(stderr, __VA_ARGS__))
#endif

namespace vkbase
{
    extern std::string  m_appName;
}

namespace vkbase::sys
{
    Event<SDL_Event*> sdlEvent;

    SDL_Window *window;
    int m_width=1000;
    int m_height=700;


    struct SaveData
    {
        int width;
        int height;
        bool fullscreen;
        bool maximized;
        int x;
        int y;
    };

    static bool fullscreenMode=false;
    void fullscreen(const bool enable)
    {
        if(window)
        {
            if (enable)
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN   );
            else
            {
                SDL_SetWindowFullscreen(window, 0);
                SDL_SetWindowSize(window, m_width-1, m_height-1);
            }
        }
        fullscreenMode=enable;
    }


    void init()
    {
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");

        // Setup SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
        {
            throw std::runtime_error("SDL init error: " + std::string(SDL_GetError()));
        }

        // get monitors count
        int monitorsCount = SDL_GetNumVideoDisplays();
        int display=0;


        int window_flags = SDL_WINDOW_VULKAN| SDL_WINDOW_RESIZABLE;

        if(fullscreenMode)
            window_flags |= SDL_WINDOW_FULLSCREEN;

        SDL_Window* w = SDL_CreateWindow(m_appName.c_str(), SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,m_width, m_height, window_flags);

        sys::window = w;


        //disable SDL mouse simulation with touch events
        SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    }

    void destroy()
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    std::vector<char> load_file(const char *path)
    {
        std::vector<char> buffer;
        SDL_RWops *rw = SDL_RWFromFile(path, "rb");
        if (rw != nullptr)
        {
            auto res_size = SDL_RWsize(rw);
            buffer.
                    resize(res_size);
            Sint64 nb_read_total = 0, nb_read = 1;
            char *buf = buffer.data();
            while (nb_read_total < res_size && nb_read != 0)
            {
                nb_read = SDL_RWread(rw, buf, 1, (res_size - nb_read_total));
                nb_read_total +=
                        nb_read;
                buf +=
                        nb_read;
            }
            SDL_RWclose(rw);
        }

        return buffer;
    }

    VkSurfaceKHR createSurface(VkInstance instance)
    {
        VkSurfaceKHR surface;
        if (SDL_Vulkan_CreateSurface(window, instance, &surface) == 0)
            throw std::runtime_error("Failed to create surface");
        return
            surface;
    }

    //list of instance extensions which are necessary for platform
    std::vector<const char *> getRequiredInstanceExtensions()
    {
        std::vector<const char *> extensions;
        uint32_t extensions_count = 0;
        SDL_Vulkan_GetInstanceExtensions(window, &extensions_count,nullptr);
        extensions.resize(extensions_count);
        SDL_Vulkan_GetInstanceExtensions(window, &extensions_count, extensions.data());
        return  extensions;
    }

    int getWidth()
    {
        SDL_GetWindowSize(window, &m_width,nullptr);
        return m_width;
    }

    int getHeight()
    {
        SDL_GetWindowSize(window,nullptr, &m_height);
        return m_height;
    }

    void setWindowSize(int width, int height)
    {
        SDL_SetWindowSize(window, width, height);
        m_width=width;
        m_height=height;
    }

    float getDPI()
    {
        float dpi = 1.0f;
        int current_display = SDL_GetWindowDisplayIndex(window);
        if(!SDL_GetDisplayDPI(current_display, &dpi, nullptr, nullptr)||dpi<=0.0f)
            return 96.0f;
        return dpi;
    }

    void info(const char *str, int size)
    {
        LOGI("%.*s",size,str);
    }

    void error(const char *str, int size)
    {
        LOGE("%.*s",size,str);
    }

    void warning(const char *str, int size)
    {
        LOGW("%.*s",size,str);
    }

    bool handleEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                return false;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                return false;


            switch (event.type)
            {
                //touch events
                case SDL_FINGERDOWN:
                {
                    engEvents.touchEvents.push(
                            {event.tfinger.fingerId, {event.tfinger.x * m_width, event.tfinger.y * m_height},
                             event.tfinger.timestamp,
                             TouchEvent::DOWN});
                    continue;
                }

                case SDL_FINGERUP:
                {
                    engEvents.touchEvents.push(
                            {event.tfinger.fingerId, {event.tfinger.x * m_width, event.tfinger.y * m_height},
                             event.tfinger.timestamp,
                             TouchEvent::UP});
                    continue;
                }

                case SDL_FINGERMOTION:
                {
                    engEvents.touchEvents.push(
                            {event.tfinger.fingerId, {event.tfinger.x * m_width, event.tfinger.y * m_height},
                             event.tfinger.timestamp,
                             TouchEvent::MOVE});
                    continue;
                }
            }

            sdlEvent.call(&event);
        }
        return true;
    }

    void captureKeyboard()
    {
        SDL_SetTextInputRect(nullptr);
        SDL_StartTextInput();
    }

    double getMilliseconds()
    {
        return static_cast<double>(SDL_GetTicks64());
    }

    std::vector<char> saveState()
    {
        SaveData data;
        data.width = m_width;
        data.height = m_height;
        data.fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
        data.maximized = (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) != 0;
        SDL_GetWindowPosition(window, &data.x, &data.y);

        std::vector<char> buffer(sizeof(SaveData));
        std::memcpy(buffer.data(), &data, sizeof(SaveData));
        return buffer;
    }
};