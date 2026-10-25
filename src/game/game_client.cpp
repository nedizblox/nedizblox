#include "game_client.hpp"

#include "core/signal_handler.hpp"

namespace {

const glm::vec3 SHADOW_LIGHT_DIR =
    glm::normalize(glm::vec3(0.30f, 0.90f, 0.40f));

constexpr float SHADOW_RADIUS = 60.0f;
constexpr float SHADOW_DEPTH = 1000.0f;

} // namespace

namespace game {

GameClient::GameClient(
    const std::string& server, uint16_t port, const std::string& nickname,
    bool debugTools
) {
    core::sighandler::listen();

    initRenderContext();

    initManagers();

    initBasicResources();

    createServices();

    createClient(server, port, nickname);

    createLocalRig(nickname);

    createController();

    if (debugTools)
        createDebugTools();

    createGameInterface();
}

GameClient::GameClient(bool debugTools) {
    core::sighandler::listen();

    m_studioMode = true;

    initRenderContext();

    initManagers();

    initBasicResources();

    createServices();

    createClient();

    createController();

    createDevTools();

    if (debugTools)
        createDebugTools();
}

GameClient::~GameClient() {
    if (m_replicator)
        m_replicator->stop();
}

void GameClient::initRenderContext() {
    m_renderContext = std::make_unique<RenderContext>(
        1250, 800, "Nedizblox Client", "assets/textures/logo.png"
    );
}

void GameClient::initManagers() {
    auto& device = m_renderContext->getDevice();
    auto& renderer = m_renderContext->getRenderer();
    auto& bindlessManager = m_renderContext->getBindlessManager();

    m_modelManager = std::make_unique<gfx::mngrs::ModelManager>(device);

    m_billboardManager =
        std::make_unique<gfx::mngrs::BillboardManager>(device, bindlessManager);

    m_textureManager =
        std::make_unique<gfx::mngrs::TextureManager>(device, bindlessManager);

    m_uiManager =
        std::make_unique<gfx::mngrs::UiManager>(device, bindlessManager);

    m_pipelineManager = std::make_unique<gfx::mngrs::PipelineManager>(
        device, renderer, bindlessManager, *m_modelManager, *m_billboardManager,
        *m_uiManager
    );
    m_pipelineManager->initPipelines(
        m_renderContext->getDescriptorSetLayout().getDescriptorSetLayout()
    );

    m_audioManager = std::make_unique<audio::AudioManager>();

    m_renderManager = std::make_unique<mngrs::RenderManager>(*m_textureManager);
}

void GameClient::initBasicResources() {
    utils::brl::load(
        *m_modelManager, *m_uiManager, m_renderContext->getWindow(),
        *m_billboardManager, *m_textureManager
    );
}

void GameClient::createController() {
    m_controller = std::make_unique<utils::Controller>(
        m_renderContext->getWindow(), *m_humanoidManager, m_localRig
    );

    if (m_studioMode)
        m_controller->setFreeCameraMode(true);
}

void GameClient::buildMap(const std::string& nbxlPath) {
    auto instances = utils::nbxf::parseNbxl(nbxlPath);

    for (auto& instance : instances) {
        if (!instance->getParent()) {
            instance->setParent(m_workspace);
        }
    }
}

void GameClient::createServices() {
    m_game = std::make_shared<types::Game>();

    m_workspace = std::make_shared<types::Workspace>();
    m_workspace->setParent(m_game);

    m_coreGui = std::make_shared<types::CoreGui>();
    m_coreGui->setParent(m_game);

    m_physics = std::make_unique<physics::Physics>(m_workspace);

    m_humanoidManager =
        std::make_unique<mngrs::HumanoidManager>(*m_physics, m_workspace);
}

void GameClient::createDebugTools() {
    m_debugTools = std::make_unique<ui::DebugTools>(
        *m_uiManager, *m_renderManager, *m_ownership, *m_replicator
    );
}

void GameClient::createDevTools() {
    m_devTools = std::make_unique<ui::DevTools>(
        *m_uiManager, m_game, *m_controller, *m_physics, *m_humanoidManager
    );
}

void GameClient::createGameInterface() {
    m_gameInterface =
        std::make_unique<ui::GameInterface>(*m_uiManager, *m_replicator);
}

void GameClient::createLocalRig(const std::string& nickname) {
    m_localRig = m_humanoidManager->createHumanoid(
        nickname, m_replicator->getPlayerId(), true
    );
}

void GameClient::createClient(
    const std::string& server, uint16_t port, const std::string& nickname
) {
    m_replicator = std::make_unique<multiplayer::ClientReplicator>(
        server, port, nickname, *m_physics, *m_humanoidManager, m_workspace
    );

    m_ownership = std::make_unique<multiplayer::ClientOwnership>(
        *m_physics, m_replicator->getPlayerId()
    );
}

void GameClient::run() {
    auto& window = m_renderContext->getWindow();
    auto& renderer = m_renderContext->getRenderer();

    if (!m_studioMode)
        m_physics->start();

    m_replicator->start();

    while (window.isOpen() && !core::sighandler::shouldStop) {
        window.update();

        m_uiManager->update();
        m_audioManager->update();
        m_controller->update();

        float dt = window.getDeltaTime();

        m_physics->update(dt);

        if (m_renderManager->isMapDirty())
            m_renderManager->rebuildMap(m_workspace);
        if (m_renderManager->isGuiDirty())
            m_renderManager->rebuildGui(m_coreGui);

        m_humanoidManager->update(dt);

        auto outgoingStates = m_ownership->update(
            dt, m_localRig, m_replicator->getNetworkInstances()
        );
        m_replicator->sendPhysicsState(outgoingStates);

        if (VkCommandBuffer cmd = renderer.beginFrame()) {
            auto projection = m_controller->getCameraProjection();
            auto view = m_controller->getCameraView();

            auto position = m_controller->getCameraPosition();
            auto [at, up] = m_controller->getCameraLook();

            m_audioManager->moveListener(position, at, up);

            m_renderManager->updateDynamicTransforms();

            m_renderManager->cullInstances(position);

            glm::mat4 lightSpace = gfx::ShadowMap::computeLightSpace(
                SHADOW_LIGHT_DIR, position, SHADOW_RADIUS, SHADOW_DEPTH,
                m_pipelineManager->getShadowMapSize()
            );
            m_pipelineManager->renderShadowPass(
                cmd, lightSpace, SHADOW_RADIUS,
                m_renderManager->getOpaqueModelInstancesData()
            );

            renderer.beginRenderPass(cmd);

            m_pipelineManager->renderModelsOpaque(
                cmd, projection, view, position,
                m_renderManager->getOpaqueModelInstancesData()
            );
            m_pipelineManager->renderModelOutlinesOpaque(
                cmd, projection, view, position,
                m_renderManager->getOpaqueModelOutlineInstancesData()
            );

            m_pipelineManager->renderSkybox(
                cmd, projection, view, m_textureManager->getCubemapId("skybox")
            );

            m_pipelineManager->renderModelsTransparent(
                cmd, projection, view, position,
                m_renderManager->getTransparentModelInstancesData()
            );
            m_pipelineManager->renderModelOutlinesTransparent(
                cmd, projection, view, position,
                m_renderManager->getTransparentModelOutlineInstancesData()
            );

            m_pipelineManager->renderBillboardTexts(
                cmd, projection, view,
                m_renderManager->getBillbTextInstancesContent()
            );

            m_pipelineManager->renderTexts(
                cmd, window, m_renderManager->getTextInstancesContent()
            );

            m_pipelineManager->renderImgui(cmd, window);

            renderer.endRenderPass(cmd);
            renderer.endFrame();
        }
    }

    m_renderContext->waitIdle();
}

} // namespace game