#include "model.hpp"
#include "core/hash.hpp"

#include <tinyobj/tiny_obj_loader.h>

#include <glm/gtx/hash.hpp>

#include <cstring>
#include <unordered_map>

namespace std {

template <>
struct hash<gfx::Model::Vertex> {
    size_t operator()(const gfx::Model::Vertex& vertex) const {
        size_t seed = 0;
        core::hash::combine(seed, vertex.position, vertex.normal, vertex.uv);
        return seed;
    }
};

} // namespace std

namespace gfx {

Model::Model(vk::Device& device, const Builder& builder) : m_device(device) {
    createVertexBuffer(builder.vertices);
    createIndexBuffer(builder.indices);
    createInstanceBuffer();
}

Model::~Model() {} // buffers will be automatically destroyed

void Model::createVertexBuffer(const std::vector<Vertex>& vertices) {
    m_vertexCount = static_cast<uint32_t>(vertices.size());

    VkDeviceSize bufferSize = sizeof(vertices[0]) * m_vertexCount;

    vk::Buffer stagingBuffer(
        m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    stagingBuffer.uploadData(vertices.data(), bufferSize);

    m_vertexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO);

    m_device.copyBuffer(stagingBuffer.getBuffer(), m_vertexBuffer->getBuffer(), bufferSize);
}

void Model::createIndexBuffer(const std::vector<uint32_t>& indices) {
    m_indexCount = static_cast<uint32_t>(indices.size());
    m_hasIndexBuffer = m_indexCount > 0;
    if (!m_hasIndexBuffer) {
        return;
    }

    VkDeviceSize bufferSize = sizeof(indices[0]) * m_indexCount;

    vk::Buffer stagingBuffer(
        m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    stagingBuffer.uploadData(indices.data(), bufferSize);

    m_indexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO);

    m_device.copyBuffer(stagingBuffer.getBuffer(), m_indexBuffer->getBuffer(), bufferSize);
}

void Model::createInstanceBuffer() {
    VkDeviceSize bufferSize = sizeof(glm::mat4) * 10000;

    m_instanceBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    m_instanceData = m_instanceBuffer->map();
}

std::unique_ptr<Model> Model::createModelFromFile(vk::Device& device, const std::string& filepath) {
    Builder builder{};
    builder.loadModel(filepath);

    return std::make_unique<Model>(device, builder);
}

void Model::draw(VkCommandBuffer commandBuffer, const std::vector<InstanceData>& instances) {
    VkBuffer buffers[] = {m_vertexBuffer->getBuffer(), m_instanceBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0, 0};

    uint32_t instanceCount = static_cast<uint32_t>(instances.size());
    if (instanceCount == 0)
        return;

    memcpy(m_instanceData, instances.data(), sizeof(InstanceData) * instanceCount);

    vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);
    if (m_hasIndexBuffer) {
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, m_indexCount, instanceCount, 0, 0, 0);
    } else {
        vkCmdDraw(commandBuffer, m_vertexCount, instanceCount, 0, 0);
    }
}

std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions;

    bindingDescriptions.push_back({0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX});
    bindingDescriptions.push_back({1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE});

    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
    attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
    attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});

    for (uint32_t i = 0; i < 4; i++) {
        attributeDescriptions.push_back(
            {3 + i, 1, VK_FORMAT_R32G32B32A32_SFLOAT,
             static_cast<uint32_t>(offsetof(InstanceData, model) + sizeof(glm::vec4) * i)});
    }

    attributeDescriptions.push_back({7, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, color)});

    attributeDescriptions.push_back({8, 1, VK_FORMAT_R32_UINT, offsetof(InstanceData, texIndex)});
    attributeDescriptions.push_back({9, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(InstanceData, texTile)});

    return attributeDescriptions;
}

void Model::Builder::loadModel(const std::string& filepath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
        throw std::runtime_error("OBJ: " + err);
    }

    vertices.clear();
    indices.clear();

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            if (index.vertex_index >= 0) {
                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2],
                };
            }

            if (index.normal_index >= 0) {
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2],
                };
            }

            if (index.texcoord_index >= 0) {
                vertex.uv = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1],
                };
            }

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }
}

} // namespace gfx