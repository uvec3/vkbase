
#ifndef MANDELBROD_ANDROIDSYSRES_H
#define MANDELBROD_ANDROIDSYSRES_H

#include "vkbase/include/VKBASE/ISysRes.h"
#include "VKBASE/ISysRes.h"
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

    virtual VkSurfaceKHR create_surface(VkInstance instance)
    {
        return NULL;
    }

};


#endif //MANDELBROD_ANDROIDSYSRES_H
