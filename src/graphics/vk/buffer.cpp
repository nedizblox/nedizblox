#include "buffer.hpp"

namespace gfx::vk {

Buffer::Buffer(Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocFlags) :
    m_device(device), m_size(size) {
    m_device.createBuffer(m_size, usage, memoryUsage, m_buffer, m_allocation, allocFlags);
}

Buffer::~Buffer() {
    unmap();
    vmaDestroyBuffer(m_device.getAllocator(), m_buffer, m_allocation);
}

void* Buffer::map() {
    if (m_isMapped) {
        return m_data;
    }

    if (vmaMapMemory(m_device.getAllocator(), m_allocation, &m_data) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to map memory");
    }

    m_isMapped = true;
    return m_data;
}

void Buffer::unmap() {
    if (!m_isMapped) {
        return;
    }

    vmaUnmapMemory(m_device.getAllocator(), m_allocation);
    m_isMapped = false;
}

} // namespace gfx::vk