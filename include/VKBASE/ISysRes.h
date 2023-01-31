#pragma once
#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include <queue>
#include <glm/glm.hpp>

struct TouchEvent
{
    enum Action
    {
        DOWN,
        MOVE,
        UP
    };

    int64_t index;
    glm::vec2 pos;
    uint64_t time;
    Action action;
};

struct EngEvents
{
    std::queue<TouchEvent> touchEvents;
};

//system-dependent methods interface
//necessary for creating engine
//you can use one of existed classes which implement this interface
//or define your own
class ISysRes
{
public:
    //load files for Engine (return empty vector if file not found)
    virtual std::vector<char> load_file(const char *path) = 0;

    //list of instance extensions which are necessary for platform
    virtual std::vector<const char *> getRequiredInstanceExtensions() = 0;

    //create VkSurfaceKHR for output
    virtual VkSurfaceKHR createSurface(VkInstance instance) = 0;

    virtual int getWidth()=0;

    virtual int getHeight()=0;

    virtual float getDPI()=0;

    virtual void info(const std::string& message)=0;

    virtual void error(const std::string& message)=0;

    virtual void warning(const std::string& message)=0;

    virtual void initIMGUIPlatformBackend()=0;

    virtual void shutdownIMGUIPlatformBackend()=0;

    virtual void newFrameIMGUIPlatformBackend()=0;

    virtual void captureKeyboard()=0;

    virtual uint64_t getMilliseconds()=0;

    EngEvents* engEvents=nullptr;
};
