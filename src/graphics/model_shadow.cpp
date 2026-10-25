#include "model_shadow.hpp"

#include <cstring>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace gfx {

ModelShadow::ModelShadow(vk::Device& device, const Builder& builder) :
    m_device(device) {
    createVertexBuffer(builder.vertices);
    createIndexBuffer(builder.indices);
    createInstanceBuffer();
}

ModelShadow::~ModelShadow() {
} // buffers will be automatically destroyed

void ModelShadow::createVertexBuffer(const std::vector<Vertex>& vertices) {
    m_vertexCount = static_cast<uint32_t>(vertices.size());

    VkDeviceSize bufferSize = sizeof(vertices[0]) * m_vertexCount;

    vk::Buffer stagingBuffer(
        m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT
    );
    stagingBuffer.uploadData(vertices.data(), bufferSize);

    m_vertexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO
    );

    m_device.copyBuffer(
        stagingBuffer.getBuffer(), m_vertexBuffer->getBuffer(), bufferSize
    );
}

void ModelShadow::createIndexBuffer(const std::vector<uint32_t>& indices) {
    m_indexCount = static_cast<uint32_t>(indices.size());
    m_hasIndexBuffer = m_indexCount > 0;
    if (!m_hasIndexBuffer) {
        return;
    }

    VkDeviceSize bufferSize = sizeof(indices[0]) * m_indexCount;

    vk::Buffer stagingBuffer(
        m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT
    );
    stagingBuffer.uploadData(indices.data(), bufferSize);

    m_indexBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO
    );

    m_device.copyBuffer(
        stagingBuffer.getBuffer(), m_indexBuffer->getBuffer(), bufferSize
    );
}

void ModelShadow::createInstanceBuffer() {
    VkDeviceSize bufferSize = sizeof(InstanceData) * kMaxInstanceCount;

    m_instanceBuffer = std::make_unique<vk::Buffer>(
        m_device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT
    );

    m_instanceData = m_instanceBuffer->map();
}

std::unique_ptr<ModelShadow> ModelShadow::createModelFromFile(
    vk::Device& device, const std::string& filepath
) {
    Builder builder{};
    builder.loadModel(filepath);

    return std::make_unique<ModelShadow>(device, builder);
}

std::unique_ptr<ModelShadow> ModelShadow::createModelFromGeometryRaw(
    vk::Device& device, const std::span<const uint8_t>& geometryRaw
) {
    Builder builder{};
    builder.loadModel(geometryRaw);

    return std::make_unique<ModelShadow>(device, builder);
}

void ModelShadow::draw(
    VkCommandBuffer commandBuffer, const std::vector<InstanceData>& instances
) {
    VkBuffer buffers[] = {
        m_vertexBuffer->getBuffer(), m_instanceBuffer->getBuffer()
    };
    VkDeviceSize offsets[] = {0, 0};

    uint32_t instanceCount = static_cast<uint32_t>(instances.size());
    if (instanceCount == 0)
        return;

    if (instanceCount > kMaxInstanceCount)
        instanceCount = kMaxInstanceCount;

    std::memcpy(
        m_instanceData, instances.data(), sizeof(InstanceData) * instanceCount
    );

    vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);
    if (m_hasIndexBuffer) {
        vkCmdBindIndexBuffer(
            commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32
        );
        vkCmdDrawIndexed(commandBuffer, m_indexCount, instanceCount, 0, 0, 0);
    } else {
        vkCmdDraw(commandBuffer, m_vertexCount, instanceCount, 0, 0);
    }
}

std::vector<VkVertexInputBindingDescription>
ModelShadow::Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions;

    bindingDescriptions.push_back(
        {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
    );
    bindingDescriptions.push_back(
        {1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE}
    );

    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription>
ModelShadow::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    attributeDescriptions.push_back(
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)}
    );

    for (uint32_t i = 0; i < 4; i++) {
        attributeDescriptions.push_back(
            {1 + i, 1, VK_FORMAT_R32G32B32A32_SFLOAT,
             static_cast<uint32_t>(
                 offsetof(InstanceData, model) + sizeof(glm::vec4) * i
             )}
        );
    }

    return attributeDescriptions;
}

void ModelShadow::Builder::loadModel(const std::string& filepath) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        filepath, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                      aiProcess_GenNormals | aiProcess_FlipUVs
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
        throw std::runtime_error(
            "Assimp: " + std::string(importer.GetErrorString())
        );
    }

    vertices.clear();
    indices.clear();

    for (size_t m = 0; m < scene->mNumMeshes; m++) {
        aiMesh* mesh = scene->mMeshes[m];
        uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());

        for (size_t i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex{};

            vertex.position = {
                mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z
            };

            vertices.push_back(vertex);
        }

        for (size_t i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (size_t j = 0; j < face.mNumIndices; j++) {
                indices.push_back(vertexOffset + face.mIndices[j]);
            }
        }
    }
}

#pragma pack(push, 1)
struct FileHeader {
    char magic[4];
    uint32_t version;
    uint32_t meshCount;
};

struct MeshHeader {
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t vertexStride;
};
#pragma pack(pop)

void ModelShadow::Builder::loadModel(
    const std::span<const uint8_t>& geometryRaw
) {
    if (geometryRaw.size() < sizeof(FileHeader)) {
        throw std::runtime_error(
            "Geometry raw buffer is too small for FileHeader"
        );
    }

    size_t offset = 0;

    FileHeader fh{};
    std::memcpy(&fh, geometryRaw.data() + offset, sizeof(FileHeader));
    offset += sizeof(FileHeader);

    if (std::memcmp(fh.magic, "MSHB", 4) != 0) {
        throw std::runtime_error("Invalid binary mesh magic header");
    }

    vertices.clear();
    indices.clear();

    for (uint32_t m = 0; m < fh.meshCount; m++) {
        if (offset + sizeof(MeshHeader) > geometryRaw.size()) {
            throw std::runtime_error(
                "Unexpected end of binary data while reading MeshHeader"
            );
        }

        MeshHeader mh{};
        std::memcpy(&mh, geometryRaw.data() + offset, sizeof(MeshHeader));
        offset += sizeof(MeshHeader);

        if (mh.vertexStride < sizeof(Vertex)) {
            throw std::runtime_error(
                "Vertex stride in binary data is smaller than shadow vertex"
            );
        }

        uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());

        size_t verticesBytes =
            static_cast<size_t>(mh.vertexCount) * mh.vertexStride;
        size_t indicesBytes =
            static_cast<size_t>(mh.indexCount) * sizeof(uint32_t);

        if (offset + verticesBytes + indicesBytes > geometryRaw.size()) {
            throw std::runtime_error(
                "Unexpected end of binary data while reading mesh buffers"
            );
        }

        size_t prevVertSize = vertices.size();
        vertices.resize(prevVertSize + mh.vertexCount);
        for (uint32_t i = 0; i < mh.vertexCount; i++) {
            std::memcpy(
                &vertices[prevVertSize + i].position,
                geometryRaw.data() + offset +
                    static_cast<size_t>(i) * mh.vertexStride,
                sizeof(glm::vec3)
            );
        }
        offset += verticesBytes;

        size_t prevIndexSize = indices.size();
        indices.resize(prevIndexSize + mh.indexCount);
        std::memcpy(
            indices.data() + prevIndexSize, geometryRaw.data() + offset,
            indicesBytes
        );
        offset += indicesBytes;

        if (vertexOffset > 0) {
            for (size_t i = prevIndexSize; i < indices.size(); i++) {
                indices[i] += vertexOffset;
            }
        }
    }
}

} // namespace gfx