#include "model.hpp"

#include <cstring>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

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

    std::memcpy(m_instanceData, instances.data(), sizeof(InstanceData) * instanceCount);

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

    attributeDescriptions.push_back({8, 1, VK_FORMAT_R32G32B32_UINT, offsetof(InstanceData, texIndices1)});
    attributeDescriptions.push_back({9, 1, VK_FORMAT_R32G32B32_UINT, offsetof(InstanceData, texIndices2)});
    attributeDescriptions.push_back({10, 1, VK_FORMAT_R32G32B32_UINT, offsetof(InstanceData, texTilesPacked)});

    return attributeDescriptions;
}

void Model::Builder::loadModel(const std::string& filepath) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        filepath, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenNormals | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("Assimp: " + std::string(importer.GetErrorString()));
    }

    vertices.clear();
    indices.clear();

    for (size_t m = 0; m < scene->mNumMeshes; ++m) {
        aiMesh* mesh = scene->mMeshes[m];
        uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());

        for (size_t i = 0; i < mesh->mNumVertices; ++i) {
            Vertex vertex{};

            vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

            if (mesh->HasNormals()) {
                vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
            }

            if (mesh->HasTextureCoords(0)) {
                vertex.uv = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
            }

            vertices.push_back(vertex);
        }

        for (size_t i = 0; i < mesh->mNumFaces; ++i) {
            aiFace face = mesh->mFaces[i];
            for (size_t j = 0; j < face.mNumIndices; ++j) {
                indices.push_back(vertexOffset + face.mIndices[j]);
            }
        }
    }
}

} // namespace gfx