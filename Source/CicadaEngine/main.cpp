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

#include "GPU/Buffer.h"
#include "GPU/Renderer.h"
#include "GPU/Surface.h"

#pragma region Constants
constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers
{
    "VK_LAYER_KHRONOS_validation",
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
        InitVulkan();
        MainLoop();
        Cleanup();
    }

private:
    #pragma region Members
    std::unique_ptr<cica::gpu::Renderer> m_renderer {nullptr};
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
    std::unique_ptr<cica::gpu::Buffer> m_triangleBuffer;
    #pragma endregion Members

    #pragma region Methods
    #pragma region Statics
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
        m_pipelineLayout = vk::raii::PipelineLayout(m_renderer->Device(), pipelineLayoutCreateInfo);

        vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo
        {
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &m_renderer->SwapchainImageFormat(),
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

        m_pipeline = vk::raii::Pipeline(m_renderer->Device(), nullptr, graphicsPipelineCreateInfo);
    }

    [[nodiscard]] vk::raii::ShaderModule CreateShaderModule(const std::vector<char>& shaderSource) const
    {
        vk::ShaderModuleCreateInfo shaderModuleCreateInfo
        {
            .codeSize = shaderSource.size() * sizeof(char),
            .pCode = reinterpret_cast<const uint32_t*>(shaderSource.data())
        };
        return {m_renderer->Device(), shaderModuleCreateInfo};
    }

    void CreateCommandPool()
    {
        vk::CommandPoolCreateInfo commandPoolCreateInfo
        {
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = static_cast<uint32_t>(m_renderer->GraphicsQueueIndex()),
        };

        vk::CommandPoolCreateInfo transferCommandPoolCreateInfo
        {
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = static_cast<uint32_t>(m_renderer->TransferQueueIndex()),
        };

        m_commandPool = vk::raii::CommandPool(m_renderer->Device(), commandPoolCreateInfo);
        m_transferCommandPool = vk::raii::CommandPool(m_renderer->Device(), transferCommandPoolCreateInfo);
    }

    void CreateCommandBuffers()
    {
        vk::CommandBufferAllocateInfo commandBufferAllocateInfo
        {
            .commandPool = m_commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
        };

        m_commandBuffers = vk::raii::CommandBuffers(m_renderer->Device(), commandBufferAllocateInfo);
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
            .imageView = m_renderer->SwapchainImageViews()[swapChainImageIndex],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        };

        vk::RenderingInfo renderingInfo
        {
            .renderArea = { .offset = {0, 0}, .extent = m_renderer->SwapchainExtent()},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentInfos
        };

        m_commandBuffers[m_currentFrame].beginRendering(renderingInfo);

        m_commandBuffers[m_currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
        m_commandBuffers[m_currentFrame].setViewport(0, vk::Viewport(.0f, .0f, static_cast<float>(m_renderer->SwapchainExtent().width), static_cast<float>(m_renderer->SwapchainExtent().height), .0f, 1.f));
        m_commandBuffers[m_currentFrame].setScissor(0, vk::Rect2D({0, 0}, m_renderer->SwapchainExtent()));

        m_commandBuffers[m_currentFrame].bindVertexBuffers(0, *m_triangleBuffer->Handle(), {0});

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
            .image = m_renderer->SwapchainImages()[imageIndex],
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

        for (int i = 0; i < m_renderer->SwapchainImages().size(); i++)
        {
            m_presentCompleteSemaphores.emplace_back(m_renderer->Device(), vk::SemaphoreCreateInfo());
            m_renderFinishedSemaphores.emplace_back(m_renderer->Device(), vk::SemaphoreCreateInfo());
        }

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_frameFences.emplace_back(m_renderer->Device(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
        }
    }

    void DrawFrame()
    {
        // wait for the frame to be finished before doing anything else, we don't want to begin asking for more swapchain
        // images while rendering is still ongoing
        while (m_renderer->Device().waitForFences(*m_frameFences[m_currentFrame], vk::True, std::numeric_limits<uint64_t>::max()) == vk::Result::eTimeout)
        {
        }

        auto [result, imageIndex] = SwapchainNextImageWrapper(
                m_renderer->Swapchain(),
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

        m_renderer->Device().resetFences(*m_frameFences[m_currentFrame]);
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

        m_renderer->GraphicsQueue().submit(submitInfo, *m_frameFences[m_currentFrame]);

        const vk::PresentInfoKHR presentInfo
        {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*m_renderFinishedSemaphores[imageIndex],
            .swapchainCount = 1,
            .pSwapchains = &*m_renderer->Swapchain(),
            .pImageIndices = &imageIndex
        };

        result = QueuePresentWrapper(m_renderer->GraphicsQueue(), presentInfo);
        //result = m_graphicsQueue.presentKHR(presentInfo);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_framebufferResized)
        {
            RecreateSwapChain();
            m_framebufferResized = false;
        }
        else if (result != vk::Result::eSuccess)
            throw std::runtime_error("Couldn't present swap chain image");

        m_semaphoreIndex = (m_semaphoreIndex + 1) % m_renderer->SwapchainImages().size();
        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void RecreateSwapChain()
    {
        // Handle window minimizing: the swapchain created will have a size of 0, so we wait until we receive a valid
        // size
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_renderer->Window(), &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(m_renderer->Window(), &width, &height);
            glfwWaitEvents();
        }

        // we should try changing the swapchain creation to pass the old swapchain as input in the createinfo, then delete the old one.
        m_renderer->Device().waitIdle();

        m_renderer->CleanupSwapChain();

        m_renderer->CreateSwapChain();
        m_renderer->CreateImageViews();
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
        m_triangleBuffer = m_renderer->CreateBuffer(sizeof(vertices[0]) * vertices.size(), cica::gpu::Buffer::BufferUsage::cVertex);
        m_triangleBuffer->Map(vertices, m_transferCommandPool, m_renderer->TransferQueue());
    }
    #pragma endregion Methods

    #pragma region Lifecycle

    void InitVulkan()
    {
        m_renderer = std::make_unique<cica::gpu::Renderer>(true, "HelloTriangle");
        CreateGraphicsPipeline();
        CreateCommandPool();
        CreateVertexBuffer();
        CreateCommandBuffers();
        CreateSyncObjects();
    }

    void MainLoop()
    {
        while(!glfwWindowShouldClose(m_renderer->Window()))
        {
            glfwPollEvents();
            DrawFrame();
        }

        // wait for any async operation to finish
        m_renderer->Device().waitIdle();
    }

    void Cleanup()
    {
        glfwDestroyWindow(m_renderer->Window());

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