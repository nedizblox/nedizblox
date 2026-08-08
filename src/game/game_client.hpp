#pragma once

#include "audio/audio.hpp"
#include "core/core.hpp"
#include "scripting/scripting.hpp"
#include "graphics/graphics.hpp"

#include "render_context.hpp"

#include "prefabs/prefabs.hpp"
#include "types/types.hpp"
#include "utils/utils.hpp"

#include "managers/managers.hpp"
#include "physics/physics.hpp"

#include "multiplayer/multiplayer.hpp"

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
    std::unique_ptr<RenderContext> m_renderContext;

    std::unique_ptr<mngrs::AssetManager> m_assetManager;
    std::unique_ptr<mngrs::InstanceManager> m_instanceManager;
    std::unique_ptr<mngrs::RenderManager> m_renderManager;
    std::unique_ptr<gfx::mngrs::ModelManager> m_modelManager;
    std::unique_ptr<gfx::mngrs::BillboardManager> m_billboardManager;
    std::unique_ptr<gfx::mngrs::UiManager> m_uiManager;
    std::unique_ptr<audio::AudioManager> m_audioManager;

    std::unique_ptr<multiplayer::Replicator> m_replicator;
    std::unique_ptr<multiplayer::Ownership> m_ownership;

    std::unique_ptr<gfx::ui::Imgui> m_imgui;
    std::unique_ptr<utils::DevTools> m_devTools;

    std::unique_ptr<core::camera::SphericalCamera> m_sphericalCamera;
    std::unique_ptr<core::camera::FreeCamera> m_freeCamera;
    std::unique_ptr<utils::Controller> m_controller;

    std::unique_ptr<physics::Physics> m_physics;

    std::shared_ptr<types::Game> m_game;
    std::shared_ptr<types::Workspace> m_workspace;
    std::shared_ptr<types::CoreGui> m_coreGui;

    std::shared_ptr<prefabs::Rig> m_localRig;

    void initRenderContext();
    void initManagers();
    void loadTextures();
    void loadModels();
    void loadUis();

    void createCamera();

    void createServices();
    void createLocalRig(const std::string& nickname);

    void createClient(const std::string& server, uint16_t port, const std::string& nickname);
};

} // namespace game