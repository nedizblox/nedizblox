#pragma once

#include "../ui/text.hpp"

#include <unordered_map>

namespace gfx::mngrs {

class UiManager {
public:
    UiManager(vk::Device& device, BindlessManager& bindlessManager);
    ~UiManager();

    UiManager(const UiManager&) = delete;
    UiManager& operator=(const UiManager&) = delete;

    uint32_t getTextureIndex(const std::string& name) const;

    void loadText(const std::string& name, vk::Sampler& sampler, const std::string& fontPath, uint32_t maxChars);

    void drawText(VkCommandBuffer commandBuffer, const std::vector<ui::Text::InstanceContent>& instancesData);

private:
    vk::Device& m_device;
    BindlessManager& m_bindlessManager;

    std::unordered_map<std::string, std::unique_ptr<ui::Text>> m_texts;
};

} // namespace gfx::mngrs