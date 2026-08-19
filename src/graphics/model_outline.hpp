#pragma once

#include "vk/buffer.hpp"
#include "vk/device.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>
#include <span>

namespace gfx {

class ModelOutline {
public:
    struct Vertex {
        glm::vec3 position{};
        glm::vec3 normal{};
        glm::vec2 uv{};

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

        bool operator==(const Vertex& other) const {
            return position == other.position && normal == other.normal && uv == other.uv;
        }
    };

    struct PushConstantObject {
        alignas(16) glm::mat4 viewProj{1.0f};
        alignas(16) glm::vec3 cameraPos{0.0f};
    };

    struct InstanceData {
        glm::mat4 model;
        glm::vec4 outlineColor;
    };

    struct Builder {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};

        void loadModel(const std::string& filepath);
        void loadModel(const std::span<const uint8_t>& geometryRaw);
    };

    ModelOutline(vk::Device& device, const Builder& builder);
    ~ModelOutline();

    ModelOutline(const ModelOutline&) = delete;
    ModelOutline& operator=(const ModelOutline&) = delete;

    static std::unique_ptr<ModelOutline> createModelFromFile(vk::Device& device, const std::string& filepath);
    static std::unique_ptr<ModelOutline> createModelFromGeometryRaw(vk::Device& device, const std::span<const uint8_t>& geometryRaw);

    void draw(VkCommandBuffer commandBuffer, const std::vector<InstanceData>& instances);

private:
    vk::Device& m_device;

    std::unique_ptr<vk::Buffer> m_vertexBuffer;
    uint32_t m_vertexCount;

    std::unique_ptr<vk::Buffer> m_indexBuffer;
    uint32_t m_indexCount;
    bool m_hasIndexBuffer = false;

    std::unique_ptr<vk::Buffer> m_instanceBuffer;
    void* m_instanceData;

    void createVertexBuffer(const std::vector<Vertex>& vertices);
    void createIndexBuffer(const std::vector<uint32_t>& indices);
    void createInstanceBuffer();
};

} // namespace gfx