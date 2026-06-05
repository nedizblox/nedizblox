#pragma once

#include "graphics/graphics.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace game::mngrs {

class AssetManager {
public:
    AssetManager(gfx::vk::Device& device, gfx::mngrs::BindlessManager& bindlessManager);
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    uint32_t getTextureId(const std::string& name) const;
    uint32_t getCubemapId(const std::string& name) const;

    void initSamplers();

    uint32_t loadTexture(
        const std::string& name, const std::string& filePath, const std::string& samplerName = "repeat",
        bool flipVertically = false, bool genMipMaps = true);
    uint32_t loadCubemap(
        const std::string& name, const std::array<std::string, 6>& facePaths,
        const std::string& samplerName = "repeat", bool flipVertically = false);
    uint32_t loadCubeFaces(
        const std::string& name, const std::vector<std::string>& facePaths,
        const std::string& samplerName = "repeat", bool flipVertically = false, bool genMipMaps = true);

    void loadFont(const std::string& name, const std::string& filePath, uint32_t maxChars, const std::string& samplerName = "text");
    void loadBillbFont(const std::string& name, const std::string& filePath, uint32_t maxChars, const std::string& samplerName = "text");

    gfx::vk::Sampler& getSampler(const std::string& name);
    gfx::ui::Text& getFont(const std::string& name);

private:
    gfx::vk::Device& m_device;
    gfx::mngrs::BindlessManager& m_bindlessManager;

    std::unordered_map<std::string, std::unique_ptr<gfx::vk::Sampler>> m_samplers;
    std::unordered_map<std::string, uint32_t> m_textures;
    std::unordered_map<std::string, uint32_t> m_cubemaps;

    std::unordered_map<std::string, std::unique_ptr<gfx::ui::Text>> m_fonts;
    std::unordered_map<std::string, std::unique_ptr<gfx::billb::Text>> m_billbFonts;
};

} // namespace game::mngrs