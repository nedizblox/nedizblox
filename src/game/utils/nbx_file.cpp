#include "nbx_file.hpp"

#include "../types/factory.hpp"

#include "network/serializer.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace game::utils {

namespace {

using Factory = std::function<std::shared_ptr<types::Instance>()>;

std::unordered_map<enums::InstanceType, Factory>& factories() {
    static std::unordered_map<enums::InstanceType, Factory> map;
    return map;
}

std::shared_ptr<types::Instance>
createPart(net::serializer::BinaryReader record) {
    auto probe = types::Part::create("Part");
    auto probeReader = record;
    probe->deserialize(probeReader);

    if (probe->getPartType() == enums::PartType::SpawnLocation) {
        auto spawn = std::make_shared<types::SpawnLocation>();
        spawn->deserialize(record);
        return spawn;
    }
    return probe;
}

void flatten(
    const std::shared_ptr<types::Instance>& node,
    std::vector<std::shared_ptr<types::Instance>>& out
) {
    if (!node)
        return;
    out.push_back(node);
    for (const auto& child : node->getChildren())
        flatten(child, out);
}

} // namespace

void nbxf::registerInstanceType(
    enums::InstanceType type,
    std::function<std::shared_ptr<types::Instance>()> factory
) {
    factories()[type] = std::move(factory);
}

std::vector<uint8_t> nbxf::serializeNbxlBuffer(
    const std::vector<std::shared_ptr<types::Instance>>& roots
) {
    std::vector<std::shared_ptr<types::Instance>> all;
    for (const auto& root : roots)
        flatten(root, all);

    uint32_t maxId = 0;
    for (const auto& inst : all)
        maxId = glm::max(maxId, inst->getId());
    std::unordered_set<uint32_t> seen;
    for (const auto& inst : all) {
        if (inst->getId() == 0 || !seen.insert(inst->getId()).second) {
            inst->setId(++maxId);
            seen.insert(maxId);
        }
    }

    net::serializer::BinaryWriter writer;
    writer.write<uint32_t>(kMagic);
    writer.write<uint16_t>(kVersion);
    writer.write<uint32_t>(static_cast<uint32_t>(all.size()));

    for (const auto& inst : all) {
        net::serializer::BinaryWriter record;
        inst->serialize(record);

        writer.write<uint32_t>(static_cast<uint32_t>(record.buffer.size()));
        writer.buffer.insert(
            writer.buffer.end(), record.buffer.begin(), record.buffer.end()
        );
    }

    return std::vector<uint8_t>(writer.buffer.begin(), writer.buffer.end());
}

void nbxf::saveNbxl(
    const std::string& filePath,
    const std::vector<std::shared_ptr<types::Instance>>& roots
) {
    const std::vector<uint8_t> buffer = serializeNbxlBuffer(roots);

    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error(
            "NBXF: Failed to open file for writing " + filePath
        );
    }
    file.write(
        reinterpret_cast<const char*>(buffer.data()),
        static_cast<std::streamsize>(buffer.size())
    );
    if (!file) {
        throw std::runtime_error("NBXF: Failed to write file " + filePath);
    }
}

std::vector<std::shared_ptr<types::Instance>>
nbxf::parseNbxl(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("NBXF: Failed to open file " + filePath);
    }
    std::vector<uint8_t> bytes(
        (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()
    );

    return parseNbxlBuffer(bytes.data(), bytes.size(), filePath);
}

std::vector<std::shared_ptr<types::Instance>> nbxf::parseNbxlBuffer(
    const uint8_t* data, size_t size, const std::string& filePath
) {
    net::serializer::BinaryReader reader(const_cast<uint8_t*>(data), size);

    if (reader.read<uint32_t>() != kMagic) {
        throw std::runtime_error("NBXF: Invalid magic number in " + filePath);
    }
    auto version = reader.read<uint16_t>();
    if (version != kVersion) {
        throw std::runtime_error(
            "NBXF: Unsupported version " + std::to_string(version)
        );
    }
    auto count = reader.read<uint32_t>();

    std::unordered_map<uint32_t, std::shared_ptr<types::Instance>> instanceMap;
    std::vector<std::shared_ptr<types::Instance>> allInstances;
    allInstances.reserve(count);

    for (uint32_t i = 0; i < count; i++) {
        auto size = reader.read<uint32_t>();
        if (reader.offset + size > reader.size) {
            throw std::runtime_error("NBXF: Record exceeds file size");
        }

        net::serializer::BinaryReader record(reader.data + reader.offset, size);
        reader.offset += size;

        auto type = record.read<enums::InstanceType>();
        std::shared_ptr<types::Instance> instance;
        if (type == enums::InstanceType::Part) {
            instance = createPart(record);
        } else {
            auto it = factories().find(type);
            instance = (it != factories().end()) ? it->second()
                                                 : types::factory::create(type);
            if (!instance)
                continue;
            instance->deserialize(record);
        }

        instanceMap[instance->getId()] = instance;
        allInstances.push_back(instance);
    }

    for (const auto& instance : allInstances) {
        uint32_t parentId = instance->getParentId();
        if (parentId == 0)
            continue;

        auto parentIt = instanceMap.find(parentId);
        if (parentIt != instanceMap.end()) {
            instance->setParent(parentIt->second);
        }
    }

    return allInstances;
}

std::vector<uint8_t>
nbxf::serializeTree(const std::shared_ptr<types::Instance>& root) {
    if (!root)
        return {};
    return serializeNbxlBuffer({root});
}

std::shared_ptr<types::Instance>
nbxf::instantiateTree(const std::vector<uint8_t>& bytes) {
    if (bytes.empty())
        return nullptr;

    auto all = parseNbxlBuffer(bytes.data(), bytes.size(), "<clipboard>");

    std::shared_ptr<types::Instance> root;
    for (const auto& instance : all) {
        if (!instance->getParent()) {
            root = instance;
            break;
        }
    }

    for (const auto& instance : all)
        instance->setId(0);

    return root;
}

} // namespace game::utils