#include "server_replicator.hpp"

namespace game::multiplayer {

namespace {

bool isRig(const std::shared_ptr<types::Instance>& instance) {
    return dynamic_cast<types::Rig*>(instance.get()) != nullptr;
}

} // namespace

ServerReplicator::ServerReplicator(physics::Physics& physics) :
    m_physics(physics) {
}

ServerReplicator::~ServerReplicator() {
    if (auto root = m_currentRoot.lock()) {
        root->removeDescendantPropertyChangedCallback(m_propSubId);
        root->removeChildrenChangedCallback(m_childrenSubId);
        root->removeDescendantChildrenChangedCallback(
            m_descendantChildrenSubId
        );
    }
}

void ServerReplicator::setRoot(const std::shared_ptr<types::Instance>& root) {
    if (m_currentRoot.lock() != root) {
        if (auto oldRoot = m_currentRoot.lock()) {
            oldRoot->removeDescendantPropertyChangedCallback(m_propSubId);
            oldRoot->removeChildrenChangedCallback(m_childrenSubId);
            oldRoot->removeDescendantChildrenChangedCallback(
                m_descendantChildrenSubId
            );
        }

        m_currentRoot = root;

        if (root) {
            m_propSubId = root->onDescendantPropertyChanged([this](auto)
                                                            { markDirty(); });
            m_childrenSubId =
                root->onChildrenChanged([this](auto) { markDirty(); });
            m_descendantChildrenSubId =
                root->onDescendantChildrenChanged([this](auto)
                                                  { markDirty(); });
        }

        markDirty();
    }
}

uint32_t ServerReplicator::countSerializableInstances(
    const std::shared_ptr<types::Instance>& parent
) const {
    if (!parent)
        return 0;

    uint32_t count = 0;
    for (const auto& child : parent->getChildren()) {
        if (!child || isRig(child))
            continue;

        count += 1 + countSerializableInstances(child);
    }
    return count;
}

void ServerReplicator::rebuildMap() {
    auto root = m_currentRoot.lock();
    if (!root) {
        m_mapBuffer.clear();
        m_dirty = false;
        return;
    }

    net::serializer::BinaryWriter writer;

    {
        std::lock_guard<std::mutex> lock(m_networkPartsMutex);
        m_networkParts.clear();

        uint32_t totalObjects = countSerializableInstances(root);
        writer.write<uint32_t>(totalObjects);

        collectInstances(root, writer);
    }

    m_mapBuffer = std::move(writer.buffer);
    m_dirty = false;
}

void ServerReplicator::collectInstances(
    const std::shared_ptr<types::Instance>& parent,
    net::serializer::BinaryWriter& writer
) {
    if (!parent)
        return;

    for (const auto& child : parent->getChildren()) {
        if (!child || isRig(child))
            continue;

        uint32_t networkId = child->getNetworkId();
        if (networkId == 0) {
            networkId = m_nextNetworkId.fetch_add(1);
            child->setNetworkId(networkId);
        }

        if (child->getType() == enums::InstanceType::Part) {
            auto part = std::static_pointer_cast<types::Part>(child);

            if (!part->getRigidBody()) {
                m_physics.createRigidBodyPart(part.get());
            } else {
                m_physics.rebindNetworkId(part.get());
            }
            m_networkParts[networkId] = part;
        }

        child->serialize(writer);

        collectInstances(child, writer);
    }
}

} // namespace game::multiplayer