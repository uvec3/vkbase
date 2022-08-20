
#ifndef MANDELBROD_ANDROIDSYSRES_H
#define MANDELBROD_ANDROIDSYSRES_H

#include "ISysRes.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <vulkan/vulkan.h>

class AndroidSysRes: public ISysRes
{
private:
    android_app* appInfo;
public:
    AndroidSysRes(android_app* appInfo)
    {
        this->appInfo = appInfo;
    }

    virtual std::vector<char> load_file(const char* path) override;
    virtual VkSurfaceKHR create_surface(VkInstance instance);
};


#endif //MANDELBROD_ANDROIDSYSRES_H
