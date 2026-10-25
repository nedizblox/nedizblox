#pragma once

#include "vk/buffer.hpp"
#include "vk/device.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <span>
#include <string>
#include <vector>

namespace gfx {

class ModelShadow {
public:
    static constexpr size_t kMaxInstanceCount = 8000;

    struct Vertex {
        glm::vec3 position{};

        static std::vector<VkVertexInputBindingDescription>
        getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription>
        getAttributeDescriptions();

        bool operator==(const Vertex& other) const {
            return position == other.position;
        }
    };

    struct PushConstantObject {
        alignas(16) glm::mat4 lightSpace{1.0f};
    };

    struct InstanceData {
        glm::mat4 model;
    };

    struct Builder {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};

        void loadModel(const std::string& filepath);
        void loadModel(const std::span<const uint8_t>& geometryRaw);
    };

    ModelShadow(vk::Device& device, const Builder& builder);
    ~ModelShadow();

    ModelShadow(const ModelShadow&) = delete;
    ModelShadow& operator=(const ModelShadow&) = delete;

    static std::unique_ptr<ModelShadow>
    createModelFromFile(vk::Device& device, const std::string& filepath);
    static std::unique_ptr<ModelShadow> createModelFromGeometryRaw(
        vk::Device& device, const std::span<const uint8_t>& geometryRaw
    );

    void draw(
        VkCommandBuffer commandBuffer,
        const std::vector<InstanceData>& instances
    );

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