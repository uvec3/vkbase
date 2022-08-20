#include "SysResDesktop.h"
#include <fstream>
#include <GLFW/glfw3.h>

std::vector<char> SysResDesktop::load_file(const char *path)
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

VkSurfaceKHR SysResDesktop::createSurface(VkInstance instance)
{
    return nullptr;
}

std::vector<const char *> SysResDesktop::getRequiredInstanceExtensions()
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
