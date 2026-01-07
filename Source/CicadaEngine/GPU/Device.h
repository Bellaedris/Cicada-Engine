//
// Created by Bellaedris on 12/11/2025.
//

#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_raii.hpp>

#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Buffer.h"
#include "../Window.h"
#include "Surface.h"

namespace cica::gpu
{
    class Device
    {
    private:
    #pragma region constants
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
    #pragma endregion constants

    #pragma region members
    GLFWwindow* m_window;

    vk::raii::Context m_context;
    vk::raii::Instance m_instance {nullptr};
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger {nullptr};

    // devices
    vk::raii::PhysicalDevice m_physicalDevice {nullptr};
    vk::raii::Device m_device {nullptr};

    // queues
    vk::raii::Queue m_graphicsQueue {nullptr};
    vk::raii::Queue m_presentQueue {nullptr};
    vk::raii::Queue m_transferQueue {nullptr};
    uint32_t m_graphicsQueueIndex {-1u};
    uint32_t m_transferQueueIndex {-1u};

    bool m_enableValidationLayers;

    uint32_t m_width = 800;
    uint32_t m_height = 600;
    #pragma endregion members

    #pragma region Methods
    void CreateInstance(bool useValidationLayers, const char* appName);
    void CreateDebugMessenger();

    void PickPhysicalDevice();
    void CreateLogicalDevice();

    std::vector<const char*> GetRequiredExtensions();
    int FindTransferOnlyQueue(const std::vector<vk::QueueFamilyProperties>& queueFamilies);
    #pragma endregion Methods

    public:
        Device(bool useValidationLayers, const char* appName);

        // ressources creation
        std::unique_ptr<Surface> CreateSurface(cica::Window* window);
        std::unique_ptr<Buffer> CreateBuffer(uint64_t size, Buffer::BufferUsage usage);

        #pragma region Accesors
        [[nodiscard]] GLFWwindow* Window() { return m_window; };
        [[nodiscard]] const vk::raii::Instance& Instance() const { return m_instance; };
        [[nodiscard]] const vk::raii::PhysicalDevice& PhysicalDevice() const { return m_physicalDevice; };
        [[nodiscard]] const vk::raii::Device& Get() const { return m_device; };
        [[nodiscard]] const vk::raii::Queue& GraphicsQueue() const { return m_graphicsQueue; };
        [[nodiscard]] const vk::raii::Queue& TransferQueue() const { return m_transferQueue; };

        [[nodiscard]] const uint32_t GraphicsQueueIndex() const { return m_graphicsQueueIndex; };
        [[nodiscard]] const uint32_t TransferQueueIndex() const { return m_transferQueueIndex; };
        #pragma endregion Accesors
    };
} // cica::gpu