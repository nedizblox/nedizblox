#include "game_client.hpp"

#include "core/signal_handler.hpp"

namespace game {

GameClient::GameClient(const std::string& server, uint16_t port, const std::string& nickname) {
    core::sighandler::listen();

    initRenderContext();

    initManagers();

    initBasicResources();

    createServices();

    createClient(server, port, nickname);

    createController();
    createTools();

    createLocalRig(nickname);
}

GameClient::~GameClient() {}

void GameClient::initRenderContext() {
    m_renderContext = std::make_unique<RenderContext>(1250, 800, "Nedizblox Client", "assets/textures/logo.png");
}

void GameClient::initManagers() {
    auto& device = m_renderContext->getDevice();
    auto& renderer = m_renderContext->getRenderer();
    auto& bindlessManager = m_renderContext->getBindlessManager();

    m_modelManager = std::make_unique<gfx::mngrs::ModelManager>(device);

    m_billboardManager = std::make_unique<gfx::mngrs::BillboardManager>(device, bindlessManager);

    m_uiManager = std::make_unique<gfx::mngrs::UiManager>(device, bindlessManager);

    m_pipelineManager = std::make_unique<gfx::mngrs::PipelineManager>(
        device, renderer, bindlessManager, *m_modelManager, *m_billboardManager, *m_uiManager);
    m_pipelineManager->initPipelines(m_renderContext->getDescriptorSetLayout().getDescriptorSetLayout());

    m_audioManager = std::make_unique<audio::AudioManager>();

    m_assetManager = std::make_unique<mngrs::AssetManager>(device, bindlessManager);

    m_renderManager = std::make_unique<mngrs::RenderManager>(*m_assetManager);
}

void GameClient::initBasicResources() {
    utils::brl::load(*m_modelManager, *m_uiManager, m_renderContext->getWindow(), *m_billboardManager, *m_assetManager);
}

void GameClient::createController() {
    m_controller = std::make_unique<utils::Controller>(m_renderContext->getWindow(), m_localRig);
}

void GameClient::buildMap(const std::string& rbxlPath) {
    auto instances = utils::rbxl::parseRbxl(rbxlPath);

    for (auto& instance : instances) {
        if (!instance->getParent()) {
            instance->setParent(m_workspace);
        }

        if (instance->getType() == enums::InstanceType::Part) {
            auto part = std::static_pointer_cast<types::Part>(instance);

            m_physics->createRigidBodyPart(part.get());
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
}

void GameClient::createTools() {
    m_devTools = std::make_unique<ui::DevTools>(*m_uiManager, m_game, *m_controller);

    m_debugTools
        = std::make_unique<ui::DebugTools>(*m_uiManager, *m_renderManager, *m_ownership, m_replicator->getClient());
}

void GameClient::createLocalRig(const std::string& nickname) {
    m_localRig = prefabs::Rig::create(*m_physics, nickname, m_replicator->getClient().getPlayerId());
    m_localRig->setParent(m_workspace);
}

void GameClient::createClient(const std::string& server, uint16_t port, const std::string& nickname) {
    m_replicator = std::make_unique<multiplayer::Replicator>(
        server, port, nickname, *m_physics, m_workspace);

    m_ownership = std::make_unique<multiplayer::Ownership>(*m_physics, m_replicator->getClient());
}

void GameClient::run() {
    auto& window = m_renderContext->getWindow();
    auto& renderer = m_renderContext->getRenderer();

    m_replicator->start();

    while (window.isOpen() && !core::sighandler::shouldStop) {
        window.update();

        m_uiManager->update();
        m_audioManager->update();
        m_controller->update();

        float dt = window.getDeltaTime();

        m_physics->step(dt);
        m_physics->stepNetworkInterpolation(dt);

        if (m_renderManager->isMapDirty())
            m_renderManager->rebuildMap(m_workspace);
        if (m_renderManager->isGuiDirty())
            m_renderManager->rebuildGui(m_coreGui);

        m_localRig->update(dt);
        m_replicator->updateNetworkRigs(dt);

        auto outgoingStates = m_ownership->update(dt, m_localRig, m_replicator->getNetworkInstances());
        m_replicator->sendPhysicsState(outgoingStates);

        if (VkCommandBuffer cmd = renderer.beginFrame()) {
            auto projection = m_controller->getCameraProjection();
            auto view = m_controller->getCameraView();

            auto position = m_controller->getCameraPosition();
            auto [at, up] = m_controller->getCameraLook();

            m_audioManager->moveListener(position, at, up);

            renderer.beginRenderPass(cmd);

            m_renderManager->updateDynamicTransforms();

            m_renderManager->sortTransparentInstances(position);

            m_pipelineManager->renderModelsOpaque(
                cmd, projection, view, position, m_renderManager->getOpaqueModelInstancesData());
            m_pipelineManager->renderModelOutlinesOpaque(
                cmd, projection, view, position, m_renderManager->getOpaqueModelOutlineInstancesData());

            m_pipelineManager->renderSkybox(cmd, projection, view, m_assetManager->getCubemapId("skybox"));

            m_pipelineManager->renderModelsTransparent(
                cmd, projection, view, position, m_renderManager->getTransparentModelInstancesData());
            m_pipelineManager->renderModelOutlinesTransparent(
                cmd, projection, view, position, m_renderManager->getTransparentModelOutlineInstancesData());

            m_pipelineManager->renderBillboardTexts(
                cmd, projection, view, m_renderManager->getBillbTextInstancesContent());

            m_pipelineManager->renderTexts(cmd, window, m_renderManager->getTextInstancesContent());

            m_pipelineManager->renderImgui(cmd, window);

            renderer.endRenderPass(cmd);
            renderer.endFrame();
        }
    }

    m_renderContext->waitIdle();
}

} // namespace game
