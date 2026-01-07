//
// Created by bellaedris on 17/10/25.
//

#include "Buffer.h"

#include "Device.h"

namespace cica::gpu
{
    Buffer::Buffer(Device* device,
                   const std::vector<uint32_t> &sharedQueueFamilies, vk::DeviceSize size, BufferUsage usage)
            : m_parentDevice(device)
    {
        m_bufferSize = size;

        // staging buffer
        vk::BufferCreateInfo stagingInfo
        {
            .flags = {},
            .size = m_bufferSize,
            .usage = vk::BufferUsageFlagBits::eTransferSrc,
            .sharingMode = vk::SharingMode::eConcurrent,
            .queueFamilyIndexCount = static_cast<uint32_t>(sharedQueueFamilies.size()),
            .pQueueFamilyIndices = sharedQueueFamilies.data(),
        };

        m_stagingBuffer = vk::raii::Buffer(m_parentDevice->Get(), stagingInfo);
        vk::MemoryRequirements memoryRequirementsStaging = m_stagingBuffer.getMemoryRequirements();

        vk::MemoryAllocateInfo memoryAllocateInfoStaging
        {
            .allocationSize = memoryRequirementsStaging.size,
            .memoryTypeIndex = FindBufferMemoryType(m_parentDevice->PhysicalDevice(), memoryRequirementsStaging,
                                                    vk::MemoryPropertyFlagBits::eHostCoherent |
                                                    vk::MemoryPropertyFlagBits::eHostVisible)
        };

        m_stagingBufferMemory = vk::raii::DeviceMemory(m_parentDevice->Get(), memoryAllocateInfoStaging);
        m_stagingBuffer.bindMemory(*m_stagingBufferMemory, 0);

        // actual buffer
        vk::BufferCreateInfo bufferInfo
        {
            .flags = {},
            .size = m_bufferSize,
            .usage = GetBufferUsage(usage) | vk::BufferUsageFlagBits::eTransferDst,
            .sharingMode = vk::SharingMode::eConcurrent,
            .queueFamilyIndexCount = static_cast<uint32_t>(sharedQueueFamilies.size()),
            .pQueueFamilyIndices = sharedQueueFamilies.data(),
        };

        m_buffer = vk::raii::Buffer(m_parentDevice->Get(), bufferInfo);
        vk::MemoryRequirements memoryRequirements = m_buffer.getMemoryRequirements();

        vk::MemoryAllocateInfo memoryAllocateInfo
                {
                        .allocationSize = memoryRequirements.size,
                        .memoryTypeIndex = FindBufferMemoryType(m_parentDevice->PhysicalDevice(), memoryRequirements,
                                                                vk::MemoryPropertyFlagBits::eDeviceLocal)
                };

        m_bufferMemory = vk::raii::DeviceMemory(m_parentDevice->Get(), memoryAllocateInfo);
        m_buffer.bindMemory(*m_bufferMemory, 0);
    }

    void Buffer::Map(const std::vector<Vertex> &vertices, const vk::raii::CommandPool &commandPool,
                     const vk::raii::Queue &queue) {
        void *dataStaging = m_stagingBufferMemory.mapMemory(0, m_bufferSize);
        memcpy(dataStaging, vertices.data(), m_bufferSize);
        m_stagingBufferMemory.unmapMemory();

        CopyBuffer(m_stagingBuffer, m_buffer, m_bufferSize, commandPool, &m_parentDevice->Get(), queue);
    }

    uint32_t Buffer::FindBufferMemoryType(
            const vk::raii::PhysicalDevice &physicalDevice,
            const vk::MemoryRequirements &memoryRequirements,
            vk::MemoryPropertyFlags properties) {
        vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if ((memoryRequirements.memoryTypeBits & (1 << i)) &&
                (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        throw std::runtime_error("couldn't find suitable memory to allocate buffer");
    }

    void Buffer::CopyBuffer(
            const vk::raii::Buffer &src,
            const vk::raii::Buffer &dst,
            vk::DeviceSize size,
            const vk::raii::CommandPool &commandPool,
            const vk::raii::Device *device,
            vk::raii::Queue queue
    ) {
        vk::CommandBufferAllocateInfo allocateInfo
                {
                        .commandPool = commandPool,
                        .level = vk::CommandBufferLevel::ePrimary,
                        .commandBufferCount = 1,
                };

        vk::raii::CommandBuffer commandBuffer = std::move(device->allocateCommandBuffers(allocateInfo).front());

        commandBuffer.begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        commandBuffer.copyBuffer(src, dst, vk::BufferCopy(0, 0, size));
        commandBuffer.end();

        // TODO this is horrible, the buffer shouldn't be allowed to submit a queue. Find out who should be aware of the queue
        queue.submit(vk::SubmitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer}, nullptr);
        queue.waitIdle();
    }

    vk::BufferUsageFlagBits Buffer::GetBufferUsage(Buffer::BufferUsage usage)
    {
        switch (usage)
        {
            case cVertex:
                return vk::BufferUsageFlagBits::eVertexBuffer;
            case cIndex:
                return vk::BufferUsageFlagBits::eIndexBuffer;
            default:
                return vk::BufferUsageFlagBits::eVertexBuffer;
        }
    }
}
