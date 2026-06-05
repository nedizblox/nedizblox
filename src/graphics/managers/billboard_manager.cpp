#include "billboard_manager.hpp"

namespace gfx::mngrs {

BillboardManager::BillboardManager(vk::Device& device, BindlessManager& bindlessManager) :
    m_device(device), m_bindlessManager(bindlessManager) {}

BillboardManager::~BillboardManager() {}

uint32_t BillboardManager::getTextureIndex(const std::string& name) const {
    if (auto it = m_texts.find(name); it != m_texts.end()) {
        return it->second->getTextureIndex();
    }
    
    return 0;
}

void BillboardManager::loadText(const std::string& name, vk::Sampler& sampler, const std::string& fontPath, uint32_t maxChars) {
    if (m_texts.contains(name))
        return;

    m_texts[name] = std::make_unique<billb::Text>(m_device, sampler, m_bindlessManager, fontPath, maxChars);
}

void BillboardManager::drawText(VkCommandBuffer commandBuffer, const std::vector<billb::Text::InstanceContent>& instancesData) {
    for (auto& [name, text] : m_texts) {
        text->draw(commandBuffer, instancesData);
    }
}

} // namespace gfx::mngrs