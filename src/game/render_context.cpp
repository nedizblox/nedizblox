#include "render_context.hpp"

namespace game {

RenderContext::RenderContext(int width, int height, const std::string& title, const std::string& iconPath) {
    initWindow(width, height, title, iconPath);
    initVulkan();
    initDescriptors();

    m_bindlessManager = std::make_unique<gfx::mngrs::BindlessManager>(*m_device, *m_setLayout, *m_pool);
}

RenderContext::~RenderContext() { waitIdle(); }

void RenderContext::initWindow(int width, int height, const std::string& title, const std::string& iconPath) {
    m_window = std::make_unique<win::Window>(width, height, title);
    if (!iconPath.empty()) {
        m_window->setIcon(iconPath);
    }
}

void RenderContext::initVulkan() {
    m_device = std::make_unique<gfx::vk::Device>(*m_window);
    m_renderer = std::make_unique<gfx::vk::Renderer>(*m_device, *m_window);
}

void RenderContext::initDescriptors() {
    m_pool = gfx::vk::DescriptorPool::Builder(*m_device)
                 .setMaxSets(100)
                 .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
                 .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000)
                 .build();

    m_setLayout
        = gfx::vk::DescriptorSetLayout::Builder(*m_device)
              .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1000)
              .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 10)
              .build();
}

void RenderContext::waitIdle() {
    if (m_device) {
        m_device->waitIdle();
    }
}

} // namespace game