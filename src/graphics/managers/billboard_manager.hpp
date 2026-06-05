#pragma once

#include "../billboards/text.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace gfx::mngrs {

class BillboardManager {
public:
    BillboardManager(vk::Device& device, BindlessManager& bindlessManager);
    ~BillboardManager();

    BillboardManager(const BillboardManager&) = delete;
    BillboardManager& operator=(const BillboardManager&) = delete;

    uint32_t getTextureIndex(const std::string& name) const;

    void loadText(const std::string& name, vk::Sampler& sampler, const std::string& fontPath, uint32_t maxChars);

    void drawText(
        VkCommandBuffer commandBuffer,
        const std::vector<billb::Text::InstanceContent>& instancesData);

private:
    vk::Device& m_device;
    BindlessManager& m_bindlessManager;

    std::unordered_map<std::string, std::unique_ptr<billb::Text>> m_texts;
};

} // namespace gfx::mngrs