#pragma once

#include "../vk/device.hpp"
#include "../vk/sampler.hpp"

#include "bindless_manager.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace gfx::mngrs {

class TextureManager {
public:
    TextureManager(
        gfx::vk::Device& device, gfx::mngrs::BindlessManager& bindlessManager
    );
    ~TextureManager();

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    uint32_t getTextureId(const std::string& name) const;
    uint32_t getCubemapId(const std::string& name) const;

    uint32_t loadTexture(
        const std::string& name, const std::string& filePath,
        const std::string& samplerName = "repeat", bool flipVertically = false,
        bool genMipMaps = true
    );
    uint32_t loadCubemap(
        const std::string& name, const std::array<std::string, 6>& facePaths,
        const std::string& samplerName = "repeat", bool flipVertically = false
    );

    gfx::vk::Sampler& getSampler(const std::string& name) const;

private:
    gfx::vk::Device& m_device;
    gfx::mngrs::BindlessManager& m_bindlessManager;

    std::unordered_map<std::string, std::unique_ptr<gfx::vk::Sampler>>
        m_samplers;
    std::unordered_map<std::string, uint32_t> m_textures;
    std::unordered_map<std::string, uint32_t> m_cubemaps;

    void initSamplers();
};

} // namespace gfx::mngrs