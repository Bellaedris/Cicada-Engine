//
// Created by Bellaedris on 12/11/2025.
//

#include "Device.h"

#include <algorithm>

namespace cica::gpu
{
    #pragma region static Methods
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
            vk::DebugUtilsMessageTypeFlagsEXT type,
            const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
    {
        std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

        return vk::False;
    }
    #pragma endregion staticMethods

    Device::Device(bool useValidationLayers, const char *appName)
        : m_enableValidationLayers(useValidationLayers)
    {
        CreateInstance(useValidationLayers, appName);
        if(useValidationLayers)
            CreateDebugMessenger();

        PickPhysicalDevice();
        CreateLogicalDevice();
    }

    void Device::CreateInstance(bool useValidationLayers, const char* appName)
    {
        vk::ApplicationInfo appInfo {
                .pApplicationName = appName,
                .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
                .pEngineName = "Cicada Engine",
                .engineVersion = VK_MAKE_VERSION(0, 0, 1),
                .apiVersion = vk::ApiVersion14
        };

        std::vector<const char*> requiredExtensions = GetRequiredExtensions();
        uint32_t extensionsCount = requiredExtensions.size();

        // retrieve the available extensions and ensure we have the ones required by glfw
        std::vector<vk::ExtensionProperties> extensionProperties =  m_context.enumerateInstanceExtensionProperties();
        for(uint32_t i = 0; i < extensionsCount; i++)
        {
            if(std::ranges::none_of(extensionProperties,
                                    [glfwExtension = requiredExtensions[i]](auto const& extensionProperty)
                                    { return strcmp(glfwExtension, extensionProperty.extensionName) == 0; }))
            {
                throw std::runtime_error("Required GLFW extension not supported: " + std::string(requiredExtensions[i]));
            }
        }

        // retrieve the available layers and ensure it includes the validation layers we need
        std::vector<const char*> requiredLayers;
        if(useValidationLayers)
        {
            requiredLayers.assign(validationLayers.begin(), validationLayers.end());
        }

        std::vector<vk::LayerProperties> layerProperties = m_context.enumerateInstanceLayerProperties();
        for(const char* layer : requiredLayers)
        {
            if(std::ranges::none_of(layerProperties,
                                    [requiredLayer = layer](auto const& layerProperty)
                                    { return std::strcmp(requiredLayer, layerProperty.layerName) == 0; }))
            {
                throw std::runtime_error("Required layer " + std::string(layer) + "not supported");
            }
        }

        vk::InstanceCreateInfo createInfo {
                .pApplicationInfo = &appInfo,
                .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
                .ppEnabledLayerNames = requiredLayers.data(),
                .enabledExtensionCount = extensionsCount,
                .ppEnabledExtensionNames = requiredExtensions.data(),
        };

        m_instance = vk::raii::Instance(m_context, createInfo);
    }

    std::vector<const char *> Device::GetRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if(m_enableValidationLayers)
        {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }

        return extensions;
    }

    void Device::CreateDebugMessenger()
    {
        vk::DebugUtilsMessageSeverityFlagsEXT  severityFlags(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
        );

        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
        );

        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT
        {
            .messageSeverity = severityFlags,
            .messageType = messageTypeFlags,
            .pfnUserCallback = &DebugCallback
            //.pUserData = could be used to pass a pointer to arbitrary data, for instance our renderer class, or something
        };

        m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    }

    void Device::PickPhysicalDevice()
    {
        std::vector<vk::raii::PhysicalDevice> devices = m_instance.enumeratePhysicalDevices();

        if (devices.empty())
            throw std::runtime_error("Couldn't find a GPU with Vulkan support.");

        std::multimap<int, vk::raii::PhysicalDevice> devicesScores;
        for (const auto& device : devices)
        {
            // we want our devices to handle graphics queue families, a few mandatory extensions and
            // maximize a score
            std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

            bool hasMinimalCapabilities = true;
            // check that we handle graphics queues
            const auto queueFamilyPropertiesIter = std::ranges::find_if(queueFamilies, [](vk::QueueFamilyProperties const &qfp)
            {
                return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
            });
            if (queueFamilyPropertiesIter == queueFamilies.end())
            {
                hasMinimalCapabilities = false;
            }

            // check that we handle mandatory graphics extensions
            std::vector<vk::ExtensionProperties> extensionProperties = device.enumerateDeviceExtensionProperties();
            bool hasAllExtensions = true;
            for (const auto& extension : mandatoryDeviceExtensions)
            {
                auto foundProperty = std::ranges::find_if(extensionProperties, [extension](auto const & ext)
                {
                    return strcmp(ext.extensionName, extension) == 0;
                });
                hasAllExtensions = hasAllExtensions && foundProperty != extensionProperties.end();
            }
            if (!hasAllExtensions)
            {
                hasMinimalCapabilities = false;
            }

            if (!hasMinimalCapabilities)
            {
                devicesScores.insert({0, device});
                continue;
            }

            // compute a device score
            vk::PhysicalDeviceProperties properties = device.getProperties();
            vk::PhysicalDeviceFeatures features = device.getFeatures();

            uint32_t score = 0;
            if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                score += 1000;
            }

            score += properties.limits.maxImageDimension2D;
            devicesScores.insert({score, device});
        }

        if (devicesScores.rbegin()->first > 0)
        {
            m_physicalDevice = vk::raii::PhysicalDevice(devicesScores.rbegin()->second);
            std::cout << "selected device: " << m_physicalDevice.getProperties().deviceName << std::endl;
        }
        else
            throw std::runtime_error("Couldn't find a suitable device");
    }

    void Device::CreateLogicalDevice()
    {
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();

        // TODO eventually refactor this to allow for selection of a present-only queue if no queue supports both graphics/present
        // overall try to separate the errors to pinpoint where the issue arose (graphics or present?)

        // get the first index into queueFamilyProperties which supports graphics
        uint32_t graphicsIndex = -1;
        for (int i = 0; i < queueFamilyProperties.size(); i++)
        {
            if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) /*&& m_physicalDevice.getSurfaceSupportKHR(i, m_surface)*/)
            {
                graphicsIndex = i;
                break;
            }
        }
        if (graphicsIndex == -1)
        {
            throw std::runtime_error("Couldn't find a queue for graphics and present.");
        }

        float queuePriority = 0.f;
        vk::DeviceQueueCreateInfo queueCreateInfo =
                {
                        .queueFamilyIndex = graphicsIndex,
                        .queueCount = 1,
                        .pQueuePriorities = &queuePriority,
                };

        uint32_t transferIndex = FindTransferOnlyQueue(queueFamilyProperties);
        vk::DeviceQueueCreateInfo transferQueueCreateInfo
                {
                        .queueFamilyIndex = transferIndex,
                        .queueCount = 1,
                        .pQueuePriorities = &queuePriority,
                };

        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain =
                {
                        {},                               // no feature from physicalDeviceFeature2
                        { .synchronization2 = true, .dynamicRendering = true },     // enable Vulkan13 dynamic rendering
                        { .extendedDynamicState = true, } // enable extendedDynamicState from extension
                };

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos =
                {
                        queueCreateInfo,
                        transferQueueCreateInfo
                };

        vk::DeviceCreateInfo deviceCreateInfo =
                {
                        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
                        .queueCreateInfoCount =  2,
                        .pQueueCreateInfos = queueCreateInfos.data(),
                        .enabledExtensionCount = static_cast<uint32_t>(mandatoryDeviceExtensions.size()),
                        .ppEnabledExtensionNames = mandatoryDeviceExtensions.data()
                };

        m_device = vk::raii::Device(m_physicalDevice, deviceCreateInfo);

        // just for info, queueIndex is the index of the queue inside the group of queues that match the queue family index
        m_graphicsQueue = vk::raii::Queue(m_device, graphicsIndex, 0);
        m_presentQueue = vk::raii::Queue(m_device, graphicsIndex, 0);
        m_transferQueue = vk::raii::Queue(m_device, transferIndex, 0);
        m_graphicsQueueIndex = graphicsIndex;
        m_transferQueueIndex = transferIndex;
    }

    int Device::FindTransferOnlyQueue(const std::vector<vk::QueueFamilyProperties> &queueFamilies)
    {
        // TODO a better way to find specialized queues like that would be to find the queue that has the desired flag, with
        // the lowest flag value as possible (with as few bits as possible).
        uint32_t transferIndex = -1;
        for (int i = 0; i < queueFamilies.size(); i++)
        {
            if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer &&
                (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) == static_cast<vk::QueueFlagBits>(0) &&
                (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) == static_cast<vk::QueueFlagBits>(0)
                    )
            {
                transferIndex = i;
                break;
            }
        }
        if (transferIndex == -1)
        {
            throw std::runtime_error("Couldn't find a queue for transfer only.");
        }

        return transferIndex;
    }

    std::unique_ptr<Buffer> Device::CreateBuffer(uint64_t size, Buffer::BufferUsage usage)
    {
        std::vector<uint32_t> concurrentQueues = {m_graphicsQueueIndex, m_transferQueueIndex};
        return std::make_unique<Buffer>(this, concurrentQueues, size, usage);
    }

    std::unique_ptr<Surface> Device::CreateSurface(cica::Window *window)
    {
        return std::make_unique<Surface>(this, window);
    }

} // cica::gpu