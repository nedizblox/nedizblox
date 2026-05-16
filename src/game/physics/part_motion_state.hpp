#pragma once

#include "../types/part.hpp"

#include <btBulletDynamicsCommon.h>

namespace game::physics {

class PartMotionState : public btMotionState {
public:
    PartMotionState(const btTransform& startTrans, types::Part* part) : m_part(part) {}

    virtual void getWorldTransform(btTransform& worldTrans) const override {
        btTransform t;
        t.setIdentity();

        glm::vec3 pos = m_part->getPosition();
        t.setOrigin(btVector3(pos.x, pos.y, pos.z));

        glm::quat ort = m_part->getOrientation();
        btQuaternion quat(ort.x, ort.y, ort.z, ort.w);
        t.setRotation(quat);

        worldTrans = t;
    }

    virtual void setWorldTransform(const btTransform& worldTrans) override {
        btVector3 pos = worldTrans.getOrigin();
        btQuaternion q = worldTrans.getRotation();

        glm::quat orientation(q.getW(), q.getX(), q.getY(), q.getZ());

        m_part->setPosition(glm::vec3(pos.x(), pos.y(), pos.z()));
        m_part->setOrientation(orientation);
    }

private:
    types::Part* m_part;
};

} // namespace game::physics