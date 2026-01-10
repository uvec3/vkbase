#pragma once
#include <fstream>
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>

#include "vkbase/include/VKBASE/ISysRes.h"
#include <backends/imgui_impl_glfw.h>



class SysResGLFW: public ISysRes
{
private:
    GLFWwindow* m_window;
    std::string m_resourcesPath= "";

public:

    explicit SysResGLFW(GLFWwindow *window,std::string resourcesPath="")
    {
        if(!resourcesPath.empty()&& resourcesPath.back()!='/')
            resourcesPath.push_back('/');
        m_resourcesPath= resourcesPath;
        m_window= window;
    }

    std::vector<char> load_file(const char *path) override
    {
        //open file in binary mode and go to the end
        std::ifstream file(m_resourcesPath+path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file on disk!");
        }

        //create vector of char with size of file size
        int size = static_cast<int>(file.tellg());
        std::vector<char> buffer(size);

        //go back to the beginning of the file
        file.seekg(0);
        //read
        file.read(buffer.data(), size);

        file.close();

        return buffer;
    }

    VkSurfaceKHR createSurface(VkInstance instance) override
    {
        //ask glfw to create surface
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance, m_window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface!");
        }
        return surface;
    }


    int getWidth() override
    {
        int width;
        glfwGetFramebufferSize(m_window, &width, nullptr);
        return width;
    }

    int getHeight() override
    {
        int height;
        glfwGetFramebufferSize(m_window, nullptr, &height);
        return height;
    }


    std::vector<const char *> getRequiredInstanceExtensions() override
    {
        uint32_t glfwExtensionCount = 0;
        //get from glfw
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        return extensions;
    }

    void info(const std::string &message) override
    {
        std::cout << message << std::endl;
    }

    void error(const std::string &message) override
    {
        std::cerr << message << std::endl;
    }

    void warning(const std::string &message) override
    {
        std::cout << message << std::endl;
    }

    virtual void initIMGUIPlatformBackend() override
    {
        ImGui_ImplGlfw_InitForVulkan(m_window, true);
    }

    virtual void shutdownIMGUIPlatformBackend() override
    {
        ImGui_ImplGlfw_Shutdown();
    }

    virtual void newFrameIMGUIPlatformBackend() override
    {
        ImGui_ImplGlfw_NewFrame();
    }

    float getDPI() override
    {
        return 140;
    }

    void captureKeyboard() override
    {

    }

    uint64_t getMilliseconds() override
    {
        return glfwGetTime()*1000;
    }

};


