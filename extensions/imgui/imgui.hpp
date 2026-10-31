#pragma once
#include "imgui/imgui.h"
#include <cstdint>
#include <vulkan/vulkan.h>
#include <functional>
#include <stdexcept>
#include <string>
#include "../core/EngineBase.h"


namespace vkbase::imgui
{

    void initIMGUI();

    void cleanupIMGUI();

    void createIMGUIDescriptorPool();

    void createIMGUICommandBuffers();

    void renderIMGUI();

    void rewriteIMGUICommandBuffer(uint32_t imageIndex);

    bool showDebugWindow();

    VkDescriptorSetLayout getIMGUIDescriptorSetLayout();

    void dockWindow(const char* A, const char* B, float proportion);

    int addOnUiCallback(const std::function<void()>& callback, int priority= 0);
    void removeOnUiCallback(int id);

    float getDefaultFontSize();
    const std::string& getDefaultFontName();

    void setDefaultFontSize(float size);
    void setDefaultFontName(const std::string& font_name);

    //  font_name is expected to be placed in assets: Fonts/font_name.ttf
    //
    //  It might be impossible to push the font on current frame if it haven't been used before
    //  in this case default font will be pushed and function will return false
    bool pushFont(const std::string& font_name, int size);
    //push default font of a specified size
    bool pushFont(int size);

    void scaleDefaultFontUI();


    extern VkDescriptorPool imguiDescriptorPool;
    class Image
    {
        VkImageView internalImage;
        VkSampler sampler;
        VkDescriptorSet descriptor_set;

    public:
        Image(VkImageView image):internalImage(image)
        {
            VkImageView view=image;

            VkSamplerCreateInfo sampler_info{};
            sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sampler_info.magFilter = VK_FILTER_NEAREST;
            sampler_info.minFilter = VK_FILTER_NEAREST;
            sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // outside image bounds just use border color
            sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_info.minLod = -1000;
            sampler_info.maxLod = 1000;
            sampler_info.maxAnisotropy = 1.0f;
            if(vkCreateSampler(vkbase::device, &sampler_info, nullptr, &sampler) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create sampler!");
            }


            VkDescriptorSetAllocateInfo alloc_info = {};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool = imguiDescriptorPool;
            alloc_info.descriptorSetCount = 1;
            auto set_layout=getIMGUIDescriptorSetLayout();
            alloc_info.pSetLayouts = &set_layout;
            if(vkAllocateDescriptorSets(vkbase::device, &alloc_info, &descriptor_set)!=VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate descriptor set!");
            }
            VkDescriptorImageInfo desc_image[1] = {};
            desc_image[0].sampler = sampler;
            desc_image[0].imageView = view;
            desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            VkWriteDescriptorSet write_desc[1] = {};
            write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_desc[0].dstSet = descriptor_set;
            write_desc[0].descriptorCount = 1;
            write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write_desc[0].pImageInfo = desc_image;
            vkUpdateDescriptorSets(vkbase::device, 1, write_desc, 0, nullptr);
        }

        Image(const Image& other) =delete;
        Image(Image&& other)
        {
            *this=std::move(other);
        }

        Image& operator=(const Image& other)=delete;
        Image& operator=(Image&& other)
        {
            internalImage = other.internalImage;
            sampler = other.sampler;
            descriptor_set = other.descriptor_set;

            other.sampler = VK_NULL_HANDLE;
            return *this;
        }

        ~Image()
        {
            if (sampler)
            {
                vkDestroySampler(device,sampler,nullptr);
                vkFreeDescriptorSets(device, imguiDescriptorPool, 1, &descriptor_set);
            }
        }

        operator ImTextureID() const
        {
            return descriptor_set;
        }
    };
}
