#pragma once

#include "../types/part.hpp"

#include <btBulletDynamicsCommon.h>

namespace game::physics {

class PartMotionState : public btMotionState {
public:
    PartMotionState(const btTransform& startTrans, types::Part* part) : m_part(part) {}

    void getWorldTransform(btTransform& worldTrans) const override {
        if (!m_part)
            return;

        btTransform t;
        t.setIdentity();

        glm::vec3 pos = m_part->getPosition();
        t.setOrigin(btVector3(pos.x, pos.y, pos.z));

        glm::quat ort = m_part->getOrientation();
        btQuaternion quat(ort.x, ort.y, ort.z, ort.w);
        t.setRotation(quat);

        worldTrans = t;
    }

    void setWorldTransform(const btTransform& worldTrans) override {
        if (!m_part)
            return;

        btVector3 pos = worldTrans.getOrigin();
        btQuaternion q = worldTrans.getRotation();

        glm::vec3 position(pos.x(), pos.y(), pos.z());
        glm::quat orientation(q.getW(), q.getX(), q.getY(), q.getZ());

        m_part->setPosition(position, true);
        m_part->setOrientation(orientation, true);

        if (pos.y() < kKillHeight) {
            m_pendingDestroy = true;
        }
    }

    bool isPendingDestroy() const { return m_pendingDestroy; }

    types::Part* getPart() const { return m_part; }

    void detachPart() {
        m_part = nullptr;
        m_pendingDestroy = false;
    }

private:
    static constexpr float kKillHeight = -500.0f;

    types::Part* m_part;
    bool m_pendingDestroy = false;
};

} // namespace game::physics