#include <vulkan/vulkan.h>
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <vector>
#include <algorithm>
#include <fstream>
#include <map>

#include <glm/glm.hpp>

#include "Buffer.h"

#pragma region Constants
constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

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
    vk::raii::Queue m_transferQueue {nullptr};
    uint32_t m_graphicsQueueIndex {-1u};
    uint32_t m_transferQueueIndex {-1u};
    vk::raii::SwapchainKHR m_swapChain {nullptr};
    std::vector<vk::Image> m_swapChainImages;
    vk::Format m_swapChainImageFormat{vk::Format::eUndefined};
    vk::Extent2D m_swapChainExtent;
    std::vector<vk::raii::ImageView> m_swapChainImageViews;
    vk::raii::PipelineLayout m_pipelineLayout {nullptr};
    vk::raii::Pipeline m_pipeline {nullptr};

    vk::raii::CommandPool m_commandPool {nullptr};
    vk::raii::CommandPool m_transferCommandPool {nullptr};
    std::vector<vk::raii::CommandBuffer> m_commandBuffers {};

    std::vector<vk::raii::Semaphore> m_presentCompleteSemaphores {};
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores {};
    std::vector<vk::raii::Fence> m_frameFences {};

    uint32_t m_currentFrame {0};
    uint32_t m_semaphoreIndex {0};
    bool m_framebufferResized {true};

    const std::vector<Vertex> vertices = {
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

    //triangle buffer
    Buffer m_triangleBuffer;
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

    static std::vector<char> ReadFile(const std::string& fileName)
    {
        std::ifstream file(fileName, std::ios::binary | std::ios::ate);

        if (file.is_open() == false)
            throw std::runtime_error("failed to open file " + fileName);

        std::vector<char> buffer(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

        file.close();
        return buffer;
    }

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        auto context = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        context->m_framebufferResized = true;
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

    void CreateImageViews()
    {
        m_swapChainImageViews.clear();

        constexpr vk::ImageSubresourceRange subresourceRange
        {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        vk::ImageViewCreateInfo imageViewCreateInfo
        {
            .viewType = vk::ImageViewType::e2D,
            .format = m_swapChainImageFormat,
            .subresourceRange = subresourceRange
        };

        for (const auto& image : m_swapChainImages)
        {
            imageViewCreateInfo.image = image;
            m_swapChainImageViews.emplace_back(m_device, imageViewCreateInfo);
        }
    }

    void CreateGraphicsPipeline()
    {
        vk::raii::ShaderModule shaderModule = CreateShaderModule(ReadFile("../Resources/Shaders/bin/slang.spv"));

        vk::PipelineShaderStageCreateInfo vertShaderStageCreateInfo
        {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = shaderModule,
            .pName = "vertMain", // entrypoint name
        };

        vk::PipelineShaderStageCreateInfo fragShaderStageCreateInfo
        {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = shaderModule,
            .pName = "fragMain"
        };

        vk::PipelineShaderStageCreateInfo shaderStage[] = {vertShaderStageCreateInfo, fragShaderStageCreateInfo};

        vk::VertexInputBindingDescription vertexBindingDescription = Vertex::GetBindingDescription();
        std::array<vk::VertexInputAttributeDescription, 2> vertexAttributeDescriptions = Vertex::GetAttributeDescriptions();
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo
        {
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertexBindingDescription,
            .vertexAttributeDescriptionCount = vertexAttributeDescriptions.size(),
            .pVertexAttributeDescriptions = vertexAttributeDescriptions.data(),
        };
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo
        {
            .topology = vk::PrimitiveTopology::eTriangleList,
        };

        std::vector<vk::DynamicState> dynamicStates
        {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };

        vk::PipelineDynamicStateCreateInfo dynamicState
        {
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        };

        // since viewport/scissor are dynamic, we do not need to pass pointers to this struct
        vk::PipelineViewportStateCreateInfo viewportState
        {
            .viewportCount = 1,
            .scissorCount = 1,
        };

        vk::PipelineRasterizationStateCreateInfo rasterizationStateInfo
        {
            .depthClampEnable = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eClockwise,
            .depthBiasEnable = vk::False,
            .depthBiasSlopeFactor = 1.f,
            .lineWidth = 1.f
        };

        vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo
        {
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable = vk::False,
        };

        vk::PipelineColorBlendAttachmentState colorBlendAttachment
        {
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        };

        vk::PipelineColorBlendStateCreateInfo colorBlending
        {
            .logicOpEnable = vk::False,
            .logicOp = vk::LogicOp::eCopy,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment
        };

        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo
        {
            .setLayoutCount = 0,
            .pushConstantRangeCount = 0
        };
        m_pipelineLayout = vk::raii::PipelineLayout(m_device, pipelineLayoutCreateInfo);

        vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo
        {
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &m_swapChainImageFormat,
        };

        vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo
        {
            .pNext = &pipelineRenderingCreateInfo,
            .stageCount = 2,
            .pStages = shaderStage,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyStateCreateInfo,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizationStateInfo,
            .pMultisampleState = &multisampleStateCreateInfo,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = m_pipelineLayout,
            .renderPass = nullptr, // no render passes, we're using Dynamic rendering from Vk 1.3

            // infos for pipeline derivation. This is useless as long as we do not set the flag below
            //.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
        };

        m_pipeline = vk::raii::Pipeline(m_device, nullptr, graphicsPipelineCreateInfo);
    }

    [[nodiscard]] vk::raii::ShaderModule CreateShaderModule(const std::vector<char>& shaderSource) const
    {
        vk::ShaderModuleCreateInfo shaderModuleCreateInfo
        {
            .codeSize = shaderSource.size() * sizeof(char),
            .pCode = reinterpret_cast<const uint32_t*>(shaderSource.data())
        };

        return {m_device, shaderModuleCreateInfo};
    }

    void CreateCommandPool()
    {
        vk::CommandPoolCreateInfo commandPoolCreateInfo
        {
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = static_cast<uint32_t>(m_graphicsQueueIndex),
        };

        vk::CommandPoolCreateInfo transferCommandPoolCreateInfo
        {
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = static_cast<uint32_t>(m_transferQueueIndex),
        };

        m_commandPool = vk::raii::CommandPool(m_device, commandPoolCreateInfo);
        m_transferCommandPool = vk::raii::CommandPool(m_device, transferCommandPoolCreateInfo);
    }

    void CreateCommandBuffers()
    {
        vk::CommandBufferAllocateInfo commandBufferAllocateInfo
        {
            .commandPool = m_commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
        };

        m_commandBuffers = vk::raii::CommandBuffers(m_device, commandBufferAllocateInfo);
    }

    void RecordCommandBuffer(uint32_t swapChainImageIndex)
    {
        m_commandBuffers[m_currentFrame].begin({}); // we can pass flags here, see vk::CommandBufferUsageFlagBits

        TransitionImageLayout(
            swapChainImageIndex,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput
        );

        vk::ClearValue clearColor = vk::ClearColorValue(.0f, .0f, .0f, 1.f);
        vk::RenderingAttachmentInfo attachmentInfos
        {
            .imageView = m_swapChainImageViews[swapChainImageIndex],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        };

        vk::RenderingInfo renderingInfo
        {
            .renderArea = { .offset = {0, 0}, .extent = m_swapChainExtent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentInfos
        };

        m_commandBuffers[m_currentFrame].beginRendering(renderingInfo);

        m_commandBuffers[m_currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
        m_commandBuffers[m_currentFrame].setViewport(0, vk::Viewport(.0f, .0f, static_cast<float>(m_swapChainExtent.width), static_cast<float>(m_swapChainExtent.height), .0f, 1.f));
        m_commandBuffers[m_currentFrame].setScissor(0, vk::Rect2D({0, 0}, m_swapChainExtent));

        m_commandBuffers[m_currentFrame].bindVertexBuffers(0, *m_triangleBuffer.Handle(), {0});

        m_commandBuffers[m_currentFrame].draw(3, 1, 0, 0);
        m_commandBuffers[m_currentFrame].endRendering();

        // After rendering, transition the swapchain image to PRESENT_SRC
        TransitionImageLayout(
            swapChainImageIndex,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,                 // srcAccessMask
            {},                                                      // dstAccessMask
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
            vk::PipelineStageFlagBits2::eBottomOfPipe                  // dstStage
        );
        m_commandBuffers[m_currentFrame].end();
    }

    void TransitionImageLayout(
        uint32_t imageIndex,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask
    )
    {
        vk::ImageMemoryBarrier2 barrier = {
            .srcStageMask = srcStageMask,
            .srcAccessMask = srcAccessMask,
            .dstStageMask = dstStageMask,
            .dstAccessMask = dstAccessMask,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_swapChainImages[imageIndex],
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        vk::DependencyInfo dependencyInfo = {
            .dependencyFlags = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
        };
        m_commandBuffers[m_currentFrame].pipelineBarrier2(dependencyInfo);
    }

    void CreateSyncObjects()
    {
        m_presentCompleteSemaphores.clear();
        m_renderFinishedSemaphores.clear();
        m_frameFences.clear();

        for (int i = 0; i < m_swapChainImages.size(); i++)
        {
            m_presentCompleteSemaphores.emplace_back(m_device, vk::SemaphoreCreateInfo());
            m_renderFinishedSemaphores.emplace_back(m_device, vk::SemaphoreCreateInfo());
        }

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_frameFences.emplace_back(m_device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
        }
    }

    void DrawFrame()
    {
        // wait for the frame to be finished before doing anything else, we don't want to begin asking for more swapchain
        // images while rendering is still ongoing
        while (m_device.waitForFences(*m_frameFences[m_currentFrame], vk::True, std::numeric_limits<uint64_t>::max()) == vk::Result::eTimeout)
        {
        }

        auto [result, imageIndex] = SwapchainNextImageWrapper(
            m_swapChain,
            std::numeric_limits<uint64_t>::max(),
            *m_presentCompleteSemaphores[m_semaphoreIndex],
            nullptr
        );

        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            RecreateSwapChain();
            return;
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
            throw std::runtime_error("couldn't acquire swap chain image!");

        m_device.resetFences(*m_frameFences[m_currentFrame]);
        m_commandBuffers[m_currentFrame].reset();
        RecordCommandBuffer(imageIndex);

        vk::PipelineStageFlags waitDestinationStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        const vk::SubmitInfo submitInfo
        {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*m_presentCompleteSemaphores[m_semaphoreIndex],
            .pWaitDstStageMask = &waitDestinationStageFlags,
            .commandBufferCount = 1,
            .pCommandBuffers = &*m_commandBuffers[m_currentFrame],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*m_renderFinishedSemaphores[imageIndex]
        };

        m_graphicsQueue.submit(submitInfo, *m_frameFences[m_currentFrame]);

        const vk::PresentInfoKHR presentInfo
        {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*m_renderFinishedSemaphores[imageIndex],
            .swapchainCount = 1,
            .pSwapchains = &*m_swapChain,
            .pImageIndices = &imageIndex
        };

        result = QueuePresentWrapper(m_graphicsQueue, presentInfo);
        //result = m_graphicsQueue.presentKHR(presentInfo);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_framebufferResized)
        {
            RecreateSwapChain();
            m_framebufferResized = false;
        }
        else if (result != vk::Result::eSuccess)
            throw std::runtime_error("Couldn't present swap chain image");

        m_semaphoreIndex = (m_semaphoreIndex + 1) % m_swapChainImages.size();
        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void RecreateSwapChain()
    {
        // Handle window minimizing: the swapchain created will have a size of 0, so we wait until we receive a valid
        // size
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(m_window, &width, &height);
            glfwWaitEvents();
        }

        // we should try changing the swapchain creation to pass the old swapchain as input in the createinfo, then delete the old one.
        m_device.waitIdle();

        CleanupSwapChain();

        CreateSwapChain();
        CreateImageViews();
    }

    void CleanupSwapChain()
    {
        m_swapChainImageViews.clear();
        m_swapChain = nullptr;
    }

    /**
     * @brief vk::raii::SwapchainKHR::acquireNextImageKHR without exceptions
     */
    std::pair<vk::Result, uint32_t> SwapchainNextImageWrapper(const vk::raii::SwapchainKHR &swapchain,
                                                              uint64_t timeout, vk::Semaphore semaphore,
                                                              vk::Fence fence) {
        uint32_t image_index;
        vk::Result result = static_cast<vk::Result>(swapchain.getDispatcher()->vkAcquireNextImageKHR(
            static_cast<VkDevice>(swapchain.getDevice()), static_cast<VkSwapchainKHR>(*swapchain),
            timeout, static_cast<VkSemaphore>(semaphore), static_cast<VkFence>(fence), &image_index));
        return std::make_pair(result, image_index);
    }

    /**
     * @brief vk::raii::Queue::presentKHR without exceptions
     */
    vk::Result QueuePresentWrapper(const vk::raii::Queue &queue,
                                   const vk::PresentInfoKHR &present_info) {
        return static_cast<vk::Result>(queue.getDispatcher()->vkQueuePresentKHR(
            static_cast<VkQueue>(*queue), reinterpret_cast<const VkPresentInfoKHR *>(&present_info)));
    }

    void CreateVertexBuffer()
    {
        m_triangleBuffer = Buffer(&m_device, m_physicalDevice, sizeof(vertices[0]) * vertices.size(), {m_graphicsQueueIndex, m_transferQueueIndex});
        m_triangleBuffer.Map(vertices, m_transferCommandPool, m_transferQueue);
    }

    int FindTransferOnlyQueue(const std::vector<vk::QueueFamilyProperties>& queueFamilies)
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
    #pragma endregion Methods

    #pragma region Lifecycle
    void InitWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_window = glfwCreateWindow(WIDTH, HEIGHT, "Cicada", nullptr, nullptr);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, FramebufferSizeCallback);
    }

    void InitVulkan()
    {
        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapChain();
        CreateImageViews();
        CreateGraphicsPipeline();
        CreateCommandPool();
        CreateVertexBuffer();
        CreateCommandBuffers();
        CreateSyncObjects();
    }

    void MainLoop()
    {
        while(!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
            DrawFrame();
        }

        // wait for any async operation to finish
        m_device.waitIdle();
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