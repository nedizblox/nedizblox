#include "game.hpp"

#include "utils/rbxl.hpp"

#include <format>
#include <stdexcept>

namespace game {

Game::Game() {
    try {
        initWindow();
        initVulkan();
        initDescriptors();
        initPipelines();
        loadSamplers();
        loadTextures();
        loadModels();
        loadTexts();

        buildMap();
    } catch (std::exception& e) {
        core::logger::err(e.what());
        throw;
    }
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

    m_bindlessManager = std::make_unique<gfx::BindlessManager>(*m_device, *m_setLayout, *m_pool);
}

void Game::initPipelines() {
    m_pipelines["skybox"] = gfx::vk::Pipeline::Builder(*m_device)
                                .setVertShaderPath("shaders/skybox.vert.spv")
                                .setFragShaderPath("shaders/skybox.frag.spv")
                                .setConstantSize(sizeof(gfx::Skybox::PushConstantObject))
                                .setDescriptorLayouts({m_setLayout->getDescriptorSetLayout()})
                                .setRenderPass(m_renderer->getRenderPass())
                                .setBindingDescriptions(gfx::Skybox::Vertex::getBindingDescriptions())
                                .setAttributeDescriptions(gfx::Skybox::Vertex::getAttributeDescriptions())
                                .enableDepthTest()
                                .disableDepthWrite()
                                .setDepthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
                                .build();

    m_pipelines["modelOpaque"]
        = gfx::vk::Pipeline::Builder(*m_device)
              .setVertShaderPath("shaders/model.vert.spv")
              .setFragShaderPath("shaders/model.frag.spv")
              .setConstantSize(sizeof(gfx::Model::PushConstantObject))
              .setDescriptorLayouts({m_setLayout->getDescriptorSetLayout()})
              .setRenderPass(m_renderer->getRenderPass())
              .setBindingDescriptions(gfx::Model::Vertex::getBindingDescriptions())
              .setAttributeDescriptions(gfx::Model::Vertex::getAttributeDescriptions())
              .setCullMode(VK_CULL_MODE_FRONT_BIT)
              .enableDepthTest()
              .build();

    m_pipelines["modelTransparent"]
        = gfx::vk::Pipeline::Builder(*m_device)
              .setVertShaderPath("shaders/model.vert.spv")
              .setFragShaderPath("shaders/model.frag.spv")
              .setConstantSize(sizeof(gfx::Model::PushConstantObject))
              .setDescriptorLayouts({m_setLayout->getDescriptorSetLayout()})
              .setRenderPass(m_renderer->getRenderPass())
              .setBindingDescriptions(gfx::Model::Vertex::getBindingDescriptions())
              .setAttributeDescriptions(gfx::Model::Vertex::getAttributeDescriptions())
              .setCullMode(VK_CULL_MODE_FRONT_BIT)
              .enableDepthTest()
              .disableDepthWrite()
              .enableAlphaBlending()
              .build();

    m_pipelines["text"] = gfx::vk::Pipeline::Builder(*m_device)
                              .setVertShaderPath("shaders/text.vert.spv")
                              .setFragShaderPath("shaders/text.frag.spv")
                              .setConstantSize(sizeof(gfx::ui::Text::PushConstantObject))
                              .setDescriptorLayouts({m_setLayout->getDescriptorSetLayout()})
                              .setRenderPass(m_renderer->getRenderPass())
                              .setBindingDescriptions(gfx::ui::Text::Vertex::getBindingDescriptions())
                              .setAttributeDescriptions(gfx::ui::Text::Vertex::getAttributeDescriptions())
                              .enableAlphaBlending()
                              .build();
}

void Game::loadSamplers() {
    m_samplers["default"] = gfx::vk::Sampler::Builder(*m_device)
                                .setAnisotropy(m_device->properties.limits.maxSamplerAnisotropy)
                                .setMipmaps(VK_SAMPLER_MIPMAP_MODE_LINEAR)
                                .setMaxLod(7.0f)
                                .build();

    m_samplers["skybox"] = gfx::vk::Sampler::Builder(*m_device)
                               .setAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
                               .setAnisotropy(m_device->properties.limits.maxSamplerAnisotropy)
                               .setMipmaps(VK_SAMPLER_MIPMAP_MODE_LINEAR)
                               .build();

    m_samplers["text"] = gfx::vk::Sampler::Builder(*m_device)
                             .setAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
                             .setMaxLod(1.0f)
                             .build();
}

void Game::loadTextures() {
    std::vector<std::string> studsFaces = {
        "assets/textures/smooth.png", "assets/textures/smooth.png", "assets/textures/studs.png",
        "assets/textures/inlets.png", "assets/textures/smooth.png", "assets/textures/smooth.png"};

    for (size_t i = 0; i < studsFaces.size(); i++) {
        auto tex = std::make_unique<gfx::Texture>(*m_device, *m_samplers["default"], studsFaces[i]);
        uint32_t idx = m_bindlessManager->addTexture(std::move(tex));

        if (i == 0) {
            m_textures["studs"] = idx;
        }
    }

    std::vector<std::string> smoothFaces = {
        "assets/textures/smooth.png", "assets/textures/smooth.png", "assets/textures/smooth.png",
        "assets/textures/smooth.png", "assets/textures/smooth.png", "assets/textures/smooth.png"};

    for (size_t i = 0; i < smoothFaces.size(); i++) {
        auto tex = std::make_unique<gfx::Texture>(*m_device, *m_samplers["default"], smoothFaces[i]);
        uint32_t idx = m_bindlessManager->addTexture(std::move(tex));

        if (i == 0) {
            m_textures["smooth"] = idx;
        }
    }

    auto cub = std::make_unique<gfx::Cubemap>(
        *m_device, *m_samplers["skybox"],
        std::array<std::string, 6>(
            {"assets/textures/skybox/rt.png", "assets/textures/skybox/lf.png", "assets/textures/skybox/up.png",
             "assets/textures/skybox/dn.png", "assets/textures/skybox/ft.png", "assets/textures/skybox/bk.png"}));

    m_cubemaps["skybox"] = m_bindlessManager->addCubemap(std::move(cub));
}

void Game::loadModels() {
    m_skybox = std::make_unique<gfx::Skybox>(*m_device);

    m_models["cubeOpaque"] = gfx::Model::createModelFromFile(*m_device, "assets/models/cube.obj");
    m_models["cubeTransparent"] = gfx::Model::createModelFromFile(*m_device, "assets/models/cube.obj");

    m_models["sphereOpaque"] = gfx::Model::createModelFromFile(*m_device, "assets/models/sphere.obj");
    m_models["sphereTransparent"] = gfx::Model::createModelFromFile(*m_device, "assets/models/sphere.obj");
}

void Game::loadTexts() {
    m_texts["fps"] = std::make_unique<gfx::ui::Text>(
        *m_device, *m_samplers["text"], *m_bindlessManager, "assets/fonts/GraphikLCWeb.ttf", 15);
}

void Game::collectInstances(const std::shared_ptr<types::Instance>& parent) {
    for (auto& obj : parent->getChildren()) {
        if (obj->getType() == enums::InstanceType::Part) {
            auto part = std::static_pointer_cast<types::Part>(obj);

            float transparency = part->getTransparency();
            enums::PartType shape = part->getShape();

            uint32_t studs = m_textures["studs"];
            uint32_t smooth = m_textures["smooth"];

            if (transparency <= 0.0f) {
                if (shape == enums::PartType::Block) {
                    m_instancesData["cubeOpaque"].push_back(
                        {part->getModelMatrix(), glm::vec4(glm::vec3(part->getColor()) / 255.0f, 1.0f), studs});
                } else if (shape == enums::PartType::Ball) {
                    m_instancesData["sphereOpaque"].push_back(
                        {part->getModelMatrix(), glm::vec4(glm::vec3(part->getColor()) / 255.0f, 1.0f), smooth});
                }
            } else {
                if (shape == enums::PartType::Block) {
                    m_instancesData["cubeTransparent"].push_back(
                        {part->getModelMatrix(),
                         glm::vec4(glm::vec3(part->getColor()) / 255.0f, 1.0f - transparency), studs});
                } else if (shape == enums::PartType::Ball) {
                    m_instancesData["sphereTransparent"].push_back(
                        {part->getModelMatrix(),
                         glm::vec4(glm::vec3(part->getColor()) / 255.0f, 1.0f - transparency), smooth});
                }
            }
        }

        collectInstances(obj);
    }
}

void Game::sortInstances() {
    for (auto& [name, instances] : m_instancesData) {
        std::sort(
            instances.begin(), instances.end(),
            [this](const gfx::Model::InstanceData& a, const gfx::Model::InstanceData& b) {
                glm::vec3 diffA = glm::vec3(a.model[3]) - m_camera.position;
                glm::vec3 diffB = glm::vec3(b.model[3]) - m_camera.position;

                return glm::dot(diffA, diffA) > glm::dot(diffB, diffB);
            });
    }
}

void Game::clearInstances() {
    for (auto& data : m_instancesData) {
        data.second.clear();
    }
}

void Game::buildMap() {
    m_workspace = std::make_shared<types::Instance>(enums::InstanceType::Workspace, "Workspace");

    m_physics = std::make_unique<physics::Physics>(-100.0f);

    m_instancesData["cubeOpaque"].reserve(MAX_INSTANCES);
    m_instancesData["cubeTransparent"].reserve(MAX_INSTANCES);

    m_instancesData["sphereOpaque"].reserve(MAX_INSTANCES);
    m_instancesData["sphereTransparent"].reserve(MAX_INSTANCES);

    auto parts = utils::rbxl::parseRbxl("assets/maps/crossroads.rbxl");

    for (auto& part : parts) {
        m_physics->createRigidBody(part.get());
        part->setParent(m_workspace.get());
    }
}

void Game::run() {
    while (m_window->isOpen()) {
        m_window->update();
        m_scriptManager.update();
        m_physics->step(m_window->getDeltaTime());

        if (m_window->isKeyPressed(GLFW_KEY_W)) {
            m_camera.move(core::FreeCamera::CameraDirection::FORWARD, m_window->getDeltaTime());
        }
        if (m_window->isKeyPressed(GLFW_KEY_S)) {
            m_camera.move(core::FreeCamera::CameraDirection::BACKWARD, m_window->getDeltaTime());
        }
        if (m_window->isKeyPressed(GLFW_KEY_A)) {
            m_camera.move(core::FreeCamera::CameraDirection::LEFT, m_window->getDeltaTime());
        }
        if (m_window->isKeyPressed(GLFW_KEY_D)) {
            m_camera.move(core::FreeCamera::CameraDirection::RIGHT, m_window->getDeltaTime());
        }

        glm::vec2 mouseDelta = m_window->isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)
                                   ? m_window->getMouseRel()
                                   : glm::vec2(0.0f);
        m_camera.update(glm::radians(80.0f), m_window->getAspect(), 0.1f, 1000.0f, mouseDelta);

        if (VkCommandBuffer cmd = m_renderer->beginFrame()) {
            glm::mat4 projection = m_camera.getProjection();
            glm::mat4 view = m_camera.getView();

            m_renderer->beginRenderPass(cmd);

            clearInstances();
            collectInstances(m_workspace);
            sortInstances();

            m_pipelines["modelOpaque"]->bind(cmd);
            m_bindlessManager->bind(cmd, m_pipelines["modelOpaque"]->getPipelineLayout());

            gfx::Model::PushConstantObject modelPush{};
            modelPush.viewProj = projection * view;
            m_pipelines["modelOpaque"]->pushConstant(cmd, modelPush);

            m_models["cubeOpaque"]->draw(cmd, m_instancesData["cubeOpaque"]);
            m_models["sphereOpaque"]->draw(cmd, m_instancesData["sphereOpaque"]);

            m_pipelines["skybox"]->bind(cmd);
            m_bindlessManager->bind(cmd, m_pipelines["skybox"]->getPipelineLayout());

            gfx::Skybox::PushConstantObject skyboxPush{};
            skyboxPush.viewProj = projection * glm::mat4(glm::mat3(view));
            skyboxPush.cubIndex = m_cubemaps["skybox"];
            m_pipelines["skybox"]->pushConstant(cmd, skyboxPush);

            m_skybox->draw(cmd);

            m_pipelines["modelTransparent"]->bind(cmd);
            m_bindlessManager->bind(cmd, m_pipelines["modelTransparent"]->getPipelineLayout());

            m_pipelines["modelTransparent"]->pushConstant(cmd, modelPush);

            m_models["cubeTransparent"]->draw(cmd, m_instancesData["cubeTransparent"]);
            m_models["sphereTransparent"]->draw(cmd, m_instancesData["sphereTransparent"]);

            m_pipelines["text"]->bind(cmd);
            m_bindlessManager->bind(cmd, m_pipelines["text"]->getPipelineLayout());

            gfx::ui::Text::PushConstantObject textPush{};
            textPush.proj = glm::ortho(
                0.0f, static_cast<float>(m_window->getWidth()), 0.0f,
                static_cast<float>(m_window->getHeight()));
            textPush.texIndex = m_texts["fps"]->getTextureIndex();

            float deltaTime = m_window->getDeltaTime();
            float fps = deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f;

            m_texts["fps"]->setText(std::format("FPS: {:.0f}", fps), glm::vec2(20.0f, 40.0f));

            m_pipelines["text"]->pushConstant(cmd, textPush);
            m_texts["fps"]->draw(cmd);

            m_renderer->endRenderPass(cmd);
            m_renderer->endFrame();
        }
    }

    m_device->waitIdle();
}

} // namespace game