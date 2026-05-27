#pragma once

#include "audio/audio.hpp"
#include "core/camera.hpp"
#include "core/core.hpp"
#include "game/prefabs/rig.hpp"
#include "graphics/graphics.hpp"

#include "graphics/managers/model_manager.hpp"
#include "types/types.hpp"

#include "managers/managers.hpp"
#include "physics/physics.hpp"

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

    void buildMap(const std::string& rbxlPath);

    void run();

private:
    // graphics
    std::unique_ptr<win::Window> m_window;

    std::unique_ptr<gfx::vk::Device> m_device;
    std::unique_ptr<gfx::vk::Renderer> m_renderer;

    std::unique_ptr<gfx::vk::DescriptorPool> m_pool;
    std::unique_ptr<gfx::vk::DescriptorSetLayout> m_setLayout;
    std::unique_ptr<gfx::mngrs::BindlessManager> m_bindlessManager;

    std::unordered_map<std::string, std::unique_ptr<gfx::vk::Pipeline>> m_pipelines;

    std::unordered_map<std::string, std::unique_ptr<gfx::vk::Sampler>> m_samplers;
    std::unordered_map<std::string, uint32_t> m_textures;
    std::unordered_map<std::string, uint32_t> m_cubemaps;

    std::unique_ptr<gfx::Skybox> m_skybox;
    std::unique_ptr<gfx::mngrs::ModelManager> m_modelManager;
    std::unordered_map<std::string, std::unique_ptr<gfx::ui::Text>> m_texts;

    std::unordered_map<std::string, std::vector<gfx::Model::InstanceData>> m_instancesData;

    std::unique_ptr<core::SphericalCamera> m_camera;

    // audio
    audio::AudioManager m_audioManager;
    std::unordered_set<ALuint> m_sounds;

    // engine
    ScriptManager m_scriptManager;

    std::shared_ptr<types::Instance> m_workspace;

    std::vector<std::shared_ptr<types::Part>> m_dynamicParts;

    struct DynamicTarget {
        std::shared_ptr<types::Part> part;
        std::string bucketName;
        size_t index;
    };
    std::vector<DynamicTarget> m_dynamicTargets;

    std::shared_ptr<prefabs::Rig> m_rig;

    bool m_hierarchyDirty = true;

    bool m_debugScreenToggled = false;

    std::unique_ptr<physics::Physics> m_physics;

    // utils
    void collectInstances(const std::shared_ptr<types::Instance>& parent);
    void sortInstances(std::vector<gfx::Model::InstanceData>& instances);

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