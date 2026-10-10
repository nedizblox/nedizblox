#pragma once

#include "../physics/physics.hpp"
#include "../types/types.hpp"

#include "network/serializer.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace game::mngrs {

class ServerInstanceManager {
public:
    ServerInstanceManager(physics::Physics& physics);
    ~ServerInstanceManager();

    ServerInstanceManager(const ServerInstanceManager&) = delete;
    ServerInstanceManager& operator=(const ServerInstanceManager&) = delete;

    void setRoot(const std::shared_ptr<types::Instance>& root);

    void rebuildMap();

    bool isDirty() const { return m_dirty; }
    void markDirty() { m_dirty = true; }

    const std::vector<uint8_t>& getMapBuffer() const { return m_mapBuffer; }

    std::unordered_map<uint32_t, std::shared_ptr<types::Part>> getNetworkParts() const {
        std::lock_guard<std::mutex> lock(m_networkPartsMutex);
        return m_networkParts;
    }

private:
    physics::Physics& m_physics;
    std::atomic<uint32_t> m_nextNetworkId{1000};

    std::weak_ptr<types::Instance> m_currentRoot;

    types::Instance::CallbackId m_propSubId = 0;
    types::Instance::CallbackId m_childrenSubId = 0;
    types::Instance::CallbackId m_descendantChildrenSubId = 0;

    mutable std::mutex m_networkPartsMutex;
    std::unordered_map<uint32_t, std::shared_ptr<types::Part>> m_networkParts;

    std::vector<uint8_t> m_mapBuffer;

    bool m_dirty = true;

    uint32_t countSerializableInstances(const std::shared_ptr<types::Instance>& parent) const;

    void collectInstances(const std::shared_ptr<types::Instance>& parent, net::serializer::BinaryWriter& writer);
};

} // namespace game::mngrs