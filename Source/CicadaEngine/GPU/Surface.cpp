//
// Created by Bellaedris on 03/12/2025.
//

#include "Surface.h"

namespace cica::gpu {
    Surface::Surface(Device *device, Window *window)
    : m_parentDevice(device)
    {
        // surface
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(*device->Instance(), window->GetWindow(), nullptr, &surface) != 0)
        {
            throw std::runtime_error("Couldn't create window surface.");
        }
        m_surface = vk::raii::SurfaceKHR(device->Instance(), surface);

        // swapchain
        CreateSwapchain(window->Width(), window->Height());
    }

    vk::SurfaceFormatKHR Surface::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
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

    vk::PresentModeKHR Surface::ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes)
    {
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox)
                return availablePresentMode;
        }
        return vk::PresentModeKHR::eFifo;
    }

    void Surface::RecreateSwapchain(uint32_t newWidth, uint32_t newHeight)
    {
        m_swapchain = nullptr;
        CreateSwapchain(newWidth, newHeight);
    }

    void Surface::CreateSwapchain(uint32_t width, uint32_t height)
    {
        vk::SurfaceCapabilitiesKHR surfaceCapabilities = m_parentDevice->PhysicalDevice().getSurfaceCapabilitiesKHR(m_surface);
        vk::SurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(m_parentDevice->PhysicalDevice().getSurfaceFormatsKHR(m_surface));
        vk::Extent2D extent = {width, height};
        vk::PresentModeKHR presentMode = ChooseSwapPresentMode(m_parentDevice->PhysicalDevice().getSurfacePresentModesKHR(m_surface));
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
                        .presentMode = presentMode,
                        .clipped = true,
                        .oldSwapchain = nullptr // pass the old swapchain in case of recreation, for instance when changing resolution
                };

        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;

        m_swapchain = vk::raii::SwapchainKHR(m_parentDevice->Get(), swapchainCreateInfo);
        m_swapChainImages = m_swapchain.getImages();

        // image views
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
            m_swapChainImageViews.emplace_back(m_parentDevice->Get(), imageViewCreateInfo);
        }
    }
} // cica::gpu