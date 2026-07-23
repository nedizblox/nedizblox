#include "asset_manager.hpp"

#include <stdexcept>

namespace game::mngrs {

AssetManager::AssetManager(gfx::vk::Device& device, gfx::mngrs::BindlessManager& bindlessManager) :
    m_device(device), m_bindlessManager(bindlessManager) {
    initSamplers();
}

AssetManager::~AssetManager() {}

void AssetManager::initSamplers() {
    auto deviceProperties = m_device.getDeviceProperties();

    m_samplers["repeat"] = gfx::vk::Sampler::Builder(m_device)
                               .setAnisotropy(deviceProperties.limits.maxSamplerAnisotropy)
                               .setMipmaps(VK_SAMPLER_MIPMAP_MODE_LINEAR)
                               .setMaxLod(7.0f)
                               .build();

    m_samplers["skybox"] = gfx::vk::Sampler::Builder(m_device)
                               .setAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
                               .setAnisotropy(deviceProperties.limits.maxSamplerAnisotropy)
                               .setMipmaps(VK_SAMPLER_MIPMAP_MODE_LINEAR)
                               .build();

    m_samplers["ui"]
        = gfx::vk::Sampler::Builder(m_device).setAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE).build();
}

uint32_t AssetManager::loadTexture(
    const std::string& name, const std::string& filePath, const std::string& samplerName,
    bool flipVertically, bool genMipMaps) {
    if (m_textures.contains(name)) {
        return m_textures[name];
    }

    auto& sampler = getSampler(samplerName);
    auto tex = std::make_unique<gfx::Texture>(m_device, sampler, filePath, flipVertically, genMipMaps);

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

gfx::vk::Sampler& AssetManager::getSampler(const std::string& name) const {
    auto it = m_samplers.find(name);
    if (it == m_samplers.end()) {
        throw std::runtime_error("AssetManager: Sampler \"" + name + "\" not found");
    }
    return *it->second;
}

} // namespace game::mngrs