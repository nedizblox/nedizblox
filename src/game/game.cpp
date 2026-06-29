#include "game.hpp"

#include "utils/rbxl.hpp"

#include <format>

namespace game {

Game::Game() {
    try {
        initWindow();
        initVulkan();
        initDescriptors();
        initManagers();
        initEngines();
        loadTextures();
        loadModels();
        loadUis();

        m_camera = std::make_unique<core::camera::SphericalCamera>();
    } catch (std::exception& e) { throw; }
}

Game::~Game() {}

void Game::initWindow() {
    m_window = std::make_unique<win::Window>(1250, 800, "Nedizblox");
    m_window->setIcon("assets/textures/logo.png");
}

void Game::initVulkan() {
    m_device = std::make_unique<gfx::vk::Device>(*m_window);

    m_renderer = std::make_unique<gfx::vk::Renderer>(*m_device, *m_window);
}

void Game::initDescriptors() {
    m_pool = gfx::vk::DescriptorPool::Builder(*m_device)
                 .setMaxSets(100)
                 .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
                 .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000)
                 .build();

    m_setLayout
        = gfx::vk::DescriptorSetLayout::Builder(*m_device)
              .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1000)
              .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 10)
              .build();
}

void Game::initManagers() {
    m_bindlessManager = std::make_unique<gfx::mngrs::BindlessManager>(*m_device, *m_setLayout, *m_pool);

    m_assetManager = std::make_unique<mngrs::AssetManager>(*m_device, *m_bindlessManager);
    m_assetManager->initSamplers();

    m_instanceManager = std::make_unique<mngrs::InstanceManager>();

    m_modelManager = std::make_unique<gfx::mngrs::ModelManager>(*m_device);

    m_billboardManager = std::make_unique<gfx::mngrs::BillboardManager>(*m_device, *m_bindlessManager);

    m_uiManager = std::make_unique<gfx::mngrs::UiManager>(*m_device, *m_bindlessManager);
}

void Game::initEngines() {
    m_renderEngine = std::make_unique<engines::RenderEngine>(*m_device, *m_renderer, *m_bindlessManager);
    m_renderEngine->initPipelines(m_setLayout->getDescriptorSetLayout());

    m_scriptEngine = std::make_unique<engines::ScriptEngine>();
}

void Game::loadTextures() {
    std::vector<std::string> studsFaces = {
        "assets/textures/smooth.png", "assets/textures/smooth.png", "assets/textures/studs.png",
        "assets/textures/inlets.png", "assets/textures/smooth.png", "assets/textures/smooth.png"};
    m_assetManager->loadCubeFaces("studs", studsFaces);

    std::vector<std::string> smoothFaces = {
        "assets/textures/smooth.png", "assets/textures/smooth.png", "assets/textures/smooth.png",
        "assets/textures/smooth.png", "assets/textures/smooth.png", "assets/textures/smooth.png"};
    m_assetManager->loadCubeFaces("smooth", smoothFaces);

    std::vector<std::string> headFaces = {
        "assets/textures/smooth.png", "assets/textures/smooth.png", "assets/textures/smooth.png",
        "assets/textures/smooth.png", "assets/textures/face.png",   "assets/textures/smooth.png"};
    m_assetManager->loadCubeFaces("headFace", headFaces, "repeat", true, false);

    std::array<std::string, 6> skyboxFaces
        = {"assets/textures/skybox/rt.png", "assets/textures/skybox/lf.png",
           "assets/textures/skybox/up.png", "assets/textures/skybox/dn.png",
           "assets/textures/skybox/ft.png", "assets/textures/skybox/bk.png"};
    m_assetManager->loadCubemap("skybox", skyboxFaces, "skybox");
}

void Game::loadModels() {
    m_modelManager = std::make_unique<gfx::mngrs::ModelManager>(*m_device);

    m_skybox = std::make_unique<gfx::Skybox>(*m_device);

    m_modelManager->loadModel("cube", "assets/models/cube.obj");
    m_modelManager->loadModel("sphere", "assets/models/sphere.obj");

    m_modelManager->loadModel("head", "assets/models/head.obj");
}

void Game::loadUis() {
    m_uiManager->loadText("nunito", m_assetManager->getSampler("ui"), "assets/fonts/Nunito.ttf", 16);

    m_billboardManager->loadText("nunito", m_assetManager->getSampler("repeat"), "assets/fonts/Nunito.ttf", 32);
}

void Game::buildMap(const std::string& rbxlPath) {
    m_instanceManager = std::make_unique<mngrs::InstanceManager>();

    m_workspace = std::make_shared<types::Workspace>();
    m_workspace->onChildrenChanged(
        [this](std::shared_ptr<types::Instance> newChild) { m_instanceManager->markMapDirty(); });

    m_coreGui = std::make_shared<types::CoreGui>();
    m_coreGui->onChildrenChanged(
        [this](std::shared_ptr<types::Instance> newChild) { m_instanceManager->markGuiDirty(); });

    m_physics = std::make_unique<physics::Physics>(m_workspace->getGravity());

    auto parts = utils::rbxl::parseRbxl(rbxlPath);

    for (auto& part : parts) {
        m_physics->createRigidBodyPart(part.get());
        part->setParent(m_workspace.get());
    }

    m_rig = std::make_shared<prefabs::Rig>(*m_physics);
    m_rig->setParent(m_workspace.get());

    m_rig1 = std::make_shared<prefabs::Rig>(*m_physics, "Igor");
    m_rig1->setParent(m_workspace.get());

    m_fps = std::make_shared<types::Text>();
    m_fps->setParent(m_coreGui.get());
    m_fps->setPosition(glm::vec2(50.0f, 30.0f));
}

void Game::run() {
    while (m_window->isOpen()) {
        m_window->update();
        m_scriptEngine->update();

        float dt = m_window->getDeltaTime();

        if (m_window->isKeyPressed(GLFW_KEY_W)) {
            m_rig->move(prefabs::Rig::MoveDirection::Forward, m_camera->getPhi());
        }
        if (m_window->isKeyPressed(GLFW_KEY_S)) {
            m_rig->move(prefabs::Rig::MoveDirection::Backward, m_camera->getPhi());
        }
        if (m_window->isKeyPressed(GLFW_KEY_A)) {
            m_rig->move(prefabs::Rig::MoveDirection::Left, m_camera->getPhi());
        }
        if (m_window->isKeyPressed(GLFW_KEY_D)) {
            m_rig->move(prefabs::Rig::MoveDirection::Right, m_camera->getPhi());
        }
        if (m_window->isKeyPressed(GLFW_KEY_SPACE)) {
            m_rig->jump();
        }

        m_physics->step(dt);

        m_rig1->jump();
        m_rig->update(dt);
        m_rig1->update(dt);
        auto head = m_rig->getHead();
        if (head) {
            m_camera->target = head->getPosition();
        }

        glm::vec2 mouseDelta = m_window->isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)
                                   ? m_window->getMouseDelta()
                                   : glm::vec2(0.0f);
        m_camera->update(
            glm::radians(85.0f), m_window->getAspect(), 0.1f, 1000.0f, mouseDelta, m_window->getScrollDelta());

        if (VkCommandBuffer cmd = m_renderer->beginFrame()) {
            glm::mat4 projection = m_camera->getProjection();
            glm::mat4 view = m_camera->getView();

            m_renderer->beginRenderPass(cmd);

            if (m_instanceManager->isMapDirty()) {
                uint32_t studsId = m_assetManager->getTextureId("studs");
                uint32_t smoothId = m_assetManager->getTextureId("smooth");
                uint32_t headFaceId = m_assetManager->getTextureId("headFace");

                m_instanceManager->rebuildMap(m_workspace, studsId, smoothId, headFaceId);
            } else if (m_instanceManager->isGuiDirty()) {
                m_instanceManager->rebuildGui(m_coreGui);
            }

            m_instanceManager->updateDynamicTransforms();

            m_instanceManager->sortTransparentInstances(m_camera->target);

            const auto& modelInstancesData = m_instanceManager->getModelInstancesData();

            m_renderEngine->renderModelsOpaque(cmd, *m_camera, *m_modelManager, modelInstancesData);

            m_renderEngine->renderSkybox(cmd, *m_camera, *m_skybox, m_assetManager->getCubemapId("skybox"));

            m_renderEngine->renderModelsTransparent(cmd, *m_camera, *m_modelManager, modelInstancesData);

            const auto& billbTextInstancesContent = m_instanceManager->getBillbTextInstancesContent();

            m_renderEngine->renderBillboardTexts(cmd, *m_camera, *m_billboardManager, billbTextInstancesContent);

            m_fps->setText(std::format("FPS: {:.0f}", dt > 0.0f ? 1.0f / dt : 0.0f));

            const auto& textInstancesContent = m_instanceManager->getTextInstancesContent();

            m_renderEngine->renderTexts(cmd, *m_window, *m_uiManager, textInstancesContent);

            m_renderer->endRenderPass(cmd);
            m_renderer->endFrame();
        }
    }

    m_device->waitIdle();
}

} // namespace game