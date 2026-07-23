#pragma once

#include "audio/audio.hpp"
#include "core/core.hpp"
#include "graphics/graphics.hpp"
#include "network/network.hpp"

#include "prefabs/prefabs.hpp"
#include "types/types.hpp"

#include "engines/engines.hpp"
#include "managers/managers.hpp"
#include "physics/physics.hpp"

#include <deque>
#include <memory>
#include <string>

namespace game {

class GameClient {
public:
    GameClient(const std::string& server, uint16_t port, const std::string& nickname);
    ~GameClient();

    GameClient(const GameClient&) = delete;
    GameClient& operator=(const GameClient&) = delete;

    void buildMap(const std::string& rbxlPath); // for map redacting

    void run();

private:
    std::unique_ptr<win::Window> m_window;
    std::unique_ptr<gfx::vk::Device> m_device;
    std::unique_ptr<gfx::vk::Renderer> m_renderer;

    std::unique_ptr<gfx::vk::DescriptorPool> m_pool;
    std::unique_ptr<gfx::vk::DescriptorSetLayout> m_setLayout;
    std::unique_ptr<gfx::mngrs::BindlessManager> m_bindlessManager;

    std::unique_ptr<mngrs::AssetManager> m_assetManager;
    std::unique_ptr<mngrs::InstanceManager> m_instanceManager;
    std::unique_ptr<gfx::mngrs::ModelManager> m_modelManager;
    std::unique_ptr<gfx::mngrs::BillboardManager> m_billboardManager;
    std::unique_ptr<gfx::mngrs::UiManager> m_uiManager;
    std::unique_ptr<audio::AudioManager> m_audioManager;

    std::unique_ptr<gfx::ui::Imgui> m_imgui;

    std::unique_ptr<engines::RenderEngine> m_renderEngine;
    std::unique_ptr<engines::ScriptEngine> m_scriptEngine;

    std::unique_ptr<net::Client> m_client;

    std::unique_ptr<gfx::Skybox> m_skybox;

    std::unique_ptr<core::camera::SphericalCamera> m_camera;

    std::unique_ptr<physics::Physics> m_physics;
    std::shared_ptr<types::Workspace> m_workspace;
    std::shared_ptr<types::CoreGui> m_coreGui;

    std::shared_ptr<prefabs::Rig> m_localRig;
    std::unordered_map<uint32_t, std::shared_ptr<prefabs::Rig>> m_networkRigs;

    std::unordered_map<uint32_t, std::shared_ptr<types::Part>> m_networkParts;

    std::shared_ptr<types::Text> m_fps;

    static constexpr size_t kMaxLocallyOwnedObjects = 32;
    std::deque<uint32_t> m_ownershipOrder;

    void initWindow();
    void initVulkan();
    void initDescriptors();
    void initManagers();
    void initEngines();
    void loadTextures();
    void loadModels();
    void loadUis();

    void createCamera();

    void createServices();
    void createLocalRig(const std::string& nickname);

    void createDebugUi();

    void createClient(const std::string& server, uint16_t port, const std::string& nickname);

    void sendLocalPhysicsState();

    static void signalHandler(int sig);
};

} // namespace game