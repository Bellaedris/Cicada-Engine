//
// Created by bellaedris on 17/10/25.
//

#pragma once
#include <array>
#include <cstdint>

#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>

struct Vertex
{
    glm::vec2 position;
    glm::vec3 color;

    static vk::VertexInputBindingDescription GetBindingDescription()
    {
        return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 2> GetAttributeDescriptions()
    {
        return
        {
            vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, position)},
            vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)},
        };
    }
};

class Buffer {
private:
    vk::raii::Buffer m_buffer {nullptr};
    vk::raii::DeviceMemory m_bufferMemory {nullptr};
public:
    Buffer() = default;
    Buffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, const std::vector<Vertex>& vertices);

    static uint32_t FindBufferMemoryType(const vk::raii::PhysicalDevice& physicalDevice, const vk::MemoryRequirements& memoryRequirements);

    vk::raii::Buffer& Handle() { return m_buffer; };
};