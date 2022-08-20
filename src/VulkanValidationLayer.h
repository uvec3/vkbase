#pragma once
#include <array>
#include <vector>
#include <vulkan/vulkan.hpp>

//режим Release
#ifdef NDEBUG
	//здесь слои отладки не нужны
	constexpr bool enableValidationLayers = false;
	//что-бы не хранить в памяти присваиваю 0 значение
	static const std::array<const char*, 1> validationLayers{ nullptr };

#else//DEBUG
	//включить слои отладки
	constexpr bool enableValidationLayers = true;
	//стандартная валидация объединена в слой
	static const std::array<const char*, 1> validationLayers = {
"VK_LAYER_KHRONOS_validation"
	};
#endif // DEBUg

	//проверить поддержку отладки
	bool checkValidationLayerSupport();


	//создать утилы месседжера отладки
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	
	//разрушить утилы месседжера отладки
	void DestroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);




