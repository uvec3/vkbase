#include "VulkanValidationLayer.h"
#include <cstring>
#include <vector>
#include <GLFW/glfw3.h>



//проверяет наличие необходимых расширений для использования слоев проверки
bool checkValidationLayerSupport()
{
	uint32_t layerCount;
	//запрос на получение длины списка
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	//теперь создание массива доступных
	VkLayerProperties* availableLayers=new VkLayerProperties[layerCount];
	//повторный запрос уже с возвратом по ссылке через параметр
	//массив хранит доступные слои проверки
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
	//поиск необходимых слоев в SDK
	for (const char* chekingLayers : validationLayers)
	{
		bool checking=false;
		for (uint32_t i=0;i<layerCount;++i)
			if (strcmp(chekingLayers, availableLayers[i].layerName)==0)
			{
				checking = true;
				break;
			}

		if (!checking)
			return false;
	}
	return true;
}
//cоздать месседжер отладки
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}
//разрушить месседжер отладки
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}



