#include "renderer.hpp"

#include <array>
#include <stdexcept>

namespace gfx::vk {

Renderer::Renderer(Device& device, win::Window& window) : m_device(device), m_window(window) {
    recreateSwapchain();
    createCommandBuffers();
}

Renderer::~Renderer() {
    vkFreeCommandBuffers(
        m_device.getDevice(), m_device.getCommandPool(),
        static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
}

void Renderer::createCommandBuffers() {
    m_commandBuffers.resize(base::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_device.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    if (vkAllocateCommandBuffers(m_device.getDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to allocate command buffers");
    }
}

void Renderer::recreateSwapchain() {
    while (m_window.isResized()) {
        glfwWaitEvents();
    }

    m_device.waitIdle();

    VkExtent2D extent{};
    extent.width = static_cast<uint32_t>(m_window.getWidth());
    extent.height = static_cast<uint32_t>(m_window.getHeight());

    if (m_swapchain == nullptr) {
        m_swapchain = std::make_unique<Swapchain>(m_device, extent);
    } else {
        std::shared_ptr<Swapchain> oldSwapchain = std::move(m_swapchain);
        m_swapchain = std::make_unique<Swapchain>(m_device, extent, oldSwapchain);

        if (!oldSwapchain->compareSwapFormats(*m_swapchain.get())) {
            throw std::runtime_error("Vulkan: Swapchain image (or depth) format has changed");
        }
    }
}

VkCommandBuffer Renderer::beginFrame() {
    uint32_t imageIndex;
    VkResult result = m_swapchain->acquireNextImage(m_currentFrameIndex, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return nullptr;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Vulkan: Failed to acquire swapchain image");
    }

    m_currentImageIndex = imageIndex;

    m_isFrameStarted = true;

    VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to begin recording command buffer");
    }

    return commandBuffer;
}

void Renderer::endFrame() {
    VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to record command buffer");
    }

    VkResult result = m_swapchain->submitCommandBuffers(&commandBuffer, m_currentFrameIndex, m_currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.isResized()) {
        m_window.resetResizedState();
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to present swapchain image");
    }

    m_isFrameStarted = false;

    m_currentFrameIndex = (m_currentFrameIndex + 1) % base::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::beginRenderPass(VkCommandBuffer commandBuffer) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = getRenderPass();
    renderPassInfo.framebuffer = m_swapchain->getFrameBuffer(m_currentImageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchain->getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchain->getExtent().width);
    viewport.height = static_cast<float>(m_swapchain->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = m_swapchain->getExtent();
    scissor.offset = {0, 0};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::endRenderPass(VkCommandBuffer commandBuffer) { vkCmdEndRenderPass(commandBuffer); }

} // namespace gfx::vk