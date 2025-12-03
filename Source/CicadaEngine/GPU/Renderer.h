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

namespace cica::gpu
{
    class Renderer
    {
    private:
    #pragma region constants
    const std::vector<const char*> validationLayers
    {
        "VK_LAYER_KHRONOS_validation",
    };
    #pragma endregion constants

    #pragma region members

    vk::raii::Context m_context;
    vk::raii::Instance m_instance {nullptr};
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger {nullptr};
    vk::raii::SurfaceKHR m_surface {nullptr};

    bool m_enableValidationLayers;

    uint32_t m_width = 800;
    uint32_t m_height = 600;
    #pragma endregion members

    #pragma region Methods
    void CreateInstance(bool useValidationLayers, const char* appName);
    void CreateSurface();
    void CreateDebugMessenger();

    std::vector<const char*> GetRequiredExtensions();
    #pragma endregion Methods

    public:
        Renderer(bool useValidationLayers, const char* appName);

        #pragma region Accesors
        [[nodiscard]] inline const vk::raii::Instance& Instance() const { return m_instance; };
        [[nodiscard]] inline const vk::raii::SurfaceKHR& Surface() const { return m_surface; };
        #pragma endregion Accesors
    };
} // cica::gpu