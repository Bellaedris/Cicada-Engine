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

namespace cica::gpu
{
    class Device;

    //TODO template this so we can init a buffer with just a size_t param, then map with an actual buffer that we can type check
    class Buffer
    {
    #pragma region Enums
    public:
        enum BufferUsage
        {
            cVertex = 1 << 0,
            cIndex = 1 << 1,
        };
    #pragma endregion Enums

    private:
        Device *m_parentDevice;

        vk::raii::Buffer m_stagingBuffer{nullptr};
        vk::raii::Buffer m_buffer{nullptr};
        vk::raii::DeviceMemory m_stagingBufferMemory{nullptr};
        vk::raii::DeviceMemory m_bufferMemory{nullptr};
        vk::DeviceSize m_bufferSize{};

        static vk::BufferUsageFlagBits GetBufferUsage(BufferUsage usage);
    public:
        Buffer() = default;

        Buffer(Device* device,
               const std::vector<uint32_t> &sharedQueueFamilies, vk::DeviceSize size, BufferUsage usage);

        void Map(const std::vector<Vertex> &vertices, const vk::raii::CommandPool &commandPool,
                 const vk::raii::Queue &queue);

        vk::raii::Buffer &Handle() { return m_buffer; };

        static uint32_t FindBufferMemoryType(const vk::raii::PhysicalDevice &physicalDevice,
                                             const vk::MemoryRequirements &memoryRequirements, vk::
                                             MemoryPropertyFlags properties);

        static void CopyBuffer(
                const vk::raii::Buffer &src,
                const vk::raii::Buffer &dst,
                vk::DeviceSize size,
                const vk::raii::CommandPool &commandPool,
                const vk::raii::Device *device,
                vk::raii::Queue queue
        );
    };
}