#pragma once

#include "graphics/graphics.hpp"

#include <memory>
#include <string>

namespace game {

class RenderContext {
public:
    RenderContext(int width, int height, const std::string& title, const std::string& iconPath);
    ~RenderContext();

    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;

    win::Window& getWindow() const { return *m_window; }
    gfx::vk::Device& getDevice() const { return *m_device; }
    gfx::vk::Renderer& getRenderer() const { return *m_renderer; }

    gfx::vk::DescriptorPool& getDescriptorPool() const { return *m_pool; }
    gfx::vk::DescriptorSetLayout& getDescriptorSetLayout() const { return *m_setLayout; }
    gfx::mngrs::BindlessManager& getBindlessManager() const { return *m_bindlessManager; }

    void waitIdle();

private:
    std::unique_ptr<win::Window> m_window;
    std::unique_ptr<gfx::vk::Device> m_device;
    std::unique_ptr<gfx::vk::Renderer> m_renderer;

    std::unique_ptr<gfx::vk::DescriptorPool> m_pool;
    std::unique_ptr<gfx::vk::DescriptorSetLayout> m_setLayout;
    std::unique_ptr<gfx::mngrs::BindlessManager> m_bindlessManager;

    void initWindow(int width, int height, const std::string& title, const std::string& iconPath);
    void initVulkan();
    void initDescriptors();
};

} // namespace game