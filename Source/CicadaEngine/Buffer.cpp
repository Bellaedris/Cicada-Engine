//
// Created by bellaedris on 17/10/25.
//

#include "Buffer.h"

Buffer::Buffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, const std::vector<Vertex>& vertices)
{
    vk::BufferCreateInfo bufferInfo
    {
        .flags = {},
        .size = sizeof(vertices[0]) * vertices.size(),
        .usage = vk::BufferUsageFlagBits::eVertexBuffer,
        .sharingMode = vk::SharingMode::eExclusive
    };

    m_buffer = vk::raii::Buffer(device, bufferInfo);
    vk::MemoryRequirements memoryRequirements = m_buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo memoryAllocateInfo
    {
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = FindBufferMemoryType(physicalDevice, memoryRequirements)
    };

    m_bufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);
    m_buffer.bindMemory(*m_bufferMemory, 0);

    void* data = m_bufferMemory.mapMemory(0, bufferInfo.size);
    memcpy(data, vertices.data(), bufferInfo.size);
    m_bufferMemory.unmapMemory();
}

uint32_t Buffer::FindBufferMemoryType(const vk::raii::PhysicalDevice &physicalDevice, const vk::MemoryRequirements& memoryRequirements)
{
    vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((memoryRequirements.memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("couldn't find suitable memory type!");
}
