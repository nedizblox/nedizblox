#include "asset_manager.hpp"

#include <stdexcept>

namespace game::mngrs {

AssetManager::AssetManager(gfx::vk::Device& device, gfx::mngrs::BindlessManager& bindlessManager) :
    m_device(device), m_bindlessManager(bindlessManager) {}

AssetManager::~AssetManager() {}

void AssetManager::initSamplers() {
    m_samplers["repeat"] = gfx::vk::Sampler::Builder(m_device)
                               .setAnisotropy(m_device.properties.limits.maxSamplerAnisotropy)
                               .setMipmaps(VK_SAMPLER_MIPMAP_MODE_LINEAR)
                               .setMaxLod(7.0f)
                               .build();

    m_samplers["skybox"] = gfx::vk::Sampler::Builder(m_device)
                               .setAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
                               .setAnisotropy(m_device.properties.limits.maxSamplerAnisotropy)
                               .setMipmaps(VK_SAMPLER_MIPMAP_MODE_LINEAR)
                               .build();

    m_samplers["text"] = gfx::vk::Sampler::Builder(m_device)
                             .setAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
                             .setMaxLod(1.0f)
                             .build();
}

uint32_t AssetManager::loadTexture(
    const std::string& name, const std::string& filePath, const std::string& samplerName, bool flipVertically) {
    if (m_textures.contains(name)) {
        return m_textures[name];
    }

    auto& sampler = getSampler(samplerName);
    auto tex = std::make_unique<gfx::Texture>(m_device, sampler, filePath, flipVertically);

    uint32_t index = m_bindlessManager.addTexture(std::move(tex));
    m_textures[name] = index;

    return index;
}

uint32_t AssetManager::loadCubemap(
    const std::string& name, const std::array<std::string, 6>& facePaths,
    const std::string& samplerName, bool flipVertically) {
    if (m_cubemaps.contains(name)) {
        return m_cubemaps[name];
    }

    auto& sampler = getSampler(samplerName);
    auto cubemap = std::make_unique<gfx::Cubemap>(m_device, sampler, facePaths, flipVertically);

    uint32_t index = m_bindlessManager.addCubemap(std::move(cubemap));
    m_cubemaps[name] = index;

    return index;
}

uint32_t AssetManager::loadCubeFaces(
    const std::string& name, const std::vector<std::string>& facePaths,
    const std::string& samplerName, bool flipVertically) {
    if (m_textures.contains(name)) {
        return m_textures[name];
    }

    auto& sampler = getSampler(samplerName);
    uint32_t baseIndex = 0;

    for (size_t i = 0; i < facePaths.size(); i++) {
        auto tex = std::make_unique<gfx::Texture>(m_device, sampler, facePaths[i], flipVertically);
        uint32_t registeredIndex = m_bindlessManager.addTexture(std::move(tex));

        if (i == 0) {
            baseIndex = registeredIndex;
        }
    }

    m_textures[name] = baseIndex;

    return baseIndex;
}

void AssetManager::loadFont(const std::string& name, const std::string& filePath, uint32_t maxChars, const std::string& samplerName) {
    if (m_fonts.contains(name))
        return;

    auto& sampler = getSampler(samplerName);
    m_fonts[name] = std::make_unique<gfx::ui::Text>(m_device, sampler, m_bindlessManager, filePath, maxChars);
}

uint32_t AssetManager::getTextureId(const std::string& name) const {
    auto it = m_textures.find(name);
    if (it == m_textures.end()) {
        throw std::runtime_error("AssetManager: Texture \"" + name + "\" not found");
    }
    return it->second;
}

uint32_t AssetManager::getCubemapId(const std::string& name) const {
    auto it = m_cubemaps.find(name);
    if (it == m_cubemaps.end()) {
        throw std::runtime_error("AssetManager: Cubemap \"" + name + "\" not found");
    }
    return it->second;
}

gfx::vk::Sampler& AssetManager::getSampler(const std::string& name) {
    auto it = m_samplers.find(name);
    if (it == m_samplers.end()) {
        throw std::runtime_error("AssetManager: Sampler \"" + name + "\" not found");
    }
    return *it->second;
}

gfx::ui::Text& AssetManager::getFont(const std::string& name) {
    auto it = m_fonts.find(name);
    if (it == m_fonts.end()) {
        throw std::runtime_error("AssetManager: Font \"" + name + "\" not found");
    }
    return *it->second;
}

} // namespace game::mngrs