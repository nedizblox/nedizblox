#pragma once

#include "device.hpp"

#include <memory>
#include <vector>

namespace gfx::vk {

class Swapchain {
public:
    Swapchain(Device& device, VkExtent2D windowExtent);
    Swapchain(Device& device, VkExtent2D windowExtent, std::shared_ptr<Swapchain> previous);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    VkFramebuffer getFrameBuffer(int index) const { return m_swapchainFramebuffers[index]; }
    VkRenderPass getRenderPass() const { return m_renderPass; }
    VkImageView getImageView(int index) const { return m_swapchainImageViews[index]; }
    VkExtent2D getExtent() const { return m_swapchainExtent; }
    size_t getImageCount() const { return m_swapchainImages.size(); }
    VkFormat getSwapchainImageFormat() const { return m_swapchainImageFormat; }

    VkFormat findDepthFormat() const {
        return m_device.findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    bool compareSwapFormats(const Swapchain& swapchain) const {
        return swapchain.m_swapchainDepthFormat == m_swapchainDepthFormat
               && swapchain.m_swapchainImageFormat == m_swapchainImageFormat;
    }

    VkResult acquireNextImage(uint32_t currentFrameIndex, uint32_t* imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t currentFrame, uint32_t imageIndex);

private:
    Device& m_device;
    VkExtent2D m_windowExtent;

    VkFormat m_swapchainImageFormat;
    VkFormat m_swapchainDepthFormat;
    VkExtent2D m_swapchainExtent;

    std::vector<VkFramebuffer> m_swapchainFramebuffers;
    VkRenderPass m_renderPass;

    std::vector<VkImage> m_depthImages;
    std::vector<VmaAllocation> m_depthImageAllocations;
    std::vector<VkImageView> m_depthImageViews;

    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;

    VkSwapchainKHR m_swapchain;
    std::shared_ptr<Swapchain> m_oldSwapchain;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    std::vector<VkFence> m_imagesInFlight;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    void init();
    void createSwapchain();
    void createImageViews();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();
};

} // namespace gfx::vk
