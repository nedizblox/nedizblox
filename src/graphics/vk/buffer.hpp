#pragma once

#include "device.hpp"

#include <cstring>
#include <stdexcept>

namespace gfx::vk {

class Buffer {
public:
    Buffer(
        Device& device, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
        VmaAllocationCreateFlags allocFlags = 0);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    VkBuffer getBuffer() const { return m_buffer; }
    VkDeviceSize getSize() const { return m_size; }

    void* map();
    void unmap();

    template <typename T>
    void uploadData(const T* data, VkDeviceSize size) {
        if (size > m_size) {
            throw std::runtime_error("Vulkan: Buffer size is too small");
        }

        void* mappedData = map();
        std::memcpy(mappedData, data, static_cast<size_t>(size));
        unmap();
    }

private:
    Device& m_device;

    VkBuffer m_buffer;
    VmaAllocation m_allocation;
    VkDeviceSize m_size;

    void* m_data;
    bool m_isMapped = false;
};

} // namespace gfx::vk