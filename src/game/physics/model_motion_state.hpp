#pragma once

#include "../types/model.hpp"

#include <btBulletDynamicsCommon.h>

namespace game::physics {

class ModelMotionState : public btMotionState {
public:
    ModelMotionState(const btTransform& startTrans, types::Model* model) : m_model(model) {}

    virtual void getWorldTransform(btTransform& worldTrans) const override {
        if (!m_model)
            return;

        btTransform t;
        t.setIdentity();

        glm::mat4 pivot = m_model->getPivot();
        glm::vec3 pos = glm::vec3(pivot[3]);
        t.setOrigin(btVector3(pos.x, pos.y, pos.z));

        glm::quat ort = glm::toQuat(pivot);
        btQuaternion quat(ort.x, ort.y, ort.z, ort.w);
        t.setRotation(quat);

        worldTrans = t;
    }

    virtual void setWorldTransform(const btTransform& worldTrans) override {
        if (!m_model)
            return;

        btVector3 pos = worldTrans.getOrigin();
        btQuaternion q = worldTrans.getRotation();

        glm::quat orientation(q.getW(), q.getX(), q.getY(), q.getZ());
        glm::vec3 position(pos.x(), pos.y(), pos.z());

        glm::mat4 pivot = glm::translate(glm::mat4(1.0f), position) * glm::toMat4(orientation);

        m_model->setPivot(pivot, true);

        if (pos.y() < kKillHeight) {
            m_pendingDestroy = true;
        }
    }

    bool isPendingDestroy() const { return m_pendingDestroy; }

    types::Model* getModel() const { return m_model; }

    void detachModel() {
        m_model = nullptr;
        m_pendingDestroy = false;
    }

private:
    static constexpr float kKillHeight = -500.0f;

    types::Model* m_model;
    bool m_pendingDestroy = false;
};

} // namespace game::physics