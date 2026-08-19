#include "game_client.hpp"

namespace game {

GameClient::GameClient(const std::string& server, uint16_t port, const std::string& nickname) {
    initRenderContext();

    initManagers();
    loadTextures();
    loadModels();
    loadUis();

    createServices();

    createClient(server, port, nickname);

    createLocalRig(nickname);
    createCamera();

    core::sighandler::listen();
}

GameClient::~GameClient() {}

void GameClient::initRenderContext() {
    m_renderContext = std::make_unique<RenderContext>(1250, 800, "Nedizblox Client", "assets/textures/logo.png");
}

void GameClient::initManagers() {
    auto& device = m_renderContext->getDevice();
    auto& renderer = m_renderContext->getRenderer();
    auto& bindlessManager = m_renderContext->getBindlessManager();

    m_assetManager = std::make_unique<mngrs::AssetManager>(device, bindlessManager);

    m_instanceManager = std::make_unique<mngrs::InstanceManager>(*m_assetManager);

    m_modelManager = std::make_unique<gfx::mngrs::ModelManager>(device);

    m_billboardManager = std::make_unique<gfx::mngrs::BillboardManager>(device, bindlessManager);

    m_uiManager = std::make_unique<gfx::mngrs::UiManager>(device, bindlessManager);

    m_renderManager = std::make_unique<mngrs::RenderManager>(
        device, renderer, bindlessManager, *m_modelManager, *m_billboardManager, *m_uiManager);
    m_renderManager->initPipelines(m_renderContext->getDescriptorSetLayout().getDescriptorSetLayout());

    m_audioManager = std::make_unique<audio::AudioManager>();
}

void GameClient::loadTextures() {
    m_assetManager->loadTexture("smooth", "assets/textures/smooth.png");
    m_assetManager->loadTexture("studs", "assets/textures/studs.png");
    m_assetManager->loadTexture("inlets", "assets/textures/inlets.png");
    m_assetManager->loadTexture("glue", "assets/textures/glue.png");

    m_assetManager->loadTexture("grass", "assets/textures/grass.png");
    m_assetManager->loadTexture("wood", "assets/textures/wood.png");

    m_assetManager->loadTexture("headFace", "assets/textures/face.png", "repeat", true, false);
    m_assetManager->loadTexture("nedizbloxTshirt", "assets/textures/logo_tshirt.png", "repeat", true, false);
    m_assetManager->loadTexture("spawnLocation", "assets/textures/spawn_location.png", "repeat", true, false);

    std::array<std::string, 6> skyboxFaces
        = {"assets/textures/skybox/rt.png", "assets/textures/skybox/lf.png",
           "assets/textures/skybox/up.png", "assets/textures/skybox/dn.png",
           "assets/textures/skybox/ft.png", "assets/textures/skybox/bk.png"};
    m_assetManager->loadCubemap("skybox", skyboxFaces, "skybox");
}

void GameClient::loadModels() {
    auto& device = m_renderContext->getDevice();

    m_modelManager->loadSkybox();

    m_modelManager->loadModel("cube", gfx::geoms::cube);
    m_modelManager->loadModel("sphere", gfx::geoms::sphere);
    m_modelManager->loadModel("cylinder", gfx::geoms::cylinder);
    m_modelManager->loadModel("wedge", gfx::geoms::wedge);

    m_modelManager->loadModelOutline("cube", gfx::geoms::cube);
    m_modelManager->loadModelOutline("sphere", gfx::geoms::sphere);
    m_modelManager->loadModelOutline("cylinder", gfx::geoms::cylinder);
    m_modelManager->loadModelOutline("wedge", gfx::geoms::wedge);

    m_modelManager->loadModel("head", "assets/models/head.obj");
}

void GameClient::loadUis() {
    m_imgui = std::make_unique<gfx::ui::Imgui>(
        m_renderContext->getWindow(), m_renderContext->getDevice(),
        m_assetManager->getSampler("ui"), m_renderContext->getBindlessManager());

    m_uiManager->loadText("nunito", m_assetManager->getSampler("ui"), "assets/fonts/Nunito.ttf", 50);

    m_billboardManager->loadText("nunito", m_assetManager->getSampler("repeat"), "assets/fonts/Nunito.ttf", 32);
}

void GameClient::createCamera() {
    m_sphericalCamera = std::make_unique<core::camera::SphericalCamera>();
    m_freeCamera = std::make_unique<core::camera::FreeCamera>();

    m_controller = std::make_unique<utils::Controller>(
        m_renderContext->getWindow(), *m_imgui, *m_freeCamera, *m_sphericalCamera, m_localRig);
}

void GameClient::buildMap(const std::string& rbxlPath) {
    auto instances = utils::rbxl::parseRbxl(rbxlPath);

    for (auto& instance : instances) {
        if (!instance->getParent()) {
            instance->setParent(m_workspace);
        }

        if (auto part = std::static_pointer_cast<types::Part>(instance)) {
            m_physics->createRigidBodyPart(part.get());
        }
    }
}

void GameClient::createServices() {
    m_game = std::make_shared<types::Game>();

    m_workspace = std::make_shared<types::Workspace>();
    m_workspace->onChildrenChanged(
        [this](std::shared_ptr<types::Workspace> newChild) { m_instanceManager->markMapDirty(); });
    m_workspace->setParent(m_game);

    m_coreGui = std::make_shared<types::CoreGui>();
    m_coreGui->onChildrenChanged(
        [this](std::shared_ptr<types::CoreGui> newChild) { m_instanceManager->markGuiDirty(); });
    m_workspace->setParent(m_game);

    m_physics = std::make_unique<physics::Physics>(m_workspace);

    m_devTools = std::make_unique<utils::DevTools>(*m_imgui, m_game);
}

void GameClient::createLocalRig(const std::string& nickname) {
    m_localRig = prefabs::Rig::create(*m_physics, nickname, m_replicator->getClient().getPlayerId());
    m_localRig->setParent(m_workspace);
}

void GameClient::createClient(const std::string& server, uint16_t port, const std::string& nickname) {
    m_replicator = std::make_unique<multiplayer::Replicator>(
        server, port, nickname, *m_physics, *m_instanceManager, m_workspace);

    m_ownership = std::make_unique<multiplayer::Ownership>(*m_physics, m_replicator->getClient());
}

void GameClient::run() {
    auto& window = m_renderContext->getWindow();
    auto& renderer = m_renderContext->getRenderer();

    m_replicator->start();

    while (window.isOpen() && !core::sighandler::shouldStop) {
        window.update();
        m_imgui->update();
        m_audioManager->update();
        m_controller->update();

        float dt = window.getDeltaTime();

        m_physics->step(dt);
        m_physics->stepNetworkInterpolation(dt);

        if (m_instanceManager->isMapDirty())
            m_instanceManager->rebuildMap(m_workspace);
        if (m_instanceManager->isGuiDirty())
            m_instanceManager->rebuildGui(m_coreGui);

        m_localRig->update(dt);
        m_replicator->updateNetworkRigs(dt);

        auto head = m_localRig->getHead();
        if (head)
            m_sphericalCamera->setTarget(head->getPosition());

        auto outgoingStates = m_ownership->update(m_localRig, m_replicator->getNetworkParts());
        m_replicator->sendPhysicsState(outgoingStates);

        if (VkCommandBuffer cmd = renderer.beginFrame()) {
            glm::mat4 projection{};
            glm::mat4 view{};

            glm::vec3 position{};
            glm::vec3 at{}, up{};

            if (m_controller->isFreeCameraMode()) {
                projection = m_freeCamera->getProjection();
                view = m_freeCamera->getView();

                position = m_freeCamera->getPosition();
                at = m_freeCamera->getFront();
                up = m_freeCamera->getUp();
            } else {
                projection = m_sphericalCamera->getProjection();
                view = m_sphericalCamera->getView();

                position = m_sphericalCamera->getPosition();
                at = m_sphericalCamera->getFront();
                up = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            m_audioManager->moveListener(position, at, up);

            renderer.beginRenderPass(cmd);

            m_instanceManager->updateDynamicTransforms();

            m_instanceManager->sortTransparentInstances(position);

            m_renderManager->renderModelsOpaque(cmd, projection, view, position, m_instanceManager->getOpaqueModelInstancesData());
            m_renderManager->renderModelOutlinesOpaque(cmd, projection, view, position, m_instanceManager->getOpaqueModelOutlineInstancesData());

            m_renderManager->renderSkybox(cmd, projection, view, m_assetManager->getCubemapId("skybox"));

            m_renderManager->renderModelsTransparent(cmd, projection, view, position, m_instanceManager->getTransparentModelInstancesData());
            m_renderManager->renderModelOutlinesTransparent(cmd, projection, view, position, m_instanceManager->getTransparentModelOutlineInstancesData());

            m_renderManager->renderBillboardTexts(cmd, projection, view, m_instanceManager->getBillbTextInstancesContent());

            m_renderManager->renderTexts(cmd, window, m_instanceManager->getTextInstancesContent());

            m_renderManager->renderImgui(cmd, window, *m_imgui);

            renderer.endRenderPass(cmd);
            renderer.endFrame();
        }
    }

    m_renderContext->waitIdle();
}

} // namespace game