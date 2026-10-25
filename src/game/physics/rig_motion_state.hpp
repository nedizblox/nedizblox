#pragma once

#include "body_motion_state.hpp"

#include "../types/rig.hpp"

namespace game::physics {

class RigMotionState : public BodyMotionState {
public:
    RigMotionState(const btTransform& startTrans, types::Rig* rig) :
        m_rig(rig) {
    }

    void getWorldTransform(btTransform& worldTrans) const override {
        if (!m_rig)
            return;

        btTransform t;
        t.setIdentity();

        glm::mat4 pivot = m_rig->getPivot();
        glm::vec3 pos = glm::vec3(pivot[3]);
        t.setOrigin(btVector3(pos.x, pos.y, pos.z));

        glm::quat ort = glm::toQuat(pivot);
        btQuaternion quat(ort.x, ort.y, ort.z, ort.w);
        t.setRotation(quat);

        worldTrans = t;
    }

    void setWorldTransform(const btTransform& worldTrans) override {
        if (!m_rig)
            return;

        btVector3 pos = worldTrans.getOrigin();
        btQuaternion q = worldTrans.getRotation();

        glm::quat orientation(q.getW(), q.getX(), q.getY(), q.getZ());
        glm::vec3 position(pos.x(), pos.y(), pos.z());

        glm::mat4 pivot = glm::translate(glm::mat4(1.0f), position) *
                          glm::toMat4(orientation);

        m_rig->setPivot(pivot, true);

        if (pos.y() < kKillHeight) {
            m_pendingDestroy = true;
        }
    }

    types::Instance* getTarget() const override {
        return m_rig;
    }

    types::Rig* getRig() const {
        return m_rig;
    }

    void detach() override {
        m_rig = nullptr;
        m_pendingDestroy = false;
    }

private:
    types::Rig* m_rig;
};

} // namespace game::physics