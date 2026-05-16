#pragma once

#include "device.hpp"
#include "swapchain.hpp"
#include "window/window.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace gfx::vk {

class Renderer {
public:
    Renderer(Device& device, win::Window& window);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    VkRenderPass getRenderPass() const { return m_swapchain->getRenderPass(); }

    VkCommandBuffer getCurrentCommandBuffer() const {
        assert(m_isFrameStarted && "Cannot get command buffer when frame not in progress");
        return m_commandBuffers[m_currentFrameIndex];
    }

    uint32_t getFrameIndex() const {
        assert(m_isFrameStarted && "Cannot get frame index when frame not in progress");
        return m_currentFrameIndex;
    }

    VkCommandBuffer beginFrame();
    void endFrame();

    void beginRenderPass(VkCommandBuffer commandBuffer);
    void endRenderPass(VkCommandBuffer commandBuffer);

private:
    win::Window& m_window;
    Device& m_device;

    std::unique_ptr<Swapchain> m_swapchain;
    uint32_t m_currentFrameIndex = 0;
    uint32_t m_currentImageIndex = 0;
    bool m_isFrameStarted = false;

    std::vector<VkCommandBuffer> m_commandBuffers;

    void createCommandBuffers();
    void recreateSwapchain();
};

} // namespace gfx::vk