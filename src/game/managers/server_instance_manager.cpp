#include "server_instance_manager.hpp"

#include <cstring>

namespace game::mngrs {

ServerInstanceManager::ServerInstanceManager(physics::Physics& physics) : m_physics(physics) {}

ServerInstanceManager::~ServerInstanceManager() {
    if (auto root = m_currentRoot.lock()) {
        root->removeDescendantPropertyChangedCallback(m_propSubId);
        root->removeChildrenChangedCallback(m_childrenSubId);
        root->removeDescendantChildrenChangedCallback(m_descendantChildrenSubId);
    }
}

void ServerInstanceManager::setRoot(const std::shared_ptr<types::Instance>& root) {
    if (m_currentRoot.lock() != root) {
        if (auto oldRoot = m_currentRoot.lock()) {
            oldRoot->removeDescendantPropertyChangedCallback(m_propSubId);
            oldRoot->removeChildrenChangedCallback(m_childrenSubId);
            oldRoot->removeDescendantChildrenChangedCallback(m_descendantChildrenSubId);
        }

        m_currentRoot = root;

        if (root) {
            m_propSubId = root->onDescendantPropertyChanged([this](auto) { markDirty(); });
            m_childrenSubId = root->onChildrenChanged([this](auto) { markDirty(); });
            m_descendantChildrenSubId
                = root->onDescendantChildrenChanged([this](auto) { markDirty(); });
        }

        markDirty();
    }
}

void ServerInstanceManager::rebuildMap() {
    auto root = m_currentRoot.lock();
    if (!root) {
        m_mapBuffer.clear();
        m_dirty = false;
        return;
    }

    std::vector<net::packets::PartInfo> partInfos;

    {
        std::lock_guard<std::mutex> lock(m_networkPartsMutex);
        m_networkParts.clear();

        collectInstances(root, partInfos);
    }

    net::packets::InitialMapPacket header{};
    header.partCount = static_cast<uint32_t>(partInfos.size());

    size_t headerSize = sizeof(header);
    size_t dataSize = partInfos.size() * sizeof(net::packets::PartInfo);

    m_mapBuffer.resize(headerSize + dataSize);
    std::memcpy(m_mapBuffer.data(), &header, headerSize);

    if (!partInfos.empty()) {
        std::memcpy(m_mapBuffer.data() + headerSize, partInfos.data(), dataSize);
    }

    m_dirty = false;
}

void ServerInstanceManager::collectInstances(
    const std::shared_ptr<types::Instance>& parent, std::vector<net::packets::PartInfo>& outInfos) {
    if (!parent)
        return;

    for (const auto& child : parent->getChildren()) {
        if (!child)
            continue;

        if (child->getType() == enums::InstanceType::Part) {
            auto part = std::static_pointer_cast<types::Part>(child);

            uint32_t networkId = part->getNetworkId();
            if (networkId == 0) {
                networkId = m_nextNetworkId.fetch_add(1);
                part->setNetworkId(networkId);
            }

            if (!part->getRigidBody()) {
                m_physics.createRigidBodyPart(part.get());
            }

            m_networkParts[networkId] = part;

            net::packets::PartInfo info{};
            info.networkId = networkId;
            info.parentNetworkId = parent ? parent->getNetworkId() : 0;
            std::strncpy(info.name, part->getName().c_str(), sizeof(info.name) - 1);
            info.position = part->getPosition();
            info.rotation = part->getOrientation();
            info.size = part->getSize();
            info.color = part->getColor();
            info.transparency = part->getTransparency();
            info.anchored = part->getAnchored();
            info.shape = static_cast<net::packets::PartShape>(part->getShape());
            info.type = static_cast<net::packets::PartType>(part->getPartType());

            outInfos.push_back(info);
        }

        collectInstances(child, outInfos);
    }
}

} // namespace game::mngrs