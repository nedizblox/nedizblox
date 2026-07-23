#pragma once

#include "base.hpp"
#include "window/window.hpp"

#include <vma/vk_mem_alloc.h>

#include <vector>

namespace gfx::vk {

class Device {
public:
    Device(win::Window& window);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

    VkDevice getDevice() const { return m_device; }
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    VkPhysicalDeviceProperties getDeviceProperties() const { return m_deviceProperties; }
    VkInstance getInstance() const { return m_instance; }
    VmaAllocator getAllocator() const { return m_allocator; }
    VkSurfaceKHR getSurface() const { return m_surface; }
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getPresentQueue() const { return m_presentQueue; }
    VkCommandPool getCommandPool() const { return m_commandPool; }

    base::SwapchainSupportDetails getSwapchainSupport() const {
        return querySwapchainSupport(m_physicalDevice);
    }
    base::QueueFamilyIndices findPhysicalQueueFamilies() const {
        return findQueueFamilies(m_physicalDevice);
    }
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

    void waitIdle() const;

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void createBuffer(
        VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer,
        VmaAllocation& allocation, VmaAllocationCreateFlags allocFlags = 0);
    void createImage(
        const VkImageCreateInfo& createInfo, VmaMemoryUsage memoryUsage, VkImage& image,
        VmaAllocation& allocation, VmaAllocationCreateFlags allocFlags = 0);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount = 1);

    void generateMipmaps(VkImage image, int32_t width, int32_t height, uint32_t mipLevels, uint32_t layerCount = 1);

    void transitionImageLayout(
        VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
        uint32_t mipLevels = 1, uint32_t layerCount = 1);

private:
    win::Window& m_window;

    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device;

    VkPhysicalDeviceProperties m_deviceProperties;

    VmaAllocator m_allocator;

    VkSurfaceKHR m_surface;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;

    VkCommandPool m_commandPool;

    bool isDeviceSuitable(VkPhysicalDevice physicalDevice) const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
    std::vector<const char*> getRequiredExtensions() const;
    bool checkValidationLayerSupport() const;
    base::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice) const;
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const;
    base::SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice physicalDevice) const;

    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createAllocator();
    void createCommandPool();
};

} // namespace gfx::vk
