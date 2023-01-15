#include <iostream>
#include <stdexcept>//for runtime_error
#include <functional>//for lambda
#include <cstdlib>//for macros EXIT
#include <vector>
#include <array>
#include <optional>//wraps object
#include <set>
#if VKBASE_SHADER_RUNTIME_COMPILE
    #include <shaderc/shaderc.hpp>
#endif

#include "EngineBase.h"


namespace vkbase
{
    void EngineBase::init(ISysRes& sysRes)
    {
        #if VKBASE_SHADER_RUNTIME_COMPILE
            std::cout << "Shader runtime compile enabled" << std::endl;
        #endif

        this->sysRes=&sysRes;
        createInstance();
        setupDebugMessenger();
        surface= this->sysRes->createSurface(instance);
        pickPhysicalDevice();
        queueFamilyIndices = findQueueFamilies(physicalDevice);
        createLogicalDevice();
        createCommandPool();
        createCommandPoolRest();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createFrameBuffers();//uses render pass
        createSyncObjects();
        setupLogic();
    }


    inline void EngineBase::createInstance()
    {
        if constexpr (enableValidationLayers)
            if(!checkValidationLayerSupport())
                throw std::runtime_error("validation layers requested, but not available!");

        VkApplicationInfo appInfo = {};

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;


        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;


        auto extensionsGLWF = sysRes->getRequiredInstanceExtensions();

        if constexpr (enableValidationLayers)
            extensionsGLWF.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);//макрос=«VK_EXT_debug_utils»

        //предает кол-во расширений
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsGLWF.size());
        //и массив с ними
        createInfo.ppEnabledExtensionNames = extensionsGLWF.data();


        //if in Releaze
        if (enableValidationLayers)
        {
            //добавить слои из массива
            //count
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            //prt on array
            createInfo.ppEnabledLayerNames = validationLayers.data();

            //мессенжер отладки
            //структура для инфы
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
            //заполнение структуры
            populateDebugMessengerCreateInfo(debugCreateInfo);
            //передача информации месседжера отладки как pNext параметр
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
        } else
        {//иначе ничего не добавлять
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }


        //создание instance если прошло не успешно генерация исключения
        if (vkCreateInstance(&createInfo/*info*/, nullptr/* пользовательские ответные реакции*/,
                             &instance/*дескриптор на выходе*/) != VK_SUCCESS)
            throw std::runtime_error("Crate insanse failed");


        /*свойства РАСШИРЕНИй api vulkan*/
        uint32_t extensionCount;
        //запрашивает разширения но получает только их количество
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        //создает масив для получения исключений
        auto *extensions = new VkExtensionProperties[extensionCount];
        //опять запрашивает расширения и их количество и записывает в массив
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);
        //вывод
        std::cout << "available extensions:" << std::endl;
        for (uint32_t i = 0; i < extensionCount; ++i)
        {
            std::cout << "\t" << i << ")" << extensions[i].extensionName << std::endl;
        }
        delete[] extensions;
        /*РАСШИРЕНИЯ*/
    }

    //получить физическое устойство
    inline void EngineBase::pickPhysicalDevice()
    {
        //количество видеокарт
        uint32_t deviceCount= 0;
        //запрос количества видеокарт (пока без данных)
        vkEnumeratePhysicalDevices(instance/*дескриптор vulkan*/, &deviceCount/*кол-во(_out)*/, nullptr/*массив*/);
        //если кол-во видеокарт поддерживающих vulkan =0 то
        //продолжать нет смысла, но можна выкинуть исключение и завершить программу
        if (deviceCount == 0)
            throw std::runtime_error("failed to find GPUs with Vulkan support! ");
        //ИНИЦИАЛИЗАЦИЯ МАССИВА ДЛЯ ХРАНЕНИЯ ДОСТУПНЫХ ФИЗИЧЕСКИХ УСТРОЙСТВ (ГП)
        std::vector<VkPhysicalDevice> devices(deviceCount);
        //запрос данных
        vkEnumeratePhysicalDevices(instance/*дескриптор vulkan*/, &deviceCount/*кол-во (_out)*/,
                                   devices.data()/*массив(_out)*/);

        //теперь необходимо их проверить
        for (VkPhysicalDevice &physDevice: devices)
        {
            //берем первый девайс подходящий под заданые в isDeviseSuitable условия
            if (isDeviseSuitable(physDevice))
            {
                physicalDevice = physDevice;
                return;
            }
        }
        //если return не вызван то значит ни одно устройство не соответствует параметрам
        //и это означает. что нужно сгенерировать исключение и завершить прогу
        throw std::runtime_error("Physical devise has found, but do not corresponds to the parameters!");
    }

    //проверка девайса(phys devise or GP) на необходимые условия
    inline bool EngineBase::isDeviseSuitable(VkPhysicalDevice &physDevise)
    {
        //обьявление оьекта структуры для хранения свойств физического устройства
        VkPhysicalDeviceProperties deviceProperties;
        //получение свойств физического устройства
        vkGetPhysicalDeviceProperties(physDevise,/*out*/ &deviceProperties);

        //для дополнительных характеристик* используется отдельная структура
        VkPhysicalDeviceFeatures deviceFeatures;
        //запрос характеристик у девайса
        vkGetPhysicalDeviceFeatures(physDevise, /*out*/&deviceFeatures);

        //запрос индексов семейств очередей
        QueueFamilyIndices indices = findQueueFamilies(physDevise);

        //проверка адэкватности цепей обмена
        bool swapChainAdequate = false;
        //если все расширения поддерживаются (иначе может вызвать ошибку)
        //проверка совместимости устройства и поверхности для работы цепей обмена
        if (checkDeviceExtensionSupport(physDevise))
        {
            //тогда запрашиваю детали поддержки
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physDevise);
            //и если доступен хотя бы 1 формат изображения и  режим представления то все ок
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        //если все очереди найдены, необходимые расширения поддерживаются и цепи смогут работать то считать устройство пригодным
        return indices.isComplete() && checkDeviceExtensionSupport(physDevise) && swapChainAdequate;
    }

    //проверка расширений физического устойства

    inline bool EngineBase::checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        //запрос расширений у устройства
        uint32_t extensionCount;
        //количество
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        //массив
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        //запись в массив
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
        //сверяюсь со списком
        for (const char *regExtension: deviceExtensions)
        {
            bool found{false};
            for (VkExtensionProperties &foundExtension: availableExtensions)
                if (strcmp(foundExtension.extensionName, regExtension) == 0)
                {
                    found = true;
                    break;
                }
            //если это рассширение не найдено то вернуть false
            if (!found)
                return false;
        }

        return true;
    }


    //создание логического устойсва
    inline void EngineBase::createLogicalDevice()
    {
        //задание множества из необходимых семейств очередей(повторов не будет)
        std::set<uint32_t> uniqueQueueFamilies{queueFamilyIndices.graphicsFamily.value(),
                                               queueFamilyIndices.presentFamily.value()};

        //массив структур данных про очереди для логического устройства(для каждой очереди отдельная стуктура)
        VkDeviceQueueCreateInfo *queueCreateInfos = new VkDeviceQueueCreateInfo[uniqueQueueFamilies.size()]{};
        //приоритет очередей(0.0-1.0) даже если очередь одна
        float queuePriority = 1.0f;

        int i{0};
        for (uint32_t queueFamily: uniqueQueueFamilies)
        {
            //тип структуры
            queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            //передача индекса семейства очереди на физическом устройстве
            queueCreateInfos[i].queueFamilyIndex = queueFamily;
            //количество очередей этого семейства(типа), которых необходимо создать
            queueCreateInfos[i].queueCount = 1;
            //принимает массив приоритетов для каждой из очередей( количество которых задано в queueCount)
            queueCreateInfos[i].pQueuePriorities = &queuePriority;
            ++i;
        }

        //набор функций устройсва пока пусто!!
        VkPhysicalDeviceFeatures deviceFeatures = {.shaderFloat64=true};


        //теперь двумя предидущими заполняем основную
        VkDeviceCreateInfo createInfo = {};
        //тип
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        //количество семейств очередей
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(uniqueQueueFamilies.size());
        //указатель на массив структур про очереди
        createInfo.pQueueCreateInfos = queueCreateInfos;
        //включеные функции
        createInfo.pEnabledFeatures = &deviceFeatures;
        //включение расширений
        //количество расширений для логического устройства
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        //сами разширения
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        //если включены слои проверки
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else
        {
            createInfo.enabledLayerCount = 0;
        }

        //ну и теперь можно создать логическое устойство!
        if (vkCreateDevice(physicalDevice/*дескриптор физического устройства*/,
                           &createInfo/*структура с данными о логическом устройстве*/,
                           nullptr/*указатель обратных вызовов*/,
                           &logicalDevice/*будующий дескриптор логического устройства*/) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }
        //получение дескриптора графической очереди логического устройства
        vkGetDeviceQueue(logicalDevice, queueFamilyIndices.graphicsFamily.value(),
                         0/*индекс очереди(если создано больше 1)*/, &graphicsQueue);
        //получение дескриптора очереди поверхности логического устройства
        vkGetDeviceQueue(logicalDevice, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
    }

    //создание поверхности




    //задает значения, логическое и физическое устройство для всех буфферов



    //SETUP SWAP CHAIN

    void EngineBase::recreateSwapChain()
    {
        //frame buffer can not be zero
        if (sysRes->getWidth() == 0 || sysRes->getHeight() == 0)
            return;

        //wait present queue
        vkQueueWaitIdle(presentQueue);

        //cleanup existed swap chain
        cleanupSwapChain();

        //create again
        createSwapChain();


        createImageViews();

        createFrameBuffers();

        //для отладки
        std::cout << "Swap chain was recreated\n";
    }


    inline void EngineBase::cleanupSwapChain()
    {
        //delete framebuffers
        for (auto framebuffer: swapChainFramebuffers)
        {
            vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
        }

        //объект для просмотра изображений цепи
        for (auto imageView: swapChainImageViews)
        {
            vkDestroyImageView(logicalDevice, imageView, nullptr);
        }
        //уничтожение цепи
        vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
    }


    inline void EngineBase::createSwapChain()
    {
        //запрос деталей
        SwapChainSupportDetails swapChainDetalis = querySwapChainSupport(physicalDevice);
        //из запрошеных данных выбор формата цвета
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainDetalis.formats);
        //режима презентации
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainDetalis.presentModes);
        //размер изображения
        extent = chooseSwapExtent(swapChainDetalis.capabilities);

        //следующий параметр- это минимальное количество кадров в цепочке обмена
        /*swapChainDetalis.capabilities.minImageCount-минимальное доступное количество кадров в цепи
        swapChainDetalis.capabilities.maxImageCount-максимальное количество кадров в цепи(0 означает что ограничений нет)*/
        //установливаю на 1 кадр больше минимального (скорее всего это 2)
        uint32_t imageCount = MAX_FRAMES_IN_FLIGHT;
        //проверка не меньше ли значение минимального
        if (imageCount <= swapChainDetalis.capabilities.minImageCount)
            imageCount = swapChainDetalis.capabilities.minImageCount + 1;
        //прорверка на привышение макс значения
        if (swapChainDetalis.capabilities.maxImageCount > 0 && imageCount > swapChainDetalis.capabilities.maxImageCount)
        {
            imageCount = swapChainDetalis.capabilities.maxImageCount;
        }

        //заполнение структуры
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        //поверхность
        createInfo.surface = surface;
        //количество изображений
        createInfo.minImageCount = imageCount;
        //формат цвета
        createInfo.imageFormat = surfaceFormat.format;
        //цветовое пространство
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        //расширение изображения
        createInfo.imageExtent = extent;
        //количество слоев
        createInfo.imageArrayLayers = 1;
        //операции с изображением(для каких целей нужно это изображение)
        /*• VK_IMAGE_USAGE_TRANSFER_SRC_BIT указывает, что изображение может использоваться в качестве источника команды передачи.
        • VK_IMAGE_USAGE_TRANSFER_DST_BIT указывает, что изображение может использоваться в качестве пункта назначения команды передачи.
        • VK_IMAGE_USAGE_SAMPLED_BIT указывает, что изображение можно использовать для создания VkImageView,
        подходящего для заполнения слота VkDescriptorSet, типа VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
        или VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, а также
        • VK_IMAGE_USAGE_STORAGE_BIT указывает,  что изображение можно использовать для
        создания VkImageView, подходящего для заполнения слота VkDescriptorSet
        типа VK_DESCRIPTOR_TYPE_STORAGE_IMAGE.
        • VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT указывает, что изображение можно использовать для создания VkImageView,
        подходящего для использования в качестве цветного или разрешающего вложения в VkFramebuffer.
        • VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT указывает, что изображение можно использовать для
        создания VkImageView, подходящего для использования в качестве вложения для определения глубины /
        трафарета или глубины / трафарета в VkFramebuffer.
        • VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT указывает,
        что память, привязанная к этому изображению, будет выделена с помощью VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT
        (см. Распределение памяти для более подробной информации). Этот бит можно установить для любого изображения,
        которое можно использовать для создания VkImageView, подходящего для использования в качестве цвета, разрешения,
        глубины / трафарета или входного вложения.
        • VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT указывает, что изображение можно использовать для создания
        VkImageView, подходящего для заполнения слота VkDescriptorSet типа VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        быть прочитанным из шейдера как входное вложение; и использоваться в качестве входного вложения в кадровом буфере.*/
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        //нужно указать, как обрабатывать образы цепочки подкачки
        /* //Мы будем рисовать изображения в цепочке подкачки из графической очереди, а затем отправлять их в очередь презентации
        (обе очереди взаимодействуюю с изображением, если эти обе являются одной и той же очередью то нет проблем
        но если это разные очереди передача изображения между ними достаточно сложна и тогда лучше использовать режим конкурентности,aa
        когда обе очереди могут одновременно взаимодействовать с элементом цепи, что не приведет к ошибкам но также имеет
        меньшую производительность чем линейный режим где одновременно только
        одна очередь может взаимодействовать с изображением цепи обмена)*/
        //запрос семейств очередей

        //запись индексов в масив
        uint32_t queueFamilyIndicesArray[] = {queueFamilyIndices.graphicsFamily.value(),
                                              queueFamilyIndices.presentFamily.value()};
        //если очереди являются одной и той же очередью
        if (queueFamilyIndices.graphicsFamily == queueFamilyIndices.presentFamily)
        {
            //изображение пренадлежит одному семейству очередей и право собствености должно быть явно переданоперед использованием в другой очереди

            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            //так как очередь лишь одна то нет необходимости передавать массив между которыми будет менятся
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }
            //если это разные очереди
        else
        {
            // Изображения могут использоваться в нескольких семействах очередей без явной передачи прав собственности.(меньше производительность)
            //можно использовать минимум с двумя очередями
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            //в этом (паралельном режиме необходимо указать между какими очередями может передаватся право собствености)
            //количество очередей
            createInfo.queueFamilyIndexCount = 2;
            //массив
            createInfo.pQueueFamilyIndices = queueFamilyIndicesArray;
        }

        //трансформация изображения(поворот и отражение кадра)
        //если она не нужна то оставить текущее преобразование
        createInfo.preTransform = swapChainDetalis.capabilities.currentTransform;
        ////альфа канал оконной системы(прозрачность винды, сейчас отключено)
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        //создание цепи обмена(уничтожение в cleanup)
        if (vkCreateSwapchainKHR(logicalDevice /*логическое устройство*/, &createInfo/*заполненая инфа*/, nullptr,
                                 &swapChain/*указатель на переменную для хранения дескриптора*/) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swap chain!");
        }

        //получение массива дескрипторов изображений из цепи
        //запрос колчества
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
        //запись в массив swapChainImages
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());

        //сохранение формата и экстэнта
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;

        //изменение константы на фактическое значение кадров в цепочке
        const_cast<int &>(MAX_FRAMES_IN_FLIGHT) = imageCount;
    }

    //запросс деталей поддержки цепей обмена
    EngineBase::SwapChainSupportDetails EngineBase::querySwapChainSupport(VkPhysicalDevice device)
    {
        //общая структура цепей обмена
        SwapChainSupportDetails details;

        //запрос сразу в структуру возможностей поверхности
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        //запрос форматов поверхности
        uint32_t formatCount;
        //кол-во
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        //если кол-во форматв не рано 0 то запись в измененный вектор
        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        //запрос режимов презентации
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }
        //возврат
        return details;
    }

    //выбор формата

    inline VkSurfaceFormatKHR
    EngineBase::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        //поиск необходимого формата
        for (VkSurfaceFormatKHR format: availableFormats)
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return format;
        //иначе поставить первый попавшийся
        return availableFormats[0];
    }

    //режим презентации

    inline VkPresentModeKHR
    EngineBase::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        //сначала поиск этого режима среди доступных
        for (auto &availablePresentMode: availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }


        //если же не найден установить болеее простой режим
        return VK_PRESENT_MODE_FIFO_KHR;

        //все режимы
        /*
        *-VK_PRESENT_MODE_IMMEDIATE_KHR: Изображения, отправленные вашим приложением,
        сразу же переносятся на экран, что может привести к их разрыву.
        *-VK_PRESENT_MODE_FIFO_KHR : Цепочка обмена - это очередь, в которой дисплей берет
        изображение с начала очереди, когда обновляется дисплей, и программа вставляет визуализированные
        изображения в конец очереди.Если очередь заполнена, то программа должна ждать.Это наиболее похоже
        на вертикальную синхронизацию, как в современных играх.Момент обновления дисплея известен как «вертикальный пробел».
        *-VK_PRESENT_MODE_FIFO_RELAXED_KHR : Этот режим отличается от предыдущего только в том случае,
        если приложение опаздывает и очередь пуста на последнем вертикальном бланке.Вместо того,
        чтобы ждать следующего вертикального пробела, изображение передается сразу же после его
        окончательного поступления.Это может привести к заметному разрыву.
        *-VK_PRESENT_MODE_MAILBOX_KHR : Это еще один вариант второго режима.Вместо того,
        чтобы блокировать приложение, когда очередь заполнена, изображения, которые уже
        поставлены в очередь, просто заменяются более новыми.Этот режим можно использовать
        для реализации тройной буферизации, что позволяет избежать разрыва со значительно меньшими
        задержками, чем стандартная вертикальная синхронизация, которая использует двойную буферизацию.
        *///
    }

    //размер(расширение ) цепей обмена(данная функция используется для расширения используемого изображения которое будет натянуто на экран)

    inline VkExtent2D EngineBase::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
    {
        //если ширина адекватная значит возвращаем так как есть
        if (capabilities.currentExtent.width != UINT32_MAX)
            return capabilities.currentExtent;

        //в противном случае это означает что можно выставить пределы (зависит от оконных менеджеров)
        VkExtent2D actualExtent = {};
        //большее из (минимальной доступной ширины и меньшего из(максимально доступной ширины и шириной окна ))
        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width, static_cast<uint32_t>(sysRes->getWidth())));
        //тоже что и с шириной если высота экрана между макс и мин то так и оставляю если же больше макс то макс, если меньше мин то мин
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, static_cast<uint32_t>(sysRes->getHeight())));

        return actualExtent;
    }

    //создание обьектов для просмотра изображений

    inline void EngineBase::createImageViews()
    {
        //установка размера равного количеству изображений в цепи
        swapChainImageViews.resize(swapChainImages.size());
        //интерация по цепи
        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            //структура с инфой о imageView
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            //изображение
            createInfo.image = swapChainImages[i];
            //трактовать как 1D текстуры, 2D текстур, 3D текстур и карт куба.
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            //установка ранее полученого формата
            createInfo.format = swapChainImageFormat;
            //некие цветовые приобразование(установлены по умолчанию)(!!!не все понятно!!!*******************)
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            //хз связано со стереографическим 3D-приложением(для 2 глаз)
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            //создание оъекта для просмотра изображений из цепи обмена( очистка в cleanup)
            if (vkCreateImageView(logicalDevice, &createInfo/*заполненая инфа*/, nullptr,
                                  &swapChainImageViews[i]/*дескрипто out*/) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

#if DEPTH_BUFFER
    void EngineBase::createDepthResources()
    {
        depthImages.resize(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            depthImages[i].create(
                reinterpret_cast<RTEngine*>( this),
                WIDTH,
                HEIGHT,
                VK_FORMAT_D32_SFLOAT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT| VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT,
                0,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_LAYOUT_UNDEFINED
                );
    }

#endif



    //Создание кадровых буферов

    inline void EngineBase::createFrameBuffers()
    {

        //размер соответствующий количеству кадров в цепи
        swapChainFramebuffers.resize(MAX_FRAMES_IN_FLIGHT);


        //проход по изображениям и создание буффера
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            //прикрепление изображения

            VkImageView attachments[] = {swapChainImageViews[i]
#if DEPTH_BUFFER
                    ,depthImages[i].imageView
#endif
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            //передача настроек прохода рендеринга каждому буфферу
            framebufferInfo.renderPass = renderPass;
            //количество изображений в буффере
            framebufferInfo.attachmentCount = sizeof(attachments) / sizeof(VkImageView);
            //изображения в оболочке
            framebufferInfo.pAttachments = attachments;
            //размеры изображения
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            //количество слоев изображений
            framebufferInfo.layers = 1;
            //создание дескриптора
            if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }

    }

    //Создание командного пула

    inline void EngineBase::createCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        //очередь в которую буфер будет отправлять комманды(в данном случае графическая)
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        //флаги настроек буфферов
        /*Есть два возможных флага для командных пулов:
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Намек на то, что буферы команд перезаписываются с новыми командами очень часто (может изменить поведение выделения памяти)
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Разрешить перезапись буферов команд по отдельности, без этого флага все они должны быть сброшены вместе*/
        poolInfo.flags = 0; // Optional

        //создать коммандный пул (Poll will destroy in cleanup function)
        if (vkCreateCommandPool(logicalDevice/*логическое устройство*/, &poolInfo/*инфа*/, nullptr,
                                &commandPool/*ссылка на дескриптор пула*/) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool!");
    }

    //создать коммандный пул с поддержкой перезаписи буфферов без необходимости сброса всего пула
    void EngineBase::createCommandPoolRest()
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        poolInfo.flags =
                VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; // Optional
        if (vkCreateCommandPool(logicalDevice/*логическое устройство*/, &poolInfo/*инфа*/, nullptr,
                                &commandPoolRest/*ссылка на дескриптор пула*/) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool with rest flag!");
    }


    //создание семофоров

    inline void EngineBase::createSyncObjects()
    {
        //для каждого изображения в цепи обмена свой набор семафоров и забор
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

        fenceRenderFinished.resize(MAX_FRAMES_IN_FLIGHT);
        fenceImageAvailable.resize(MAX_FRAMES_IN_FLIGHT);

        //для создания семофоров необходима структура, но она пока не имеет обязательных параметров
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;


        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        //флаг устанавливает активное состояние забора по умолчанию что позволяет пройти превый раз
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            //создание двух массивов семафоров для кождого кадра в полете
            if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fenceRenderFinished[i]) != VK_SUCCESS||
                vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fenceImageAvailable[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create semaphores!");
            }
        }
    }


    //метод отрисовки окна
    void EngineBase::drawFrame()
    {
        static VkResult result;
        //current swap chain image index
        static uint32_t imageIndex;



//        //wait for fence
//        //if fence with index currentFrame is signaled
//        vkWaitForFences(logicalDevice, 1/*count*/, &fenceImageAvailable[currentFrame], VK_TRUE/*ждать все в массиве*/,
//                        UINT64_MAX);
//
//
//        //вернуть забор в первоначальное состояние(не активен, а значит не проходит сквозь vkWaitForFences пока не активируется )
//        vkResetFences(logicalDevice, 1, &fenceImageAvailable[imageIndex]);

        result = vkAcquireNextImageKHR(logicalDevice, swapChain,
                                       UINT64_MAX,
                                       imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
                                       &imageIndex);




        /* VK_ERROR_OUT_OF_DATE_KHR: Цепочка обмена стала несовместимой с поверхностью и больше не может использоваться для рендеринга.Обычно происходит после изменения размера окна.
        VK_SUBOPTIMAL_KHR : Цепочка обмена может по - прежнему может использоваться для успешного представления на поверхности, но свойства поверхности не полностю соответсявуют.*/
        //обработка результата операции получения изображения из цепи
        if (result == VK_ERROR_OUT_OF_DATE_KHR)//невозможно рисовать в текущую цепь
        {
            recreateSwapChain();
            return;
        }
            //если VK_SUCCESS то продолжать, если же условия почти оптимальны то дорисовать этот кадр и пересоздать цепь после его отправки
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            std::runtime_error("failed to acquire swap chain image!");



        //ждать актривации забора(возможно ждать сразу несолько передавая указ на массив)
        vkWaitForFences(logicalDevice, 1/*count*/, &fenceRenderFinished[imageIndex], VK_TRUE/*ждать все в массиве*/,
                        UINT64_MAX);
        //вернуть забор в первоначальное состояние(не активен, а значит не проходит сквозь vkWaitForFences пока не активируется )
        vkResetFences(logicalDevice, 1, &fenceRenderFinished[imageIndex]);

        //
        drawPrepare();


        ///RENDERING
        //struct for rendering buffer submission
        static VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .commandBufferCount = 1,
        };

        //wait for image available semaphore
        VkSemaphore waitSemaphores[]{imageAvailableSemaphores[currentFrame]};
        submitInfo.pWaitSemaphores = waitSemaphores;

        //command buffer to submit
        submitInfo.pCommandBuffers = &cbRender[imageIndex];

        //стадии конвеера на которой ждать семафоры(эти этапы не начнут выполнятся до срабатывания семафора)
        //(цвет не начнет записыватся пока изображение не получено и фрагментный шейдер выполнятся пока не обработаны вершины)
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT};

        //передача массива ожидающих стадий
        submitInfo.pWaitDstStageMask = waitStages;

        //количество семафоров которые активируются после выполнения рендеринга
        submitInfo.signalSemaphoreCount = 1;
        //массив переданих семафоров
        //(ожидают окончания рендеринга)
        submitInfo.pSignalSemaphores = &renderFinishedSemaphores[imageIndex];

        //отправка буффера команд(с настройками семафоров) в графическую очередь
        if (vkQueueSubmit(graphicsQueue/*очередь*/, 1/*к-во структур с буферами*/,
                          &submitInfo/*ptr on array of struct*/, fenceRenderFinished[imageIndex]/*забор*/) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        //структура настройки презентации
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        //какие семафоры ждать перед началом презентации
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphores[imageIndex];
        //цепь из которой рисовать
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        //индекс изобрадения который надо нарисовать
        presentInfo.pImageIndices = &imageIndex;
        //массив VkResult для кождой цепи
        presentInfo.pResults = nullptr; // Optional
        //и добавить изображение в очередь презентации


        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
            windowResized)//если не все в порядке то пересоздать
        {
            recreateSwapChain();
            windowResized = false;
        } else if (result !=
                   VK_SUCCESS)//если нет неполадок с совместимостю и при этом неудачно отправлено то генерировать исключение
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        //дожидается завершения работы(заставляет CPU ждать пока все изображения цепи не нарисуются)
        //это не всегда хорошо так как в этом случае только одно изображение может обрабатыватся одновременно
        // vkQueueWaitIdle(presentQueue);
        //переход к следующему набору семафоров для следующего кадра
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    //освобождение памяти и заверешение работы

    void EngineBase::cleanup()
    {
        //дождатся завершения операций логического устойства
        //(если этого не сделать и завершить выполнение программы произойдет ошибка)
        vkDeviceWaitIdle(logicalDevice);

        //отметить, что рисование закончилось
        //isDrawing = false;
#if DEPTH_BUFFER
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            depthImages[i].destroy();
#endif
        //уничтожить настройки рисования(pipelines)
        //destroy();


        //уничтожиить цепь и объекты связаные с ней
        cleanupSwapChain();

        //удалить проход рендера
        vkDestroyRenderPass(logicalDevice, renderPass, nullptr);




        //удаление семафоров и заборов
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
            //забор
            vkDestroyFence(logicalDevice, fenceRenderFinished[i], nullptr);
            vkDestroyFence(logicalDevice, fenceImageAvailable[i], nullptr);
        }

        //удаление коммандного пула
        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
        vkDestroyCommandPool(logicalDevice, commandPoolRest, nullptr);


        //уничтожение логического устройства
        vkDestroyDevice(logicalDevice, nullptr);
        //если слои проверки подключены то подчистить
        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
        //уничтожение поверхности
        vkDestroySurfaceKHR(instance, surface, nullptr);
        //разрушает дескриптор(экзэмпляр(instance))
        vkDestroyInstance(instance, nullptr /*обратный вызов некой функции, здесь не нужен*/);

    }

    //DEBUG
    inline VKAPI_ATTR VkBool32 VKAPI_CALL
    EngineBase::debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData
    )
    {
        //extract this(function is static)
        auto thiz = reinterpret_cast<EngineBase *>(pUserData);

        //write message depending on severity
        switch (messageSeverity)
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                thiz->sysRes->error(pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                thiz->sysRes->warning(pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                thiz->sysRes->info(pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                thiz->sysRes->info(pCallbackData->pMessage);
                break;
            default:
                break;
        }

        return VK_FALSE;
    }


    inline void EngineBase::setupDebugMessenger()
    {
        //if validation layers disabled - do nothing
        if (!enableValidationLayers)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);
        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger/*out handle*/) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to set up debug messenger!");
        }
    }

    void EngineBase::DestroyDebugUtilsMessengerEXT(
            VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator
    )
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }

    }

    VkResult EngineBase::CreateDebugUtilsMessengerEXT(
            VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
            const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger
    )
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    bool EngineBase::checkValidationLayerSupport()
    {
        //get vulkan available layers(currently installed on system)
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);//get size
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());//get data

        //check that all necessary layers are in the list
        for (const char* checkingLayers : validationLayers)
        {
            bool found= false;
            for (uint32_t i=0;i<layerCount;++i)
            {
                if (strcmp(checkingLayers, availableLayers[i].layerName) == 0)
                {
                    found= true;
                    break;
                }
            }
            if (!found)
                return false;
        }
        return true;
    }

    void EngineBase::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData=this;
    }

    VkShaderModule EngineBase::createShaderModule(const std::vector<char> &code)
    {
        if(code.size()%4!=0)
            throw std::runtime_error("Incorrect spir-v shader code size: "+std::to_string(code.size()));

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        //cast char array to uint32_t array pointer
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        //creating a module
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    }

    [[maybe_unused]] VkShaderModule EngineBase::loadPrecompiledShader(const std::string &filename)
    {
        std::vector<char> code=sysRes->load_file((filename+".spv").c_str());
        if (code.empty())
            throw std::runtime_error("failed to load precompiled shader file: "+filename+"spv");
        return createShaderModule(code);

    }


#if VKBASE_SHADER_RUNTIME_COMPILE
    VkShaderModule EngineBase::loadSourceShader(const std::string &filename, bool save)
    {
        std::vector<char> code=sysRes->load_file((filename).c_str());
        if (code.empty())
            throw std::runtime_error("failed to load source shader file: "+filename);

        std::vector<char> compiledCode=compileShader(code,filename);

        //save compiled shader

        return createShaderModule(compiledCode);
    }

    VkShaderModule EngineBase::loadShader(const std::string &filename)
    {
        return VK_NULL_HANDLE;
    }

    std::vector<char> EngineBase::compileShader(const std::vector<char>& source, const std::string &fileName)
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        //decide shader type by extension
        shaderc_shader_kind kind;
        std::string extension= fileName.substr(fileName.find_last_of('.') + 1);
        if (extension=="vert")
            kind=shaderc_shader_kind::shaderc_glsl_vertex_shader;
        else if (extension=="frag")
            kind=shaderc_shader_kind::shaderc_glsl_fragment_shader;
        else if (extension=="comp")
            kind=shaderc_shader_kind::shaderc_glsl_compute_shader;
        else if (extension=="tesc")
            kind=shaderc_shader_kind::shaderc_glsl_tess_control_shader;
        else if (extension=="tese")
            kind=shaderc_shader_kind::shaderc_glsl_tess_evaluation_shader;
        else if (extension=="geom")
            kind=shaderc_shader_kind::shaderc_glsl_geometry_shader;
        else if (extension=="rgen")
            kind=shaderc_shader_kind::shaderc_glsl_raygen_shader;
        else if (extension=="rint")
            kind=shaderc_shader_kind::shaderc_glsl_intersection_shader;
        else if (extension=="rahit")
            kind=shaderc_shader_kind::shaderc_glsl_anyhit_shader;
        else if (extension=="rchit")
            kind=shaderc_shader_kind::shaderc_glsl_closesthit_shader;
        else if (extension=="rmiss")
            kind=shaderc_shader_kind::shaderc_glsl_miss_shader;
        else if (extension=="rcall")
            kind=shaderc_shader_kind::shaderc_glsl_callable_shader;
        else if (extension=="mesh")
            kind=shaderc_shader_kind::shaderc_glsl_mesh_shader;
        else if (extension=="task")
            kind=shaderc_shader_kind::shaderc_glsl_task_shader;
        else if (extension=="spv")
            kind=shaderc_shader_kind::shaderc_spirv_assembly;
        else
            throw std::runtime_error("Unknown shader extension: "+extension);



        // Like -DMY_DEFINE=1
        //options.AddMacroDefinition( "MY_DEFINE", "1" );
        options.SetOptimizationLevel( shaderc_optimization_level_size );


        shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv( source.data() ,source.size(), kind, fileName.c_str(), options);

        if( module.GetCompilationStatus() != shaderc_compilation_status_success ) {
            throw std::runtime_error(module.GetErrorMessage());
        }

        //get compiled code as byte vector
        return std::vector<char>{reinterpret_cast<const char*>( module.cbegin()), reinterpret_cast<const char*>( module.cend()) };
    }


    EngineBase::QueueFamilyIndices EngineBase::findQueueFamilies(VkPhysicalDevice physicalDevice)
    {
        //output struct
        QueueFamilyIndices indices;

        //retrieve list of queue families
        uint32_t queueFamilyCount;

        //get count of queue families
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        //get array of queue families
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

        //iterating and checking properties
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            //check if queue family supports graphics operations
            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphicsFamily = i;

            //check if i-family supports presentation to surface
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

            if (presentSupport)
                indices.presentFamily = i;

            //if all properties are found, break
            if (indices.isComplete())
                break;
        }


        return indices;
    }

    uint32_t EngineBase::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkMemoryHeapFlags heapFlags)
    {
        //query available types memory on the device (gets an array of memory types and heaps)
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties/*out*/);


        //typeFilter bits in order from the end means under what indices the memory can be selected
        //the memory type must support all properties and belong to a heap with supported flags
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1 << i))&& (memProperties.memoryTypes[i].propertyFlags & properties) == properties &&
                (memProperties.memoryHeaps[memProperties.memoryTypes[i].heapIndex].flags && heapFlags) == heapFlags)
                return i;
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

#endif
}
