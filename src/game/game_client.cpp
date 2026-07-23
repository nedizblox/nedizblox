#include "game_client.hpp"

#include "utils/rbxl.hpp"

#include <algorithm>
#include <csignal>
#include <format>
#include <unordered_set>

namespace game {

static std::atomic<bool> g_stop{false};

GameClient::GameClient(const std::string& server, uint16_t port, const std::string& nickname) {
    createClient(server, port, nickname);

    initWindow();
    initVulkan();
    initDescriptors();
    initManagers();
    initEngines();
    loadTextures();
    loadModels();
    loadUis();

    createCamera();

    createServices();
    createLocalRig(nickname);

    createDebugUi();

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
}

GameClient::~GameClient() {
    if (m_client) {
        m_client->stop();
    }

    m_networkParts.clear();
}

void GameClient::initWindow() {
    m_window = std::make_unique<win::Window>(1250, 800, "Nedizblox");
    m_window->setIcon("assets/textures/logo.png");
}

void GameClient::initVulkan() {
    m_device = std::make_unique<gfx::vk::Device>(*m_window);

    m_renderer = std::make_unique<gfx::vk::Renderer>(*m_device, *m_window);
}

void GameClient::initDescriptors() {
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

void GameClient::initManagers() {
    m_bindlessManager = std::make_unique<gfx::mngrs::BindlessManager>(*m_device, *m_setLayout, *m_pool);

    m_assetManager = std::make_unique<mngrs::AssetManager>(*m_device, *m_bindlessManager);

    m_instanceManager = std::make_unique<mngrs::InstanceManager>(*m_assetManager);

    m_modelManager = std::make_unique<gfx::mngrs::ModelManager>(*m_device);

    m_billboardManager = std::make_unique<gfx::mngrs::BillboardManager>(*m_device, *m_bindlessManager);

    m_uiManager = std::make_unique<gfx::mngrs::UiManager>(*m_device, *m_bindlessManager);

    // m_audioManager = std::make_unique<audio::AudioManager>();
}

void GameClient::initEngines() {
    m_renderEngine = std::make_unique<engines::RenderEngine>(*m_device, *m_renderer, *m_bindlessManager);
    m_renderEngine->initPipelines(m_setLayout->getDescriptorSetLayout());

    m_scriptEngine = std::make_unique<engines::ScriptEngine>();
}

void GameClient::loadTextures() {
    m_assetManager->loadTexture("studs", "assets/textures/studs.png");
    m_assetManager->loadTexture("inlets", "assets/textures/inlets.png");
    m_assetManager->loadTexture("glue", "assets/textures/glue.png");
    m_assetManager->loadTexture("smooth", "assets/textures/smooth.png");
    m_assetManager->loadTexture("headFace", "assets/textures/face.png", "repeat", true, false);
    m_assetManager->loadTexture("spawnLocation", "assets/textures/spawn_location.png", "repeat", true, false);

    std::array<std::string, 6> skyboxFaces
        = {"assets/textures/skybox/rt.png", "assets/textures/skybox/lf.png",
           "assets/textures/skybox/up.png", "assets/textures/skybox/dn.png",
           "assets/textures/skybox/ft.png", "assets/textures/skybox/bk.png"};
    m_assetManager->loadCubemap("skybox", skyboxFaces, "skybox");
}

void GameClient::loadModels() {
    m_modelManager = std::make_unique<gfx::mngrs::ModelManager>(*m_device);

    m_skybox = std::make_unique<gfx::Skybox>(*m_device);

    m_modelManager->loadModel("cube", "assets/models/cube.obj");
    m_modelManager->loadModel("sphere", "assets/models/sphere.obj");

    m_modelManager->loadModel("head", "assets/models/head.obj");
}

void GameClient::loadUis() {
    m_imgui = std::make_unique<gfx::ui::Imgui>(*m_window, *m_device, m_assetManager->getSampler("ui"), *m_bindlessManager);
    m_imgui->showDemoWindow(true);

    m_uiManager->loadText("nunito", m_assetManager->getSampler("ui"), "assets/fonts/Nunito.ttf", 16);

    m_billboardManager->loadText("nunito", m_assetManager->getSampler("repeat"), "assets/fonts/Nunito.ttf", 32);
}

void GameClient::createCamera() { m_camera = std::make_unique<core::camera::SphericalCamera>(); }

void GameClient::buildMap(const std::string& rbxlPath) {
    auto parts = utils::rbxl::parseRbxl(rbxlPath);

    for (auto& part : parts) {
        m_physics->createRigidBodyPart(part.get());
        part->setParent(m_workspace);
    }
}

void GameClient::createServices() {
    m_workspace = std::make_shared<types::Workspace>();
    m_workspace->onChildrenChanged(
        [this](std::shared_ptr<types::Instance> newChild) { m_instanceManager->markMapDirty(); });

    m_coreGui = std::make_shared<types::CoreGui>();
    m_coreGui->onChildrenChanged(
        [this](std::shared_ptr<types::Instance> newChild) { m_instanceManager->markGuiDirty(); });

    m_physics = std::make_unique<physics::Physics>(m_workspace->getGravity());
}

void GameClient::createLocalRig(const std::string& nickname) {
    m_localRig = prefabs::Rig::create(*m_physics, nickname, m_client->getPlayerId());
    m_localRig->setParent(m_workspace);
}

void GameClient::createDebugUi() {
    m_fps = std::make_shared<types::Text>();
    m_fps->setParent(m_coreGui);
    m_fps->setPosition(glm::vec2(50.0f, 30.0f));
}

void GameClient::createClient(const std::string& server, uint16_t port, const std::string& nickname) {
    m_client = std::make_unique<net::Client>(server, port, nickname);

    m_client->setMapCallback([this](const std::vector<net::packets::MapPartInfo>& parts) {
        for (const auto& info : parts) {
            auto part = std::make_shared<types::Part>();
            part->setNetworkId(info.networkId);
            part->setPosition(info.position);
            part->setOrientation(info.rotation);
            part->setSize(info.size);
            part->setColor(info.color);
            part->setTransparency(info.transparency);
            part->setAnchored(info.anchored);
            part->setShape(static_cast<enums::PartType>(info.shape));

            btRigidBody* rigidBody = m_physics->createRigidBodyPart(part.get());
            m_physics->setBodyNetworkMode(rigidBody, false);

            part->setRigidBody(rigidBody);

            part->setParent(m_workspace);

            m_networkParts[info.networkId] = std::move(part);
        }

        auto a = types::SpawnLocation::create();
        a->setPosition(glm::vec3(0.0f, 5.0f, 0.0f));
        a->setParent(m_workspace);

        m_instanceManager->markMapDirty();
    });

    m_client->setOldPlayersCallback([this](uint32_t playerId, const std::string& nickname, const glm::vec3& position) {
        if (playerId == m_client->getPlayerId())
            return;

        auto rig = prefabs::Rig::create(*m_physics, nickname, playerId);
        rig->setParent(m_workspace);
        rig->setPivotPosition(position);

        m_physics->setBodyNetworkMode(rig->getRigidBody(), false);
        m_networkRigs[playerId] = std::move(rig);
        m_instanceManager->markMapDirty();
    });

    m_client->setPlayerJoinedCallback([this](uint32_t playerId, const std::string& nickname) {
        if (playerId == m_client->getPlayerId())
            return;

        auto rig = prefabs::Rig::create(*m_physics, nickname, playerId);
        rig->setParent(m_workspace);

        m_physics->setBodyNetworkMode(rig->getRigidBody(), false);
        m_networkRigs[playerId] = std::move(rig);

        m_instanceManager->markMapDirty();
    });

    m_client->setPlayerLeftCallback([this](uint32_t playerId) {
        auto it = m_networkRigs.find(playerId);

        if (it != m_networkRigs.end() && it->second) {
            auto& rig = it->second;
            rig->destroy();
            m_networkRigs.erase(playerId);

            m_instanceManager->markMapDirty();
        }
    });

    m_client->setPhysicsUpdateCallback(
        [this](uint32_t senderPlayerId, const std::vector<net::packets::PhysicalObjectState>& objects) {
            for (const auto& object : objects) {
                btRigidBody* body = nullptr;

                if (auto rigIt = m_networkRigs.find(object.networkId);
                    rigIt != m_networkRigs.end() && rigIt->second) {
                    body = rigIt->second->getRigidBody();
                } else if (
                    auto partIt = m_networkParts.find(object.networkId);
                    partIt != m_networkParts.end() && partIt->second) {
                    body = partIt->second->getRigidBody();
                }

                if (body) {
                    bool isKinematic
                        = (body->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT) != 0;
                    if (isKinematic) {
                        m_physics->applyNetworkState(object);
                    }
                }
            }
        });
}

void GameClient::sendLocalPhysicsState() {
    btRigidBody* playerBody = m_localRig->getRigidBody();
    if (!playerBody)
        return;

    std::unordered_set<uint32_t> candidates;

    struct SearchNode {
        uint32_t netId;
        int depth;
    };
    std::vector<SearchNode> frontier;

    std::vector<uint32_t> touched = m_physics->getBodyCollisions(playerBody);
    for (uint32_t netId : touched) {
        frontier.push_back({netId, 0});
    }

    constexpr int kMaxCollisionDepth = 3;
    std::unordered_set<uint32_t> visited;

    while (!frontier.empty()) {
        SearchNode current = frontier.back();
        frontier.pop_back();

        if (!visited.insert(current.netId).second)
            continue;

        auto it = m_networkParts.find(current.netId);
        if (it == m_networkParts.end() || !it->second || it->second->getAnchored())
            continue;

        candidates.insert(current.netId);

        if (current.depth < kMaxCollisionDepth) {
            btRigidBody* b = it->second->getRigidBody();
            if (b) {
                std::vector<uint32_t> more = m_physics->getBodyCollisions(b);
                for (uint32_t nextId : more) {
                    if (visited.find(nextId) == visited.end()) {
                        frontier.push_back({nextId, current.depth + 1});
                    }
                }
            }
        }
    }

    for (uint32_t netId : candidates) {
        auto it = m_networkParts.find(netId);
        if (it == m_networkParts.end() || !it->second)
            continue;

        btRigidBody* b = it->second->getRigidBody();
        if (!b)
            continue;

        auto orderIt = std::find(m_ownershipOrder.begin(), m_ownershipOrder.end(), netId);

        if (orderIt == m_ownershipOrder.end()) {
            if (m_ownershipOrder.size() >= kMaxLocallyOwnedObjects) {
                uint32_t oldestId = m_ownershipOrder.front();
                m_ownershipOrder.pop_front();

                auto oldIt = m_networkParts.find(oldestId);
                if (oldIt != m_networkParts.end() && oldIt->second) {
                    btRigidBody* oldBody = oldIt->second->getRigidBody();
                    if (oldBody) {
                        m_physics->setBodyNetworkMode(oldBody, false);
                    }
                }
            }

            m_physics->setBodyNetworkMode(b, true);
            m_ownershipOrder.push_back(netId);
        } else {
            m_ownershipOrder.erase(orderIt);
            m_ownershipOrder.push_back(netId);
        }
    }

    constexpr float kReleaseDistanceSq = 15.0f * 15.0f;
    std::vector<net::packets::PhysicalObjectState> outgoingStates;

    std::vector<uint32_t> activeOwnerships(m_ownershipOrder.begin(), m_ownershipOrder.end());

    for (uint32_t netId : activeOwnerships) {
        auto it = m_networkParts.find(netId);
        if (it == m_networkParts.end() || !it->second)
            continue;

        btRigidBody* body = it->second->getRigidBody();
        if (!body)
            continue;

        bool isFarAway = false;
        btVector3 btPos = body->getWorldTransform().getOrigin();
        float distSq
            = glm::distance2(m_localRig->getPivotPosition(), glm::vec3(btPos.x(), btPos.y(), btPos.z()));

        if (distSq > kReleaseDistanceSq) {
            isFarAway = true;
        }

        bool isContactLost = (candidates.find(netId) == candidates.end());
        bool shouldRelease = isFarAway || (!body->isActive() && isContactLost);

        if (shouldRelease) {
            outgoingStates.push_back(m_physics->captureNetworkState(netId, body));
            m_physics->setBodyNetworkMode(body, false);

            auto orderIt = std::find(m_ownershipOrder.begin(), m_ownershipOrder.end(), netId);
            if (orderIt != m_ownershipOrder.end()) {
                m_ownershipOrder.erase(orderIt);
            }
        } else {
            outgoingStates.push_back(m_physics->captureNetworkState(netId, body));
        }
    }

    outgoingStates.push_back(m_physics->captureNetworkState(m_client->getPlayerId(), playerBody));

    if (!outgoingStates.empty()) {
        m_client->sendPhysicsState(outgoingStates);
    }
}

void GameClient::run() {
    m_client->start();

    while (m_window->isOpen() && !g_stop) {
        m_window->update();
        m_imgui->update();
        // m_audioManager->update();

        float dt = m_window->getDeltaTime();

        if (!m_imgui->isKeyboardFocused()) {
            if (m_window->isKeyPressed(GLFW_KEY_W))
                m_localRig->move(enums::RigMoveDirection::Forward, m_camera->getPhi());
            if (m_window->isKeyPressed(GLFW_KEY_S))
                m_localRig->move(enums::RigMoveDirection::Backward, m_camera->getPhi());
            if (m_window->isKeyPressed(GLFW_KEY_A))
                m_localRig->move(enums::RigMoveDirection::Left, m_camera->getPhi());
            if (m_window->isKeyPressed(GLFW_KEY_D))
                m_localRig->move(enums::RigMoveDirection::Right, m_camera->getPhi());
            if (m_window->isKeyPressed(GLFW_KEY_SPACE))
                m_localRig->jump();
        }

        sendLocalPhysicsState();

        m_physics->step(dt);
        m_physics->stepNetworkInterpolation(dt);

        m_localRig->update(dt);
        auto head = m_localRig->getHead();
        if (head) {
            m_camera->setTarget(head->getPosition());
        }

        for (auto& [name, rig] : m_networkRigs) {
            if (rig)
                rig->update(dt);
        }

        // m_audioManager->moveListener(*m_camera);

        glm::vec2 mouseDelta{};
        glm::vec2 scrollDelta{};
        if (!m_imgui->isMouseFocused()) {
            mouseDelta = m_window->isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT) ? m_window->getMouseDelta()
                                                                                 : glm::vec2(0.0f);
            scrollDelta = m_window->getScrollDelta();
        }

        m_camera->update(m_window->getAspect(), mouseDelta, scrollDelta);

        if (VkCommandBuffer cmd = m_renderer->beginFrame()) {
            glm::mat4 projection = m_camera->getProjection();
            glm::mat4 view = m_camera->getView();

            m_renderer->beginRenderPass(cmd);

            if (m_instanceManager->isMapDirty()) {
                uint32_t studsId = m_assetManager->getTextureId("studs");
                uint32_t inletsId = m_assetManager->getTextureId("inlets");
                uint32_t glueId = m_assetManager->getTextureId("glue");
                uint32_t smoothId = m_assetManager->getTextureId("smooth");
                uint32_t headFaceId = m_assetManager->getTextureId("headFace");

                m_instanceManager->rebuildMap(m_workspace, studsId, inletsId, smoothId, glueId);
            } else if (m_instanceManager->isGuiDirty()) {
                m_instanceManager->rebuildGui(m_coreGui);
            }

            m_instanceManager->updateDynamicTransforms();

            m_instanceManager->sortTransparentInstances(m_camera->getPosition());

            const auto& modelInstancesData = m_instanceManager->getModelInstancesData();

            m_renderEngine->renderModelsOpaque(cmd, *m_camera, *m_modelManager, modelInstancesData);

            m_renderEngine->renderSkybox(cmd, *m_camera, *m_skybox, m_assetManager->getCubemapId("skybox"));

            m_renderEngine->renderModelsTransparent(cmd, *m_camera, *m_modelManager, modelInstancesData);

            const auto& billbTextInstancesContent = m_instanceManager->getBillbTextInstancesContent();

            m_renderEngine->renderBillboardTexts(cmd, *m_camera, *m_billboardManager, billbTextInstancesContent);

            m_fps->setText(std::format("FPS: {:.0f}", dt > 0.0f ? 1.0f / dt : 0.0f));

            const auto& textInstancesContent = m_instanceManager->getTextInstancesContent();

            m_renderEngine->renderTexts(cmd, *m_window, *m_uiManager, textInstancesContent);

            m_renderEngine->renderImgui(cmd, *m_window, *m_imgui);

            m_renderer->endRenderPass(cmd);
            m_renderer->endFrame();
        }
    }

    m_device->waitIdle();
}

void GameClient::signalHandler(int sig) {
    core::logger::info(std::format("Received signal {}, stopping", sig));
    g_stop = true;
}

} // namespace game