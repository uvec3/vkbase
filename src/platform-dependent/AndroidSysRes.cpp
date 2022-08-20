

#include "AndroidSysRes.h"
#include <android/asset_manager.h>
#include <vulkan/vulkan.h>
#include <game-activity/GameActivity.h>


std::vector<char> AndroidSysRes::load_file(const char *path)
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

VkSurfaceKHR AndroidSysRes::create_surface(VkInstance instance)
{
    return NULL;
}
