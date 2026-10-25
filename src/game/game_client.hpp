#pragma once

#include "audio/audio.hpp"
// #include "scripting/scripting.hpp"
#include "graphics/graphics.hpp"

#include "render_context.hpp"

#include "types/types.hpp"
#include "ui/ui.hpp"
#include "utils/utils.hpp"

#include "managers/managers.hpp"
#include "physics/physics.hpp"

#include "multiplayer/multiplayer.hpp"

namespace game {

class GameClient {
public:
    GameClient(
        const std::string& server, uint16_t port, const std::string& nickname,
        bool debugTools = false
    );                                   // client mode
    GameClient(bool debugTools = false); // studio mode
    ~GameClient();

    GameClient(const GameClient&) = delete;
    GameClient& operator=(const GameClient&) = delete;

    void buildMap(const std::string& nbxlPath);

    void run();

private:
    std::unique_ptr<RenderContext> m_renderContext;

    std::unique_ptr<gfx::mngrs::ModelManager> m_modelManager;
    std::unique_ptr<gfx::mngrs::BillboardManager> m_billboardManager;
    std::unique_ptr<gfx::mngrs::TextureManager> m_textureManager;
    std::unique_ptr<gfx::mngrs::UiManager> m_uiManager;
    std::unique_ptr<gfx::mngrs::PipelineManager> m_pipelineManager;

    std::unique_ptr<audio::AudioManager> m_audioManager;

    std::unique_ptr<mngrs::RenderManager> m_renderManager;

    std::unique_ptr<multiplayer::ClientReplicator> m_replicator;
    std::unique_ptr<multiplayer::ClientOwnership> m_ownership;

    std::unique_ptr<ui::DevTools> m_devTools;
    std::unique_ptr<ui::DebugTools> m_debugTools;
    std::unique_ptr<ui::GameInterface> m_gameInterface;

    std::unique_ptr<utils::Controller> m_controller;

    std::unique_ptr<physics::Physics> m_physics;
    std::unique_ptr<mngrs::HumanoidManager> m_humanoidManager;

    std::shared_ptr<types::Game> m_game;
    std::shared_ptr<types::Workspace> m_workspace;
    std::shared_ptr<types::CoreGui> m_coreGui;

    std::shared_ptr<types::Rig> m_localRig;

    bool m_studioMode = false;

    void initRenderContext();

    void initManagers();

    void initBasicResources();

    void createController();

    void createServices();
    
    void createDebugTools();
    void createDevTools();
    void createGameInterface();

    void createLocalRig(const std::string& nickname);

    void createClient(
        const std::string& server = "", uint16_t port = 0,
        const std::string& nickname = ""
    );
};

} // namespace game