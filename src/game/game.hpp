#pragma once

#include "audio/audio.hpp"
#include "core/core.hpp"
#include "graphics/graphics.hpp"

#include "types/types.hpp"

#include "physics/physics.hpp"
#include "script_manager.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace game {

class Game {
public:
    Game();
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    void run();

private:
    // graphics
    std::unique_ptr<win::Window> m_window;

    std::unique_ptr<gfx::vk::Device> m_device;
    std::unique_ptr<gfx::vk::Renderer> m_renderer;

    std::unique_ptr<gfx::vk::DescriptorPool> m_pool;
    std::unique_ptr<gfx::vk::DescriptorSetLayout> m_setLayout;
    std::unique_ptr<gfx::BindlessManager> m_bindlessManager;

    std::unordered_map<std::string, std::unique_ptr<gfx::vk::Pipeline>> m_pipelines;

    std::unordered_map<std::string, std::unique_ptr<gfx::vk::Sampler>> m_samplers;
    std::unordered_map<std::string, uint32_t> m_textures;
    std::unordered_map<std::string, uint32_t> m_cubemaps;

    std::unique_ptr<gfx::Skybox> m_skybox;
    std::unordered_map<std::string, std::unique_ptr<gfx::Model>> m_models;
    std::unordered_map<std::string, std::unique_ptr<gfx::ui::Text>> m_texts;

    std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>> m_instancesData;

    core::FreeCamera m_camera;

    // audio
    audio::AudioManager m_audioManager;
    std::unordered_set<ALuint> m_sounds;

    // engine
    ScriptManager m_scriptManager;

    std::shared_ptr<types::Instance> m_workspace;

    std::unique_ptr<physics::Physics> m_physics;

    // utils
    void collectInstances(const std::shared_ptr<types::Instance>& parent);
    void sortInstances();
    void clearInstances();

    void buildMap();

    // init
    void initWindow();
    void initVulkan();
    void initDescriptors();
    void initPipelines();
    void loadSamplers();
    void loadTextures();
    void loadModels();
    void loadTexts();
};

} // namespace game