#pragma once

#include "../../core/EngineBase.h"
#include <cuda_runtime.h>
#include "cuda.hpp"

namespace vkbase::cuda
{
    class Image
    {
    public:
        Image(uint32_t width, uint32_t height, VkFormat format=VK_FORMAT_R8G8B8A8_UNORM):
        width(width),height(height)
        {
            create_image();
        }

        Image(const Image& other) =delete;
        Image(Image&& other)
        {
            *this=std::move(other);
        }

        Image& operator=(const Image& other)=delete;

        Image& operator=(Image&& other) noexcept
        {
            width=other.width;
            height=other.height;
            image_view = other.image_view;
            image = other.image;
            memory = other.memory;
            cuda_surface = other.cuda_surface;
            mipmapArray = other.mipmapArray;
            extMem = other.extMem;

            other.image_view=VK_NULL_HANDLE;

            return *this;
        }

        ~Image()
        {
            destroy();
        }

        uint32_t width;
        uint32_t height;

        VkImageView image_view=VK_NULL_HANDLE;
        VkImage image=VK_NULL_HANDLE;
        VkDeviceMemory memory=VK_NULL_HANDLE;


        cudaSurfaceObject_t cuda_surface;
        cudaMipmappedArray_t mipmapArray;
        cudaExternalMemory_t extMem;

    private:
        void create_image()
        {
            auto format=VK_FORMAT_R8G8B8A8_UNORM;

            // 1. Tell Vulkan the image memory will be exported
            VkExternalMemoryImageCreateInfo extImageInfo = {};
            extImageInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
            extImageInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_PLATFORM;

            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.pNext = &extImageInfo; // Chain the export info
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = format;
            imageInfo.extent = {width, height, 1};
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples=VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage=VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;

            vkCreateImage(vkbase::device,&imageInfo,nullptr,&image);
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device, image, &memRequirements);

            // 2. Allocate the memory with export flags
            VkExportMemoryAllocateInfo exportAllocInfo = {};
            exportAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
            exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_PLATFORM;

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.pNext = &exportAllocInfo; // Chain the export info
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = vkbase::findMemoryType(memRequirements.memoryTypeBits,0,0);
            vkAllocateMemory(vkbase::device,&allocInfo,nullptr,&memory);

            vkBindImageMemory(device, image, memory, 0);

            VkImageViewCreateInfo view_create_info{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_create_info.format = format;
            view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_create_info.subresourceRange.baseMipLevel = 0;
            view_create_info.subresourceRange.levelCount = 1;
            view_create_info.subresourceRange.baseArrayLayer = 0;
            view_create_info.subresourceRange.layerCount = 1;
            view_create_info.image=image;
            vkCreateImageView(vkbase::device,&view_create_info,nullptr,&image_view);

            // 3. Extract Handle from Vulkan
            NativePlatformHandle sharedHandle;

            #ifdef _WIN32
                        // Windows extraction
                        VkMemoryGetWin32HandleInfoKHR getHandleInfo = { VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR };
                        getHandleInfo.memory = memory;
                        getHandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

                        // Function pointer loaded via vkGetDeviceProcAddr
                        pfnVkGetMemoryWin32HandleKHR(device,&getHandleInfo,&sharedHandle);
            #else
                        // Linux extraction
                        VkMemoryGetFdInfoKHR getFdInfo = { VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR };
                        getFdInfo.memory = imageMemory;
                        getFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

                        // Function pointer loaded via vkGetDeviceProcAddr
                        pfnVkGetMemoryFdKHR(device, &getFdInfo, &sharedHandle);
            #endif

            // 1. Import the Vulkan memory handle
            cudaExternalMemoryHandleDesc extMemDesc = {};
            extMemDesc.type = CUDA_EXTERNAL_MEMORY_HANDLE_TYPE_PLATFORM;
            #ifdef _WIN32
                extMemDesc.handle.win32.handle = sharedHandle;
            #else
                extMemDesc.handle.fd = sharedHandle;
            #endif

            extMemDesc.size = memRequirements.size; // Must match Vulkan's allocation exactly

            cudaImportExternalMemory(&extMem, &extMemDesc);

            #ifdef _WIN32
                    CloseHandle(sharedHandle);
            #else
                    close(sharedHandle);
            #endif

            // 2. Map the external memory to a CUDA Mipmapped Array
            cudaExternalMemoryMipmappedArrayDesc mipDesc = {};
            mipDesc.offset = 0;
            // Match Vulkan format (VK_FORMAT_R8G8B8A8_UNORM = 4 channels of 8-bit unsigned)
            mipDesc.formatDesc = cudaCreateChannelDesc(8, 8, 8, 8, cudaChannelFormatKindUnsigned);
            mipDesc.extent = make_cudaExtent(width, height, 0);
            mipDesc.flags = cudaArraySurfaceLoadStore; // CRITICAL: Allows writing
            mipDesc.numLevels = 1;


            cudaExternalMemoryGetMappedMipmappedArray(&mipmapArray, extMem, &mipDesc);

            // 3. Extract the base level array and create a Surface Object
            cudaArray_t cudaArr;
            cudaGetMipmappedArrayLevel(&cudaArr, mipmapArray, 0);

            cudaResourceDesc resDesc = {};
            resDesc.resType = cudaResourceTypeArray;
            resDesc.res.array.array = cudaArr;

            cudaCreateSurfaceObject(&cuda_surface, &resDesc);
        }

        void destroy()
        {
            if (image_view)
            {
                cudaDestroySurfaceObject(cuda_surface);
                cudaFreeMipmappedArray(mipmapArray);
                cudaDestroyExternalMemory(extMem);

                vkDestroyImageView(device,image_view,nullptr);
                vkDestroyImage(device, image, nullptr);
                vkFreeMemory(device, memory, nullptr);
            }

            image_view=VK_NULL_HANDLE;
        }
    };
}
