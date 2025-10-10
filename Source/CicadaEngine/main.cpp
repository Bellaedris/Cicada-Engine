#include <vulkan/vulkan.h>
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <vector>
#include <algorithm>
#include <map>

#pragma region Constants
constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers
        {
            "VK_LAYER_KHRONOS_validation",
        };

const std::vector<const char*> mandatoryDeviceExtensions = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

#pragma endregion Constants

class HelloTriangleApplication
{
public:
    void run()
    {
        InitWindow();
        InitVulkan();
        MainLoop();
        Cleanup();
    }

private:
    #pragma region Members
    GLFWwindow* m_window;

    vk::raii::Context m_context; // the context is automatically created by the raii constructor
    vk::raii::Instance m_instance {nullptr};
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger {nullptr};
    vk::raii::SurfaceKHR m_surface {nullptr};
    vk::raii::PhysicalDevice m_physicalDevice {nullptr};
    vk::raii::Device m_device {nullptr};
    vk::raii::Queue m_graphicsQueue {nullptr};
    vk::raii::Queue m_presentQueue {nullptr};
    vk::raii::SwapchainKHR m_swapChain {nullptr};
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat{vk::Format::eUndefined};
    vk::Extent2D m_swapChainExtent;
    #pragma endregion Members

    #pragma region Methods
    #pragma region Statics
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
            vk::DebugUtilsMessageTypeFlagsEXT type,
            const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
    {
        std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

        return vk::False;
    }
    #pragma endregion Statics

    void SetupDebugMessenger()
    {
        if(!enableValidationLayers)
            return;

        vk::DebugUtilsMessageSeverityFlagsEXT  severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
        );

        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
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

    void CreateInstance()
    {
        constexpr vk::ApplicationInfo appInfo {
            .pApplicationName = "Cicada Engine",
            .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
            .pEngineName = "No Engine",
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
        if(enableValidationLayers)
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

    std::vector<const char*> GetRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if(enableValidationLayers)
        {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }

        return extensions;
    }

    void PickPhysicalDevice()
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

    void CreateLogicalDevice()
    {
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();

        // TODO eventually refactor this to allow for selection of a present-only queue if no queue supports both graphics/present
        // overall try to separate the errors to pinpoint where the issue arose (graphics or present?)

        // get the first index into queueFamilyProperties which supports graphics
        uint32_t graphicsIndex = -1;
        for (int i = 0; i < queueFamilyProperties.size(); i++)
        {
            if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) && m_physicalDevice.getSurfaceSupportKHR(i, m_surface))
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

        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain =
        {
            {},                               // no feature from physicalDeviceFeature2
            { .dynamicRendering = true },     // enable Vulkan13 dynamic rendering
            { .extendedDynamicState = true, } // enable extendedDynamicState from extension
        };

        vk::DeviceCreateInfo deviceCreateInfo =
        {
            .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount =  1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(mandatoryDeviceExtensions.size()),
            .ppEnabledExtensionNames = mandatoryDeviceExtensions.data()
        };

        m_device = vk::raii::Device(m_physicalDevice, deviceCreateInfo);

        m_graphicsQueue = vk::raii::Queue(m_device, graphicsIndex, 0);
        m_presentQueue = vk::raii::Queue(m_device, graphicsIndex, 0);
    }

    void CreateSurface()
    {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(*m_instance, m_window, nullptr, &surface) != 0)
        {
            throw std::runtime_error("Couldn't create window surface.");
        }
        m_surface = vk::raii::SurfaceKHR(m_instance, surface);
    }

    vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
    {
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    /**
     * Returns the most suitable present mode from a list of available ones in a surface attached to a device
     * @param availablePresentModes a list of all present modes compatible with this device/surface combination
     * @return Mailbox if found, fifo otherwise
     */
    vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
    {
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox)
                return availablePresentMode;
        }
        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;

        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);

        return
        {
            std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
    }

    void CreateSwapChain()
    {
        vk::SurfaceCapabilitiesKHR surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface);
        vk::SurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(m_physicalDevice.getSurfaceFormatsKHR(m_surface));
        vk::Extent2D extent = ChooseSwapExtent(surfaceCapabilities);
        unsigned int swapChainImageCount = std::max(3u, surfaceCapabilities.minImageCount);
        // maxImageCount == 0 means that there is no upper limit
        swapChainImageCount = surfaceCapabilities.maxImageCount == 0 ? swapChainImageCount : std::min(swapChainImageCount, surfaceCapabilities.maxImageCount);

        vk::SwapchainCreateInfoKHR swapchainCreateInfo
        {
            .flags = vk::SwapchainCreateFlagsKHR(),
            .surface = m_surface,
            .minImageCount = swapChainImageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1, // always 1 unless steroscopic 3D
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive, // change if graphics & present queues are different (to concurrent). Alternatively, keep exclusive but we will have to move ownerships
            .queueFamilyIndexCount = 0, // change if graphcis & present are different (change to 2)
            .pQueueFamilyIndices = nullptr, // change to an array of the queue indices if graphics & present are different
            .preTransform = surfaceCapabilities.currentTransform,
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .clipped = true,
            .oldSwapchain = nullptr // pass the old swapchain in case of recreation, for instance when changing resolution
        };

        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;

        m_swapChain = vk::raii::SwapchainKHR(m_device, swapchainCreateInfo);
        m_swapChainImages = m_swapChain.getImages();
    }
    #pragma endregion Methods

    #pragma region Lifecycle
    void InitWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(WIDTH, HEIGHT, "Cicada", nullptr, nullptr);
    }

    void InitVulkan()
    {
        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapChain();
    }

    void MainLoop()
    {
        while(!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
        }
    }

    void Cleanup()
    {
        glfwDestroyWindow(m_window);

        glfwTerminate();
    }
    #pragma endregion Lifecycle
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}