//
// Created by Bellaedris on 03/12/2025.
//

#pragma once

#include "../Window.h"
#include "Device.h"

namespace cica::gpu
{
    class Device;

    /**
     * Manages a vkSurface and its associated swapchain. Not bundled in Device in case we need a renderer with multiple
     * render surfaces
     */
    class Surface
    {
    private:
        #pragma region Members
        //surface
        vk::raii::SurfaceKHR m_surface {nullptr};

        // swapchain
        vk::raii::SwapchainKHR m_swapchain {nullptr};
        std::vector<vk::Image> m_swapChainImages;
        vk::Format m_swapChainImageFormat{vk::Format::eUndefined};
        vk::Extent2D m_swapChainExtent;
        std::vector<vk::raii::ImageView> m_swapChainImageViews;

        const Device* m_parentDevice;
        #pragma endregion Members

        vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
        /**
         * Returns the most suitable present mode from a list of available ones in a surface attached to a device
         * @param availablePresentModes a list of all present modes compatible with this device/surface combination
         * @return Mailbox if found, fifo otherwise
         */
        vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    public:
        Surface(Device *device, Window *window);

        void CreateSwapchain(uint32_t width, uint32_t height);

        /**
         * \brief When the rendering surface changes in size, we have to recreate the swapchain. To do so, we nullify the
         * vk::raii::swapchain since we can't have 2 swapchains for a surface, same for images and all,
         * then we just recreate it with the new size
         * \param newWidth New width of the surface
         * \param newHeight New height of the surface
         */
        void RecreateSwapchain(uint32_t newWidth, uint32_t newHeight);

        [[nodiscard]] const vk::raii::SwapchainKHR& Swapchain() const { return m_swapchain; };
        [[nodiscard]] const vk::Format& SwapchainImageFormat() const { return m_swapChainImageFormat; };
        [[nodiscard]] const vk::Extent2D& SwapchainExtent() const { return m_swapChainExtent; };
        [[nodiscard]] const std::vector<vk::raii::ImageView>& SwapchainImageViews() const { return m_swapChainImageViews; };
        [[nodiscard]] const std::vector<vk::Image>& SwapchainImages() const { return m_swapChainImages; };
    };
} // gpu::cica