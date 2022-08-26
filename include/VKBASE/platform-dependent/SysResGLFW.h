#pragma once

#include <vulkan/vulkan.h>
#include "vkbase/include/VKBASE/ISysRes.h"
#include "GLFW/glfw3.h"

class SysResGLFW: public ISysRes
{
private:
    GLFWwindow* m_window;

public:

    SysResGLFW(GLFWwindow *window)
    {
        m_window = window;
    }

    std::vector<char> load_file(const char *path) override
    {
        //open file in binary mode and go to the end
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
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


    std::vector<const char *> getRequiredInstanceExtensions() override
    {
        //count
        uint32_t glfwExtensionCount = 0;
        //получаем из glfw
        const char **glfwExtensions;
        //запрос количества зарегестрированых расширений
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        //инициализация вектора
        //vector можно инициализировав передав начальный и послеконечный элемент (как я узнал позже интератор)
        //массива так как для foreach
        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);


        //ну и возврат необходимых для работы расширений
        return extensions;
    }

    void info(const std::string &message) override
    {
        //std::cout << message << std::endl;
    }

    void error(const std::string &message) override
    {
        std::cerr << message << std::endl;
    }

    void warning(const std::string &message) override
    {
        std::cout << message << std::endl;
    }

};


